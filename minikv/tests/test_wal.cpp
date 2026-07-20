#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/stat.h>
#include "core/wal.h"
using namespace minikv::core;

TEST(WALTest, AppendAndReplay) {
    std::string path = "/tmp/minikv_wal_test.log";
    ::unlink(path.c_str());
    { WAL wal(path); wal.append(minikv::Slice("r1")); wal.append(minikv::Slice("r2")); wal.sync(); }
    { WAL wal(path); auto r = wal.replay(); ASSERT_EQ(r.size(), 2u); EXPECT_EQ(r[0], "r1"); }
    ::unlink(path.c_str());
}

TEST(WALTest, Truncate) {
    std::string path = "/tmp/minikv_wal_trunc.log";
    ::unlink(path.c_str());
    { WAL wal(path); wal.append(minikv::Slice("data")); wal.truncate(); }
    struct stat st;
    EXPECT_NE(::stat(path.c_str(), &st), 0);
}
