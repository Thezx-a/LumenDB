#include "dv/filter.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace dv {

static bool tryNumeric(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return false;
    // Check if both strings are purely numeric (allowing leading minus and dot)
    auto isNumeric = [](const std::string& s) {
        if (s.empty()) return false;
        const char* p = s.c_str();
        if (*p == '-') ++p;
        if (*p == '\0') return false;
        while (*p) {
            if ((*p < '0' || *p > '9') && *p != '.') return false;
            ++p;
        }
        return true;
    };
    return isNumeric(a) && isNumeric(b);
}

static bool compareNumeric(const std::string& a, const std::string& b, FilterOp op) {
    double va = std::strtod(a.c_str(), nullptr);
    double vb = std::strtod(b.c_str(), nullptr);
    switch (op) {
        case FilterOp::Equals: return va == vb;
        case FilterOp::NotEquals: return va != vb;
        case FilterOp::GreaterThan: return va > vb;
        case FilterOp::LessThan: return va < vb;
        case FilterOp::GreaterEqual: return va >= vb;
        case FilterOp::LessEqual: return va <= vb;
        default: return false;
    }
}

bool evaluateFilter(const FilterNode& filter, uint64_t id, const FieldAccessor& accessor) {
    if (filter.isLeaf()) {
        std::string fieldVal = accessor(id, filter.field);
        if (fieldVal.empty() && filter.op != FilterOp::NotEquals) return false;

        // For comparison operators, try numeric first, fall back to string
        if (filter.op == FilterOp::Equals || filter.op == FilterOp::NotEquals ||
            filter.op == FilterOp::GreaterThan || filter.op == FilterOp::LessThan ||
            filter.op == FilterOp::GreaterEqual || filter.op == FilterOp::LessEqual) {
            if (tryNumeric(fieldVal, filter.value)) {
                return compareNumeric(fieldVal, filter.value, filter.op);
            }
        }

        switch (filter.op) {
            case FilterOp::Equals:
                return fieldVal == filter.value;
            case FilterOp::NotEquals:
                return fieldVal != filter.value;
            case FilterOp::Contains:
                return fieldVal.find(filter.value) != std::string::npos;
            case FilterOp::GreaterThan:
                return fieldVal > filter.value;
            case FilterOp::LessThan:
                return fieldVal < filter.value;
            case FilterOp::GreaterEqual:
                return fieldVal >= filter.value;
            case FilterOp::LessEqual:
                return fieldVal <= filter.value;
            default:
                return false;
        }
    }

    switch (filter.op) {
        case FilterOp::And:
            for (auto& child : filter.children)
                if (!evaluateFilter(child, id, accessor)) return false;
            return true;
        case FilterOp::Or:
            for (auto& child : filter.children)
                if (evaluateFilter(child, id, accessor)) return true;
            return false;
        case FilterOp::Not:
            return filter.children.empty() ? false : !evaluateFilter(filter.children[0], id, accessor);
        default:
            return false;
    }
}

} // namespace dv
