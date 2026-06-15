#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>
#include "TaskStore.h"
#include "FileIO.h"

static const std::string kTestFile = "/tmp/fileio_test.bin";
static const std::string kSwapFile = "/tmp/fileio_test.bin.swp";

// Helper: remove test files before/after each test
struct FileIOTest : ::testing::Test {
    void SetUp() override    { std::remove(kTestFile.c_str()); std::remove(kSwapFile.c_str()); }
    void TearDown() override { std::remove(kTestFile.c_str()); std::remove(kSwapFile.c_str()); }
};

// ── round-trip: empty store ───────────────────────────────────────────────────

TEST_F(FileIOTest, RoundTripEmpty) {
    TaskStore src;
    FileIO::save(kTestFile, src);

    TaskStore dst;
    FileIO::load(kTestFile, dst);

    EXPECT_EQ(dst.size(), 0u);
    // nextId should be 0 on empty store
    unsigned next = dst.create("x", "", 0, 0, -1);
    EXPECT_EQ(next, 0u);
}

// ── round-trip: single task, no deps ─────────────────────────────────────────

TEST_F(FileIOTest, RoundTripSingleNoDeps) {
    TaskStore src;
    unsigned id = src.create("hello", "world", 2, 1, 9999);
    const Task& orig = src.get(id);
    int64_t savedCreatedAt = orig.createdAt;

    FileIO::save(kTestFile, src);

    TaskStore dst;
    FileIO::load(kTestFile, dst);

    ASSERT_EQ(dst.size(), 1u);
    const Task& t = dst.get(id);
    EXPECT_EQ(t.title,       "hello");
    EXPECT_EQ(t.description, "world");
    EXPECT_EQ(t.priority,    2);
    EXPECT_EQ(t.status,      1);
    EXPECT_EQ(t.dueDate,     int64_t(9999));
    EXPECT_EQ(t.createdAt,   savedCreatedAt);
    EXPECT_EQ(t.deps.size(), 0u);
}

// ── round-trip: dueDate == -1 ─────────────────────────────────────────────────

TEST_F(FileIOTest, RoundTripNoDueDate) {
    TaskStore src;
    unsigned id = src.create("no due", "", 0, 0, -1);

    FileIO::save(kTestFile, src);

    TaskStore dst;
    FileIO::load(kTestFile, dst);

    EXPECT_EQ(dst.get(id).dueDate, int64_t(-1));
}

// ── round-trip: task with multiple deps ──────────────────────────────────────

TEST_F(FileIOTest, RoundTripMultipleDeps) {
    TaskStore src;
    unsigned a = src.create("a", "", 0, 0, -1);
    unsigned b = src.create("b", "", 0, 0, -1);
    unsigned c = src.create("c", "", 0, 0, -1);
    unsigned d = src.create("d", "", 0, 0, -1);

    Task t = src.get(d);
    t.deps.pushBack(a);
    t.deps.pushBack(b);
    t.deps.pushBack(c);
    src.update(d, t);

    FileIO::save(kTestFile, src);

    TaskStore dst;
    FileIO::load(kTestFile, dst);

    const Task& loaded = dst.get(d);
    ASSERT_EQ(loaded.deps.size(), 3u);
    EXPECT_EQ(loaded.deps[0], a);
    EXPECT_EQ(loaded.deps[1], b);
    EXPECT_EQ(loaded.deps[2], c);
}

// ── round-trip: multiple tasks, nextId re-derived correctly ──────────────────

TEST_F(FileIOTest, RoundTripMultipleTasks) {
    TaskStore src;
    src.create("t0", "d0", 0, 0, -1);
    src.create("t1", "d1", 1, 1, 1000);
    unsigned last = src.create("t2", "d2", 3, 2, 2000);

    FileIO::save(kTestFile, src);

    TaskStore dst;
    FileIO::load(kTestFile, dst);

    ASSERT_EQ(dst.size(), 3u);
    EXPECT_EQ(dst.get(0).title, "t0");
    EXPECT_EQ(dst.get(1).title, "t1");
    EXPECT_EQ(dst.get(2).title, "t2");

    // nextId should be max_id + 1 = 3
    unsigned next = dst.create("t3", "", 0, 0, -1);
    EXPECT_EQ(next, last + 1);
}

// ── file appearance / disappearance ──────────────────────────────────────────

TEST_F(FileIOTest, SaveCreatesFile) {
    ASSERT_FALSE(std::filesystem::exists(kTestFile));
    TaskStore s;
    s.create("t", "", 0, 0, -1);
    FileIO::save(kTestFile, s);
    EXPECT_TRUE(std::filesystem::exists(kTestFile));
}

TEST_F(FileIOTest, LoadMissingFileThrows) {
    ASSERT_FALSE(std::filesystem::exists(kTestFile));
    TaskStore s;
    EXPECT_THROW(FileIO::load(kTestFile, s), std::runtime_error);
}

TEST_F(FileIOTest, RemoveDeletesFile) {
    TaskStore s;
    FileIO::save(kTestFile, s);
    ASSERT_TRUE(std::filesystem::exists(kTestFile));
    std::remove(kTestFile.c_str());
    EXPECT_FALSE(std::filesystem::exists(kTestFile));
}

TEST_F(FileIOTest, SwapLifecycle) {
    // Simulate: mutation → swap written; quit → swap removed, main untouched.
    TaskStore s;
    s.create("task", "", 2, 0, -1);

    // Swap appears after mutation.
    FileIO::save(kSwapFile, s);
    EXPECT_TRUE(std::filesystem::exists(kSwapFile));
    EXPECT_FALSE(std::filesystem::exists(kTestFile));

    // Clean quit: swap removed, main still absent (Phase 7 writes main).
    std::remove(kSwapFile.c_str());
    EXPECT_FALSE(std::filesystem::exists(kSwapFile));
    EXPECT_FALSE(std::filesystem::exists(kTestFile));
}
