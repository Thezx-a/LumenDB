#include "filter.h"
#include <iostream>
#include <cassert>
#include <vector>

void demo_parse_and_evaluate() {
    std::cout << "=== Filter Parse & Evaluate Demo ===" << std::endl;

    // Parse: age > 25 AND city == 'Beijing'
    FilterParser parser("age > 25 AND city == 'Beijing'");
    FilterNode filter = parser.Parse();
    FilterEvaluator evaluator;

    // Create sample documents
    std::vector<DocumentMeta> docs = {
        {{"name", "Alice"}, {"age", "30"}, {"city", "Beijing"}},
        {{"name", "Bob"},   {"age", "22"}, {"city", "Beijing"}},
        {{"name", "Charlie"}, {"age", "28"}, {"city", "Shanghai"}},
        {{"name", "Diana"}, {"age", "35"}, {"city", "Beijing"}},
    };

    std::cout << "Filter: age > 25 AND city == 'Beijing'" << std::endl;
    for (const auto& doc : docs) {
        bool match = evaluator.Evaluate(filter, doc);
        std::cout << "  " << doc.at("name")
                  << " (age=" << doc.at("age")
                  << ", city=" << doc.at("city") << ")"
                  << " -> " << (match ? "MATCH" : "skip") << std::endl;
    }
}

void demo_complex_filters() {
    std::cout << "\n=== Complex Filter Demo ===" << std::endl;

    std::vector<DocumentMeta> docs = {
        {{"source", "wiki"}, {"tags", "python"}, {"date", "2024"}},
        {{"source", "wiki"}, {"tags", "rust"},  {"date", "2023"}},
        {{"source", "email"}, {"tags", "python"}, {"date", "2024"}},
        {{"source", "blog"}, {"tags", "cpp"},  {"date", "2022"}},
    };

    // Test 1: OR filter
    {
        FilterParser p("source == 'wiki' OR source == 'email'");
        FilterNode f = p.Parse();
        FilterEvaluator ev;
        std::cout << "Filter: source == 'wiki' OR source == 'email'" << std::endl;
        for (const auto& doc : docs) {
            if (ev.Evaluate(f, doc)) {
                std::cout << "  MATCH: " << doc.at("source") << " / " << doc.at("tags") << std::endl;
            }
        }
    }

    // Test 2: NOT filter
    {
        FilterParser p("NOT source == 'blog'");
        FilterNode f = p.Parse();
        FilterEvaluator ev;
        std::cout << "\nFilter: NOT source == 'blog'" << std::endl;
        for (const auto& doc : docs) {
            if (ev.Evaluate(f, doc)) {
                std::cout << "  MATCH: " << doc.at("source") << std::endl;
            }
        }
    }

    // Test 3: Numeric comparison
    {
        FilterParser p("date >= 2024");
        FilterNode f = p.Parse();
        FilterEvaluator ev;
        std::cout << "\nFilter: date >= 2024" << std::endl;
        for (const auto& doc : docs) {
            if (ev.Evaluate(f, doc)) {
                std::cout << "  MATCH: " << doc.at("source")
                          << " (date=" << doc.at("date") << ")" << std::endl;
            }
        }
    }

    // Test 4: Combined with parentheses
    {
        FilterParser p("(source == 'wiki' OR source == 'blog') AND tags == 'python'");
        FilterNode f = p.Parse();
        FilterEvaluator ev;
        std::cout << "\nFilter: (source == 'wiki' OR source == 'blog') AND tags == 'python'" << std::endl;
        for (const auto& doc : docs) {
            if (ev.Evaluate(f, doc)) {
                std::cout << "  MATCH: " << doc.at("source")
                          << " / " << doc.at("tags") << std::endl;
            }
        }
    }
}

void demo_programmatic_ast() {
    std::cout << "\n=== Programmatic AST Construction ===" << std::endl;

    // Build filter programmatically: age > 25 AND city == 'Beijing'
    auto filter = FilterNode::And(
        FilterNode::Gt("age", "25"),
        FilterNode::Eq("city", "Beijing")
    );

    FilterEvaluator evaluator;

    DocumentMeta doc1 = {{"age", "30"}, {"city", "Beijing"}};
    DocumentMeta doc2 = {{"age", "20"}, {"city", "Beijing"}};
    DocumentMeta doc3 = {{"age", "30"}, {"city", "Shanghai"}};

    assert(evaluator.Evaluate(filter, doc1) == true);
    assert(evaluator.Evaluate(filter, doc2) == false);
    assert(evaluator.Evaluate(filter, doc3) == false);
    std::cout << "Programmatic AST construction test passed!" << std::endl;
}

void demo_selectivity() {
    std::cout << "\n=== Selectivity Estimation Demo ===" << std::endl;

    std::vector<DocumentMeta> docs;
    // Generate 1000 sample documents
    for (int i = 0; i < 1000; i++) {
        std::string age = std::to_string(18 + (i % 50));
        std::string city = (i % 3 == 0) ? "Beijing" : (i % 3 == 1) ? "Shanghai" : "Guangzhou";
        std::string source = (i % 5 == 0) ? "wiki" : "other";
        docs.push_back({{"age", age}, {"city", city}, {"source", source}});
    }

    FilterEvaluator evaluator;

    // Estimate selectivity by sampling
    auto measure = [&](const std::string& expr, int sample_size) -> double {
        FilterParser p(expr);
        FilterNode f = p.Parse();
        int matches = 0;
        for (int i = 0; i < std::min(sample_size, static_cast<int>(docs.size())); i++) {
            if (evaluator.Evaluate(f, docs[i])) matches++;
        }
        return static_cast<double>(matches) / std::min(sample_size, static_cast<int>(docs.size()));
    };

    std::cout << "city == 'Beijing' selectivity: "
              << measure("city == 'Beijing'", 100) << std::endl;
    std::cout << "age > 25 selectivity: "
              << measure("age > 25", 100) << std::endl;
    std::cout << "city == 'Beijing' AND age > 25 selectivity: "
              << measure("city == 'Beijing' AND age > 25", 100) << std::endl;
}

int main() {
    demo_parse_and_evaluate();
    demo_complex_filters();
    demo_programmatic_ast();
    demo_selectivity();
    std::cout << "\nAll filter demos completed successfully!" << std::endl;
    return 0;
}
