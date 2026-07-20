#include "skiplist.h"

// SkipList is a header-only template. This file provides explicit instantiation
// for common types to avoid duplicate symbol issues if linked across TUs.

template class SkipList<std::string, std::string>;
template class SkipList<std::string, int64_t>;
template class SkipList<int64_t, std::string>;
