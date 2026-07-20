# MiniKV 设计文档

## 1. LSM-Tree 架构

MiniKV 采用 Log-Structured Merge-Tree (LSM-Tree) 架构，核心思想：

- **写入路径**: WAL → MemTable → Immutable MemTable → SSTable (flush)
- **读取路径**: MemTable → Immutable → L0 SSTables → L1+ SSTables
- **后台**: Compaction 合并 SSTable，控制读放大和空间放大

## 2. 核心组件

### SkipList (MemTable)
- 概率数据结构，期望 O(logN) 查询/插入
- p=0.5, maxLevel=32
- 读写锁保证并发安全

### Bloom Filter
- Double hashing (MurmurHash2)
- 每个 SSTable 对应一个 Bloom Filter

### WAL
- 顺序追加写
- 格式: [crc(4B)][len(4B)][data]
- 崩溃恢复: replay 按顺序重放

### SSTable
- Data Block (4KB, 前缀压缩) → Meta Block (Bloom) → Index Block → Footer (48B)
- 查找: Bloom → 二分 Index → 读 Data Block

### Compaction
- L0→L1: 处理 key 范围重叠
- Ln→Ln+1: 归并排序，保留最新版本
- Tombstone 在最后一层真正删除

## 3. 文件格式

(MiniKV/src/core/sstable_builder.cpp 中实现)

## 4. 并发模型

- 读写锁: MemTable (shared_mutex)
- 后台线程: flush + compaction (ThreadPool)
- 网络层: epoll reactor + connection per-fd
