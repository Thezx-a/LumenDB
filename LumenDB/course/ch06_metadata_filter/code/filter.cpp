#include "filter.h"
#include <iostream>

// FilterParser and FilterEvaluator are header-only.
// This file provides the integration: parse a string and evaluate against documents.

std::vector<DocumentMeta> SearchWithFilter(
    const std::vector<DocumentMeta>& docs,
    const std::string& filter_expr
) {
    FilterParser parser(filter_expr);
    FilterNode filter = parser.Parse();
    FilterEvaluator evaluator;

    std::vector<DocumentMeta> results;
    for (const auto& doc : docs) {
        if (evaluator.Evaluate(filter, doc)) {
            results.push_back(doc);
        }
    }
    return results;
}
