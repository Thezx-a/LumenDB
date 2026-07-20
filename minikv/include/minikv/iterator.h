#pragma once
#include "minikv/slice.h"
#include "minikv/status.h"

namespace minikv {

class Iterator {
public:
    virtual ~Iterator() = default;
    virtual bool valid() const = 0;
    virtual void seekToFirst() = 0;
    virtual void seek(const Slice& target) = 0;
    virtual void next() = 0;
    virtual Slice key() const = 0;
    virtual Slice value() const = 0;
    virtual Status status() const = 0;
};

}  // namespace minikv
