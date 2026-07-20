#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>
#include <optional>
#include <stdexcept>
#include <sstream>

enum class FilterOp {
    EQ, NE, GT, GTE, LT, LTE,
    AND, OR, NOT
};

struct FilterNode {
    FilterOp op;
    std::string field;
    std::string value; // string representation of the value
    std::vector<FilterNode> children;

    // Leaf node: field op value
    static FilterNode Eq(const std::string& field, const std::string& val) {
        return {FilterOp::EQ, field, val, {}};
    }
    static FilterNode Ne(const std::string& field, const std::string& val) {
        return {FilterOp::NE, field, val, {}};
    }
    static FilterNode Gt(const std::string& field, const std::string& val) {
        return {FilterOp::GT, field, val, {}};
    }
    static FilterNode Gte(const std::string& field, const std::string& val) {
        return {FilterOp::GTE, field, val, {}};
    }
    static FilterNode Lt(const std::string& field, const std::string& val) {
        return {FilterOp::LT, field, val, {}};
    }
    static FilterNode Lte(const std::string& field, const std::string& val) {
        return {FilterOp::LTE, field, val, {}};
    }

    // Composite nodes
    static FilterNode And(FilterNode a, FilterNode b) {
        return {FilterOp::AND, "", "", {std::move(a), std::move(b)}};
    }
    static FilterNode Or(FilterNode a, FilterNode b) {
        return {FilterOp::OR, "", "", {std::move(a), std::move(b)}};
    }
    static FilterNode Not(FilterNode a) {
        return {FilterOp::NOT, "", "", {std::move(a)}};
    }
};

// Document metadata: a simple map from field name to string value
using DocumentMeta = std::unordered_map<std::string, std::string>;

class FilterEvaluator {
public:
    bool Evaluate(const FilterNode& node, const DocumentMeta& meta) const {
        switch (node.op) {
            case FilterOp::EQ: {
                auto it = meta.find(node.field);
                if (it == meta.end()) return false;
                return compareValues(it->second, node.value) == 0;
            }
            case FilterOp::NE: {
                auto it = meta.find(node.field);
                if (it == meta.end()) return true;
                return compareValues(it->second, node.value) != 0;
            }
            case FilterOp::GT: {
                auto it = meta.find(node.field);
                if (it == meta.end()) return false;
                return compareValues(it->second, node.value) > 0;
            }
            case FilterOp::GTE: {
                auto it = meta.find(node.field);
                if (it == meta.end()) return false;
                return compareValues(it->second, node.value) >= 0;
            }
            case FilterOp::LT: {
                auto it = meta.find(node.field);
                if (it == meta.end()) return false;
                return compareValues(it->second, node.value) < 0;
            }
            case FilterOp::LTE: {
                auto it = meta.find(node.field);
                if (it == meta.end()) return false;
                return compareValues(it->second, node.value) <= 0;
            }
            case FilterOp::AND: {
                for (const auto& child : node.children) {
                    if (!Evaluate(child, meta)) return false;
                }
                return true;
            }
            case FilterOp::OR: {
                for (const auto& child : node.children) {
                    if (Evaluate(child, meta)) return true;
                }
                return false;
            }
            case FilterOp::NOT:
                return !Evaluate(node.children[0], meta);
        }
        return false;
    }

private:
    // Try numeric comparison, fall back to lexicographic
    static int compareValues(const std::string& a, const std::string& b) {
        try {
            double da = std::stod(a);
            double db = std::stod(b);
            if (da < db) return -1;
            if (da > db) return 1;
            return 0;
        } catch (...) {
            if (a < b) return -1;
            if (a > b) return 1;
            return 0;
        }
    }
};

// Recursive descent parser: "age > 25 AND city == 'Beijing'"
class FilterParser {
public:
    explicit FilterParser(const std::string& input) : input_(input), pos_(0) {}

    FilterNode Parse() {
        auto node = parseOr();
        skipWhitespace();
        if (pos_ < input_.size()) {
            throw std::runtime_error("Unexpected characters at position " + std::to_string(pos_));
        }
        return node;
    }

private:
    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(input_[pos_])) ++pos_;
    }

    char peek() {
        skipWhitespace();
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    char advance() {
        skipWhitespace();
        return pos_ < input_.size() ? input_[pos_++] : '\0';
    }

    bool match(const std::string& s) {
        skipWhitespace();
        if (input_.compare(pos_, s.size(), s) == 0) {
            pos_ += s.size();
            return true;
        }
        return false;
    }

    std::string parseIdentifier() {
        skipWhitespace();
        size_t start = pos_;
        while (pos_ < input_.size() && (std::isalnum(input_[pos_]) || input_[pos_] == '_' || input_[pos_] == '.')) {
            ++pos_;
        }
        if (pos_ == start) throw std::runtime_error("Expected identifier at " + std::to_string(pos_));
        return input_.substr(start, pos_ - start);
    }

    std::string parseValue() {
        skipWhitespace();
        if (peek() == '\'') {
            advance(); // skip quote
            size_t start = pos_;
            while (pos_ < input_.size() && input_[pos_] != '\'') ++pos_;
            std::string val = input_.substr(start, pos_ - start);
            if (pos_ < input_.size()) advance(); // skip closing quote
            return val;
        }
        if (peek() == '"') {
            advance();
            size_t start = pos_;
            while (pos_ < input_.size() && input_[pos_] != '"') ++pos_;
            std::string val = input_.substr(start, pos_ - start);
            if (pos_ < input_.size()) advance();
            return val;
        }
        // number or bare word
        size_t start = pos_;
        while (pos_ < input_.size() && (std::isalnum(input_[pos_]) || input_[pos_] == '.' || input_[pos_] == '-' || input_[pos_] == '+')) {
            ++pos_;
        }
        if (pos_ == start) throw std::runtime_error("Expected value at " + std::to_string(pos_));
        return input_.substr(start, pos_ - start);
    }

    FilterOp parseCompareOp() {
        skipWhitespace();
        if (match(">=")) return FilterOp::GTE;
        if (match("<=")) return FilterOp::LTE;
        if (match("==") || match("=")) return FilterOp::EQ;
        if (match("!=")) return FilterOp::NE;
        if (match(">")) return FilterOp::GT;
        if (match("<")) return FilterOp::LT;
        throw std::runtime_error("Expected comparison operator at " + std::to_string(pos_));
    }

    FilterNode parseComparison() {
        if (match("(")) {
            auto node = parseOr();
            if (!match(")")) throw std::runtime_error("Expected ')' at " + std::to_string(pos_));
            return node;
        }
        if (match("NOT") || match("not")) {
            return FilterNode::Not(parseComparison());
        }
        std::string field = parseIdentifier();
        FilterOp op = parseCompareOp();
        std::string value = parseValue();
        return {op, field, value, {}};
    }

    FilterNode parseAnd() {
        auto left = parseComparison();
        while (match("AND") || match("and")) {
            auto right = parseComparison();
            left = FilterNode::And(std::move(left), std::move(right));
        }
        return left;
    }

    FilterNode parseOr() {
        auto left = parseAnd();
        while (match("OR") || match("or")) {
            auto right = parseAnd();
            left = FilterNode::Or(std::move(left), std::move(right));
        }
        return left;
    }

    std::string input_;
    size_t pos_;
};
