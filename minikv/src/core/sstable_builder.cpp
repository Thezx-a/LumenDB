#include "core/sstable_builder.h"
#include <fcntl.h>
#include <unistd.h>
#include "utils/coding.h"
#include "utils/crc32.h"

namespace minikv {
namespace core {

static const uint64_t kSSTableMagic = 0x4D4B53535441424CULL;  // "MKSSTABL"
static const size_t kFooterSize = 48;

SSTableBuilder::SSTableBuilder(const std::string& path, size_t block_size)
    : path_(path), fd_(-1), block_size_(block_size),
      data_block_(block_size), offset_(0), finished_(false), entry_count_(0) {
    bloom_ = std::make_unique<BloomFilter>(10000, 0.01);
    fd_ = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

SSTableBuilder::~SSTableBuilder() {
    if (!finished_) finish();
    if (fd_ >= 0) ::close(fd_);
}

Status SSTableBuilder::add(uint64_t internalKey, const Slice& key, const Slice& value) {
    char keyBuf[8];
    utils::encodeFixed64(keyBuf, internalKey);
    data_block_.add(Slice(keyBuf, 8), value);
    bloom_->add(key);
    last_key_ = key.toString();
    entry_count_++;
    if (data_block_.size() >= block_size_) {
        return flushDataBlock();
    }
    return Status::Ok();
}

Status SSTableBuilder::flushDataBlock() {
    if (data_block_.empty()) return Status::Ok();
    Slice block_data = data_block_.finish();
    uint32_t crc = utils::crc32c(block_data.data(), static_cast<int>(block_data.size()));
    uint64_t block_offset = offset_;
    char header[8];
    utils::encodeFixed32(header, crc);
    utils::encodeFixed32(header + 4, static_cast<uint32_t>(block_data.size()));
    ssize_t n = ::write(fd_, header, 8);
    if (n != 8) return Status::IOError("SSTable write block header failed");
    n = ::write(fd_, block_data.data(), block_data.size());
    if (n != static_cast<ssize_t>(block_data.size()))
        return Status::IOError("SSTable write block data failed");
    offset_ += 8 + block_data.size();

    // Update index
    std::string indexEntry;
    utils::encodeVariant32(indexEntry, static_cast<uint32_t>(last_key_.size()));
    indexEntry.append(last_key_);
    char handle[16];
    utils::encodeFixed64(handle, block_offset);
    utils::encodeFixed64(handle + 8, block_data.size() + 8);
    indexEntry.append(handle, 16);
    index_block_.append(indexEntry);

    // Reset data block
    data_block_ = BlockBuilder(block_size_);
    return Status::Ok();
}

Status SSTableBuilder::finish() {
    if (finished_) return Status::Ok();
    finished_ = true;
    flushDataBlock();

    // Write bloom filter as meta block
    std::string bloom_data;
    bloom_->persist(path_ + ".bloom");
    // Write bloom inline
    // (simplified: write bloom size as meta block)

    // Write index block
    uint64_t index_offset = offset_;
    uint32_t index_crc = utils::crc32c(index_block_.data(), static_cast<int>(index_block_.size()));
    char idxHeader[8];
    utils::encodeFixed32(idxHeader, index_crc);
    utils::encodeFixed32(idxHeader + 4, static_cast<uint32_t>(index_block_.size()));
    ::write(fd_, idxHeader, 8);
    ::write(fd_, index_block_.data(), index_block_.size());
    offset_ += 8 + index_block_.size();

    // Write footer
    writeFooter();
    ::fdatasync(fd_);
    return Status::Ok();
}

void SSTableBuilder::writeFooter() {
    char footer[kFooterSize];
    std::memset(footer, 0, kFooterSize);
    // Simplified footer: index_offset(8) + index_size(8) + magic(8) + padding
    utils::encodeFixed64(footer, offset_ - (8 + index_block_.size())); // index offset
    utils::encodeFixed64(footer + 8, index_block_.size() + 8);          // index size
    utils::encodeFixed64(footer + 40, kSSTableMagic);
    ::write(fd_, footer, kFooterSize);
    offset_ += kFooterSize;
}

uint64_t SSTableBuilder::fileSize() const { return offset_; }

}  // namespace core
}  // namespace minikv
