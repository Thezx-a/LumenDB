#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "minikv/slice.h"
#include "minikv/status.h"
#include "minikv/write_batch.h"

namespace minikv {
namespace core {

class WAL {
public:
    explicit WAL(const std::string& path);
    ~WAL();

    Status append(const Slice& data);
    Status sync();
    std::vector<std::string> replay();
    Status truncate();

    bool exists() const;

private:
    std::string path_;
    int fd_;
};

}  // namespace core
}  // namespace minikv
