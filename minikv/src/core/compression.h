#pragma once

#include <cstdint>
#include <string>
#include "minikv/slice.h"
#include "minikv/status.h"

namespace minikv {
namespace core {

// Compression algorithms supported by SSTable data blocks.
//   - kNone:   stored raw (no compression)
//   - kSnappy: Google snappy — very fast, modest compression (~2-3x)
//   - kZstd:   Facebook zstd — strong compression (~3-4x), ~2x snappy CPU
enum class CompressionType : uint8_t {
    kNone   = 0,
    kSnappy = 1,
    kZstd   = 2,
};

// Compress `input` using `type` and write the result to `output`.
// Returns Status::NotSupported if the scheme is unavailable at compile time.
Status compressBlock(CompressionType type, const Slice& input, std::string& output);

// Decompress a buffer previously produced by `compressBlock`.
// `uncompressed_size` is the expected output length; required for Zstd and
// used as a sanity check for Snappy.
Status decompressBlock(CompressionType type,
                       const Slice&  compressed,
                       size_t        uncompressed_size,
                       std::string&  output);

// String name for diagnostics.
const char* compressionName(CompressionType type);

}  // namespace core
}  // namespace minikv