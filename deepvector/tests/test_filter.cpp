#include <gtest/gtest.h>
#include <string>
#include "dv/filter.h"

using namespace lumendb;

TEST(FilterTest, EqualsTrue) {
    auto filter = FilterNode::eq("tags", "urgent,important");
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "tags") return "urgent,important";
        return "";
    };
    EXPECT_TRUE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, EqualsFalse) {
    auto filter = FilterNode::eq("tags", "spam");
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "tags") return "important";
        return "";
    };
    EXPECT_FALSE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, ContainsTrue) {
    auto filter = FilterNode::contains("tags", "urgent");
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "tags") return "urgent,important";
        return "";
    };
    EXPECT_TRUE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, GtTimestamp) {
    auto filter = FilterNode::gt("timestamp", "1000");
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "timestamp") return "2000";
        return "";
    };
    EXPECT_TRUE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, AndCombination) {
    auto filter = FilterNode::andAlso(
        FilterNode::contains("tags", "urgent"),
        FilterNode::gt("timestamp", "1000")
    );
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "tags") return "urgent,normal";
        if (field == "timestamp") return "2000";
        return "";
    };
    EXPECT_TRUE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, AndCombinationFalse) {
    auto filter = FilterNode::andAlso(
        FilterNode::contains("tags", "urgent"),
        FilterNode::lt("timestamp", "1000")
    );
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "tags") return "urgent,normal";
        if (field == "timestamp") return "2000";
        return "";
    };
    EXPECT_FALSE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, OrCombination) {
    auto filter = FilterNode::orElse(
        FilterNode::eq("tags", "spam"),
        FilterNode::eq("tags", "ham")
    );
    auto accessor = [](uint64_t, const std::string& field) -> std::string {
        if (field == "tags") return "ham";
        return "";
    };
    EXPECT_TRUE(evaluateFilter(filter, 1, accessor));
}

TEST(FilterTest, MissingField) {
    auto filter = FilterNode::eq("nonexistent", "value");
    auto accessor = [](uint64_t, const std::string&) -> std::string { return ""; };
    EXPECT_FALSE(evaluateFilter(filter, 1, accessor));
}
