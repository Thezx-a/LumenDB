# TitanKV Storage Engine — Design Notes

This document describes the C++ storage engine that powers TitanKV, including
the LSM-Tree layout, on-disk file formats, and durability model.

> **Status:** As of Phase 1 the engine has been upgraded with block
> compression (WP 1.2.1), Manifest persistence (WP 1.2.4), and the
> InternalKey codec foundation (WP 1.2.2 phase A). MVCC reads, range
> deletes, column families, and OCC transactions remain pending.

---

## 1. Component Map

```
minikv/
├── include/minikv/         Public headers (DB / Options / Slice / Iterator)
├── src/core/               LSM-Tree core
│   ├── db_impl.{h,cpp}     Top-level DB orchestrator (open / put / get / del)
│   ├── memtable.{h,cpp}    SkipList-backed in-memory table
│   ├── skip_list.h         Concurrent skip list (R/W lock)
│   ├── wal.{h,cpp}         Write-ahead log
│   ├── block.{h,cpp}       Sorted data block (LevelDB-style prefix sharing)
│   ├── bloom_filter.h      Per-SST bloom filter (MurmurHash2, persists to .bloom)
│   ├── sstable_builder.{h,cpp}  SSTable writer (compressed data blocks)
│   ├── sstable_reader.{h,cpp}   SSTable reader (transparent decompression)
│   ├── sstable_iterator.{h,cpp} Sequential iterator over one SST
│   ├── memtable_iterator.{h,cpp} Sequential iterator over one MemTable
│   ├── merging_iterator.{h,cpp}  Min-heap merge of children
│   ├── compaction.{h,cpp}  Background compaction manager (L0 -> L1 stub)
│   ├── version.{h,cpp}     In-memory view of LSM level contents
│   ├── manifest.{h,cpp}    Durable record of Version edits (append-only)
│   ├── compression.{h,cpp} Snappy / Zstd wrapper for block payload
│   └── internal_key.{h,cpp} [user_key | seq | type] codec + comparator
└── src/network/            Legacy HTTP server (kept from earlier MiniKV)
```

---

## 2. LSM-Tree Shape

```
        Write ─────► MemTable (SkipList, ~4MB default)
                            │
                            │ flushMemTable()
                            ▼
                      L0  ┌─────────────────┐  (compaction trigger: 4 files)
                          │ file_001.sst     │
                          │ file_002.sst     │
                          │ ...              │
                          └─────────────────┘
                            │ compaction (L0 -> L1)
                            ▼
                      L1  ┌─────────────────┐  (Leveled; size-tiered planned)
                          │ file_010.sst     │
                          │ ...              │
                          └─────────────────┘
                            │ compaction (Ln -> Ln+1)
                            ▼
                       L2 ... L7
```

Default `Options`:

| Field                | Default  | Notes |
|----------------------|----------|-------|
| `memtable_size`      | 4 MB     | flush trigger |
| `block_size`         | 4 KB     | SSTable data block |
| `lru_cache_capacity` | 8 MB     | reserved for block cache |
| `max_level`          | 7        | L0 .. L7 |
| `level0_compaction_trigger` | 4 | L0 file count threshold |
| `wal_sync`           | true     | fdatasync after each batch |
| `bloom_filter_enabled` | true   | one bloom per SST, sidecar `.bloom` |
| `bloom_false_positive_rate` | 0.01 | 7-8 bits/key |
| `compression`        | 1 (snappy) | per-block compression scheme |

---

## 3. On-disk File Formats

### 3.1 SSTable

```
┌─────────────────────────────────────────────────────────┐
│ Data block #0                                          │
│ Data block #1                                          │
│ ...                                                    │
│ Data block #N                                          │
│ Index block (8-byte header, uncompressed)              │
│ Footer (48 bytes)                                      │
└─────────────────────────────────────────────────────────┘
```

**Data block header (13 bytes, WP 1.2.1):**

| Offset | Field               | Size |
|--------|---------------------|------|
| 0      | crc32c of payload   | 4    |
| 4      | physical_size       | 4    |
| 8      | uncompressed_size   | 4    |
| 12     | compression_type    | 1    |

`compression_type` is one of `CompressionType` (`kNone=0 / kSnappy=1 / kZstd=2`).
Reader always trusts the recorded scheme, so an SST written with snappy can be
read even if the engine's default `Options.compression` later changes.

Block payload (after decompression) uses LevelDB-style prefix-shared entries
with a restart array at the tail (`BlockBuilder::add` / `BlockReader::get`).

**Index block:** legacy 8-byte `[crc(4)][size(4)]` header followed by
`[varint last_user_key_len][last_user_key][offset(8)][size(8)]` entries — one
per data block. Used to binary-search the block containing a key.

**Footer (48 bytes, WP 1.2.1):**

| Offset | Field            | Size | Notes |
|--------|------------------|------|-------|
| 0      | index_offset     | 8    | absolute offset of index block |
| 8      | index_size       | 8    | including its 8-byte header |
| 16     | format_version   | 1    | currently `1` |
| 17     | reserved         | 23   | zeroed |
| 40     | magic            | 8    | `0x4D4B53535441424C` ("MKSSTABL") |

### 3.2 Bloom filter sidecar

Stored as `<sst_path>.bloom`. Layout: `[num_hashes(4)][bits_per_key(4)][bits_size(8)][bits...]`.
Bloom is consulted before any block read; a `mightContain` miss lets the
reader skip the block entirely.

### 3.3 WAL

`<db_path>/wal.log`. Records are `[crc(4)][len(4)][payload]`. Payload is a
concatenation of `[type(1)][key_len(4)][val_len(4)][key][value]` entries from
the originating `WriteBatch`. `WAL::truncate()` is called after every
successful MemTable flush so restart only replays unflushed data.

### 3.4 MANIFEST

`<db_path>/MANIFEST`. Append-only log of Version edits with `[crc(4)][size(4)][payload]`
header. Payload:

| Bytes | Field        |
|-------|--------------|
| 1     | type (kReset=0 / kAdd=1 / kDel=2) |
| 4     | level (LE) |
| 8     | file_number (LE) |
| 4     | path length (LE) |
| n     | path bytes |

`Manifest::open()` replays the file end-to-end and reconstructs the in-memory
levels vector. A torn tail record (CRC mismatch / short read) is silently
discarded, giving crash-safe recovery. `Version::addLevelFile` /
`removeLevelFiles` write through to MANIFEST and fsync, so every change to the
LSM topology is durably logged before the WAL is truncated.

---

## 4. Internal Key Codec (WP 1.2.2 Phase A)

`core/internal_key.h` provides the future internal-key encoding while the
legacy hash-based MemTable remains in use:

```
internal_key = user_key_bytes || trailer(8 bytes LE)
trailer      = (seq << 8) | static_cast<uint8_t>(ValueType)
```

Sort order (`InternalKeyCompare`):

1. user_key ascending (byte-wise)
2. seq descending (so latest version sorts first)
3. ValueType ascending (kValue before kDeletion)

This matches the RocksDB convention and supports:

- **MVCC snapshot reads** — filter out entries with seq > snapshot
- **Range deletes** — proper byte-wise ordering on user_key
- **OCC transactions** — read/write set comparison with deterministic order

Phase B of WP 1.2.2 rewires `SkipList` to use `std::string` keys (with
`InternalKeyCompare` as comparator) and propagates the encoding through
MemTable, SSTable builder/reader, and iterators.

---

## 5. Write Path

```
Application
   │
   │ DB::put(key, value) ──or── DB::write(batch)
   ▼
DBImpl::write()
   │ 1. Acquire write_mutex_
   │ 2. Allocate sequence numbers from atomic seq_
   │ 3. Append each op to MemTable (shared_mutex-protected SkipList)
   │ 4. Append batch to WAL; fsync if Options.wal_sync && WriteOptions.sync
   │ 5. maybeFlush(): if MemTable exceeded Options.memtable_size,
   │    swap to immutable and trigger flushMemTable() on the same thread
   ▼
MemTable (SkipList)        WAL
```

`flushMemTable()` writes the immutable MemTable to a new SST in L0,
calls `Version::addLevelFile(0, ...)` (which writes MANIFEST + fsyncs), and
finally truncates the WAL. The background `CompactionManager` polls
`Version::shouldCompactL0()` and triggers `compactL0()` when L0 exceeds 4 files.

---

## 6. Read Path

```
Application
   │
   │ DB::get(key)
   ▼
DBImpl::get()
   │ 1. MemTable::get(key, seq_) → newest matching entry (R-lock)
   │ 2. immutable_memtable_->get(...) if present
   │ 3. For each level 0..N, for each SST in level:
   │      - SSTableReader::open(path) (cached upstream in future)
   │      - BloomFilter::mightContain(key)  → skip if false
   │      - Index binary search → block handle
   │      - readBlock(): read 13B header, validate CRC, decompress payload
   │      - BlockReader::get(key) → value if present
   │ 4. Return first hit (newest seq wins because levels are visited L0..LN)
```

Iterators (`newIterator`) merge MemTable + immutable MemTable + every SST via
`MergingIterator`, which is a min-heap keyed by the iterator's `key()` bytes.

---

## 7. Durability Model

| Event                          | Effect                                                                                       |
|--------------------------------|----------------------------------------------------------------------------------------------|
| Clean shutdown                 | MemTable flushed; WAL truncated; MANIFEST up to date.                                        |
| Crash before WAL fsync         | Last batch lost; MANIFEST + prior SSTs intact.                                               |
| Crash after WAL fsync, before SST flush | WAL replayed on restart → MemTable rebuilt → flush re-creates the missing SST.     |
| Crash during SST write         | Partial SST on disk; MANIFEST does not list it (Version::addLevelFile hasn't run); ignored. |
| Crash after SST + MANIFEST fsync, before WAL truncate | WAL still has the data; on restart WAL replays into MemTable and flushes a duplicate SST. MergingIterator dedupes by key. |
| Crash mid-MANIFEST append      | Tail record has bad CRC / short length; replay discards it. No phantom entries loaded.      |

---

## 8. Pending Work

| WP       | Topic                              | Why it matters                                                |
|----------|------------------------------------|---------------------------------------------------------------|
| 1.2.2 B  | SkipList string-key rewire + MVCC  | Fixes the hash-based internal-key collision risk; enables snapshot reads. |
| 1.2.3    | Range Delete                       | Requires WP 1.2.2 B for correct byte-wise user_key ordering. |
| 1.2.5    | Column Family                      | Multi-namespace isolation; shared WAL with CF-prefix encoding. |
| 1.2.6    | Optimistic transactions (OCC)      | Begin / Commit / Rollback on top of MVCC snapshot reads.     |
| 1.2.7    | Compaction strategy switch         | Leveled (current) vs Size-tiered; benchmarked via `benches/`. |

See `docs/REFACTORING.md` for cross-cutting tracking.