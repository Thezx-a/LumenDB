#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace dv {

enum class FilterOp : uint8_t {
    Equals = 0,
    NotEquals = 1,
    GreaterThan = 2,
    LessThan = 3,
    GreaterEqual = 4,
    LessEqual = 5,
    Contains = 6,
    And = 10,
    Or = 11,
    Not = 12,
};

struct FilterNode {
    FilterOp op;
    std::string field;
    std::string value;
    std::vector<FilterNode> children;

    bool isLeaf() const { return op < FilterOp::And; }
    static FilterNode eq(const std::string& field, const std::string& value) {
        return {FilterOp::Equals, field, value, {}};
    }
    static FilterNode contains(const std::string& field, const std::string& value) {
        return {FilterOp::Contains, field, value, {}};
    }
    static FilterNode gt(const std::string& field, const std::string& value) {
        return {FilterOp::GreaterThan, field, value, {}};
    }
    static FilterNode lt(const std::string& field, const std::string& value) {
        return {FilterOp::LessThan, field, value, {}};
    }
    static FilterNode andAlso(FilterNode a, FilterNode b) {
        return {FilterOp::And, "", "", {std::move(a), std::move(b)}};
    }
    static FilterNode orElse(FilterNode a, FilterNode b) {
        return {FilterOp::Or, "", "", {std::move(a), std::move(b)}};
    }
};

using FieldAccessor = std::function<std::string(uint64_t id, const std::string& field)>;

bool evaluateFilter(const FilterNode& filter, uint64_t id, const FieldAccessor& accessor);

} // namespace dv
