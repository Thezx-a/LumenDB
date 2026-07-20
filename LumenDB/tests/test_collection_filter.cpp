#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <random>
#include <cstdio>
#include "lumendb/collection.h"
#include "lumendb/filter.h"
#include "lumendb/storage/document_store.h"

using namespace lumendb;

class CollectionFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ::system("rm -rf /tmp/lumendb_filter_test");
        config_.dim = 8;
        config_.metric = DistanceMetric::L2;
        config_.hnsw_m = 16;
        config_.hnsw_ef_construction = 100;
        config_.hnsw_ef_search = 40;
        coll_ = std::make_unique<Collection>(config_, "/tmp/lumendb_filter_test");

        std::mt19937 rng(123);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        for (int i = 0; i < 50; ++i) {
            std::vector<float> v(config_.dim);
            for (size_t d = 0; d < config_.dim; ++d) v[d] = dist(rng);
            storage::DocumentMeta meta;
            meta.text = "doc_" + std::to_string(i);
            meta.tags = (i % 2 == 0) ? "even" : "odd";
            meta.timestamp = i * 100;
            coll_->add(v.data(), meta);
        }
    }

    void TearDown() override {
        coll_.reset();
        ::system("rm -rf /tmp/lumendb_filter_test");
    }

    CollectionConfig config_;
    std::unique_ptr<Collection> coll_;
};

TEST_F(CollectionFilterTest, FilterByTags) {
    std::vector<float> query(config_.dim, 0.0f);
    auto filter = FilterNode::eq("tags", "even");
    auto results = coll_->searchWithFilter(query.data(), 10, filter);
    EXPECT_GE(results.size(), 1u);
    for (auto& r : results) {
        auto meta = coll_->getMeta(r.id);
        ASSERT_TRUE(meta.has_value());
        EXPECT_EQ(meta->tags, "even");
    }
}

TEST_F(CollectionFilterTest, FilterByTimestamp) {
    std::vector<float> query(config_.dim, 0.0f);
    auto filter = FilterNode::gt("timestamp", "2000");
    auto results = coll_->searchWithFilter(query.data(), 10, filter);
    EXPECT_GE(results.size(), 1u);
    for (auto& r : results) {
        auto meta = coll_->getMeta(r.id);
        ASSERT_TRUE(meta.has_value());
        EXPECT_GT(meta->timestamp, 2000);
    }
}

TEST_F(CollectionFilterTest, NoResultsForImpossibleFilter) {
    std::vector<float> query(config_.dim, 0.0f);
    auto filter = FilterNode::eq("tags", "nonexistent_tag_xyz");
    auto results = coll_->searchWithFilter(query.data(), 10, filter);
    EXPECT_TRUE(results.empty());
}

TEST_F(CollectionFilterTest, GetMeta) {
    auto meta = coll_->getMeta(1);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->text, "doc_0");
    EXPECT_EQ(meta->tags, "even");
    EXPECT_EQ(meta->timestamp, 0);
}
