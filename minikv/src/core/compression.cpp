#include "core/compression.h"

#include <snappy.h>
#include <zstd.h>

namespace minikv {
namespace core {

Status compressBlock(CompressionType type,
                     const Slice&    input,
                     std::string&    output) {
    switch (type) {
        case CompressionType::kNone:
            output.assign(input.data(), input.size());
            return Status::Ok();
        case CompressionType::kSnappy: {
            size_t bound = snappy::MaxCompressedLength(input.size());
            output.resize(bound);
            size_t actual = 0;
            snappy::RawCompress(input.data(), input.size(),
                                &output[0], &actual);
            output.resize(actual);
            return Status::Ok();
        }
        case CompressionType::kZstd: {
            size_t bound = ZSTD_compressBound(input.size());
            output.resize(bound);
            size_t actual = ZSTD_compress(&output[0], bound,
                                           input.data(), input.size(),
                                           /*level=*/3);
            if (ZSTD_isError(actual)) {
                return Status::IOError(ZSTD_getErrorName(actual));
            }
            output.resize(actual);
            return Status::Ok();
        }
        default:
            return Status::NotSupported("unknown compression type");
    }
}

Status decompressBlock(CompressionType type,
                       const Slice&    compressed,
                       size_t          uncompressed_size,
                       std::string&    output) {
    switch (type) {
        case CompressionType::kNone:
            output.assign(compressed.data(), compressed.size());
            return Status::Ok();
        case CompressionType::kSnappy: {
            if (!snappy::Uncompress(compressed.data(), compressed.size(),
                                     &output)) {
                return Status::Corruption("snappy decompress failed");
            }
            if (output.size() != uncompressed_size) {
                return Status::Corruption("snappy decompressed size mismatch");
            }
            return Status::Ok();
        }
        case CompressionType::kZstd: {
            output.resize(uncompressed_size);
            size_t actual = ZSTD_decompress(&output[0], uncompressed_size,
                                             compressed.data(), compressed.size());
            if (ZSTD_isError(actual)) {
                return Status::Corruption(ZSTD_getErrorName(actual));
            }
            if (actual != uncompressed_size) {
                return Status::Corruption("zstd decompressed size mismatch");
            }
            return Status::Ok();
        }
        default:
            return Status::NotSupported("unknown compression type");
    }
}

const char* compressionName(CompressionType type) {
    switch (type) {
        case CompressionType::kNone:   return "none";
        case CompressionType::kSnappy: return "snappy";
        case CompressionType::kZstd:   return "zstd";
        default:                       return "unknown";
    }
}

}  // namespace core
}  // namespace minikv