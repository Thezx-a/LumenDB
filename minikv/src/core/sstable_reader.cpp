#include "core/sstable_reader.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include "utils/coding.h"

namespace minikv {
namespace core {

static const size_t kFooterSize = 48;
static const uint64_t kSSTableMagic = 0x4D4B53535441424CULL;

std::unique_ptr<SSTableReader> SSTableReader::open(const std::string& path) {
    auto reader = std::unique_ptr<SSTableReader>(new SSTableReader());
    reader->path_ = path;
    reader->fd_ = ::open(path.c_str(), O_RDONLY);
    if (reader->fd_ < 0) return nullptr;

    struct stat st;
    ::fstat(reader->fd_, &st);
    reader->file_size_ = st.st_size;
    if (static_cast<size_t>(st.st_size) < kFooterSize) return nullptr;

    char footer[kFooterSize];
    ::lseek(reader->fd_, st.st_size - kFooterSize, SEEK_SET);
    ::read(reader->fd_, footer, kFooterSize);

    uint64_t magic = utils::decodeFixed64(footer + 40);
    if (magic != kSSTableMagic) return nullptr;

    reader->index_offset_ = utils::decodeFixed64(footer);
    reader->index_size_ = utils::decodeFixed64(footer + 8);

    // Read index block (skip CRC header)
    ::lseek(reader->fd_, reader->index_offset_, SEEK_SET);
    char idxHeader[8];
    ::read(reader->fd_, idxHeader, 8);
    uint32_t idxLen = utils::decodeFixed32(idxHeader + 4);
    std::string idxData(idxLen, '\0');
    ::read(reader->fd_, idxData.data(), idxLen);
    reader->index_data_ = idxData;

    // Parse index entries (simplified)
    size_t offset = 0;
    while (offset < idxData.size()) {
        const char* p = idxData.data() + offset;
        const char* limit = idxData.data() + idxData.size();
        uint32_t keyLen;
        uint32_t consumed;
        if (!utils::decodeVariant32(p, limit, keyLen, consumed)) break;
        p += consumed;
        std::string key(p, keyLen);
        p += keyLen;
        uint64_t blockOffset = utils::decodeFixed64(p);
        uint64_t blockSize = utils::decodeFixed64(p + 8);
        reader->index_entries_.push_back({key, {blockOffset, blockSize}});
        offset = (p + 16) - idxData.data();
    }

    reader->bloom_ = BloomFilter::load(path + ".bloom");

    return reader;
}

std::optional<std::string> SSTableReader::get(const Slice& key) const {
    if (index_entries_.empty()) return std::nullopt;
    if (bloom_ && !bloom_->mightContain(key)) return std::nullopt;

    // Binary search index to find the block that might contain key
    auto it = std::upper_bound(
        index_entries_.begin(), index_entries_.end(), key.toString(),
        [](const std::string& k, const std::pair<std::string, BlockHandle>& entry) {
            return k < entry.first;
        });
    if (it == index_entries_.begin()) return std::nullopt;
    --it;

    // Read the data block
    const BlockHandle& handle = it->second;
    ::lseek(fd_, handle.offset, SEEK_SET);
    char blockHeader[8];
    ::read(fd_, blockHeader, 8);
    uint32_t blockLen = utils::decodeFixed32(blockHeader + 4);
    std::string blockData(blockLen, '\0');
    ::read(fd_, blockData.data(), blockLen);

    BlockReader reader{Slice(blockData)};
    return reader.get(key);
}

Status SSTableReader::scan(const Slice& start, const Slice& end,
                            std::function<void(const Slice&, const Slice&)> callback) const {
    // Simplified: linear scan all blocks
    for (const auto& [lastKey, handle] : index_entries_) {
        if (!end.empty() && lastKey > end.toString()) break;
        ::lseek(fd_, handle.offset, SEEK_SET);
        char blockHeader[8];
        ::read(fd_, blockHeader, 8);
        uint32_t blockLen = utils::decodeFixed32(blockHeader + 4);
        std::string blockData(blockLen, '\0');
        ::read(fd_, blockData.data(), blockLen);
        // Full scan would go here
    }
    return Status::Ok();
}

}  // namespace core
}  // namespace minikv
