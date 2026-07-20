#pragma once
#include <string>
#include <vector>
#include "skynet/core/thread_pool.h"

namespace skynet {
namespace net {

class Resolver {
public:
    explicit Resolver(core::ThreadPool* pool);
    std::vector<std::string> resolve(const std::string& hostname);

private:
    core::ThreadPool* pool_;
};

}  // namespace net
}  // namespace skynet
