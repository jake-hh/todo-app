#include <gtest/gtest.h>
#include "../src/app/data/TaskStore.h"

// Helper: create a minimal task in the store
static unsigned makeTask(TaskStore& store, const std::string& title = "task") {
    return store.create(title, "", 0, 0, -1);
}

// ── create ────────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, CreateAssignsSequentialIds) {
    TaskStore store;
    unsigned a = makeTask(store, "a");
    unsigned b = makeTask(store, "b");
    unsigned c = makeTask(store, "c");
    EXPECT_EQ(a, 0u);
    EXPECT_EQ(b, 1u);
    EXPECT_EQ(c, 2u);
}

TEST(TaskStoreTest, CreateStoresAllFields) {
    TaskStore store;
    unsigned id = store.create("title", "desc", 2, 1, 9999);
    const Task& t = store.get(id);
    EXPECT_EQ(t.title, "title");
    EXPECT_EQ(t.description, "desc");
    EXPECT_EQ(t.priority, 2);
    EXPECT_EQ(t.status, 1);
    EXPECT_EQ(t.dueDate, 9999);
    EXPECT_GT(t.createdAt, 0);
}

TEST(TaskStoreTest, SizeIncreasesOnCreate) {
    TaskStore store;
    EXPECT_EQ(store.size(), 0u);
    makeTask(store);
    EXPECT_EQ(store.size(), 1u);
    makeTask(store);
    EXPECT_EQ(store.size(), 2u);
}

// ── get ───────────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, GetReturnsCorrectTask) {
    TaskStore store;
    unsigned id = makeTask(store, "hello");
    EXPECT_EQ(store.get(id).title, "hello");
}

TEST(TaskStoreTest, GetThrowsForMissingId) {
    TaskStore store;
    EXPECT_THROW(store.get(99), std::out_of_range);
}

TEST(TaskStoreTest, GetConstOverload) {
    TaskStore store;
    unsigned id = makeTask(store, "const");
    const TaskStore& cstore = store;
    EXPECT_EQ(cstore.get(id).title, "const");
}

// ── update ────────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, UpdateModifiesFields) {
    TaskStore store;
    unsigned id = makeTask(store, "old");
    Task t = store.get(id);
    t.title = "new";
    t.priority = 3;
    store.update(id, t);
    EXPECT_EQ(store.get(id).title, "new");
    EXPECT_EQ(store.get(id).priority, 3);
}

TEST(TaskStoreTest, UpdatePreservesId) {
    TaskStore store;
    unsigned id = makeTask(store);
    Task t = store.get(id);
    t.id = 999; // should be ignored
    store.update(id, t);
    EXPECT_EQ(store.get(id).id, id);
}

TEST(TaskStoreTest, UpdateThrowsForMissingId) {
    TaskStore store;
    Task t{};
    EXPECT_THROW(store.update(99, t), std::out_of_range);
}

// ── remove ────────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, RemoveDeletesTask) {
    TaskStore store;
    unsigned id = makeTask(store);
    store.removeSplice(id);
    EXPECT_EQ(store.size(), 0u);
    EXPECT_THROW(store.get(id), std::out_of_range);
}

TEST(TaskStoreTest, RemoveThrowsForMissingId) {
    TaskStore store;
    EXPECT_THROW(store.removeSplice(99), std::out_of_range);
}

// ── removeCascade ─────────────────────────────────────────────────────────────

TEST(TaskStoreTest, RemoveCascadeDeletesTargetAndTransitiveDeps) {
    // A → B → C  (A is blocked by B, B is blocked by C)
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.get(a).deps.pushBack(b);
    store.get(b).deps.pushBack(c);

    store.removeCascade(a);

    EXPECT_EQ(store.size(), 0u);
}

TEST(TaskStoreTest, RemoveCascadeCleansDanglingRefs) {
    // X → B → C  and  Y → B (Y also depends on B which gets cascade-deleted via X)
    TaskStore store;
    unsigned x = makeTask(store, "X");
    unsigned y = makeTask(store, "Y");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.get(x).deps.pushBack(b);
    store.get(y).deps.pushBack(b);
    store.get(b).deps.pushBack(c);

    store.removeCascade(x); // deletes x, b, c

    EXPECT_EQ(store.size(), 1u);
    EXPECT_EQ(store.get(y).deps.size(), 0u); // dangling ref to b removed
}

TEST(TaskStoreTest, RemoveCascadeThrowsForMissingId) {
    TaskStore store;
    EXPECT_THROW(store.removeCascade(99), std::out_of_range);
}

// ── removeSplice ──────────────────────────────────────────────────────────────

TEST(TaskStoreTest, RemoveSpliceWiresParentToGrandchildren) {
    // A → B → C  splice B: A should now depend on C
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.get(a).deps.pushBack(b);
    store.get(b).deps.pushBack(c);

    store.removeSplice(b);

    EXPECT_EQ(store.size(), 2u);
    EXPECT_THROW(store.get(b), std::out_of_range);
    EXPECT_EQ(store.get(a).deps.size(), 1u);
    EXPECT_EQ(store.get(a).deps[0], c);
}

TEST(TaskStoreTest, RemoveSpliceNoDuplicateDepsWhenChildAlreadyPresent) {
    // A depends on [B, C]; B depends on [C]. Splicing B: A should have [C] once.
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.get(a).deps.pushBack(b);
    store.get(a).deps.pushBack(c);
    store.get(b).deps.pushBack(c);

    store.removeSplice(b);

    EXPECT_EQ(store.get(a).deps.size(), 1u);
    EXPECT_EQ(store.get(a).deps[0], c);
}

TEST(TaskStoreTest, RemoveSpliceLeafLeavesParentWithNoDeps) {
    // A → B (leaf). Splicing B: A should have no deps.
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.get(a).deps.pushBack(b);

    store.removeSplice(b);

    EXPECT_EQ(store.size(), 1u);
    EXPECT_EQ(store.get(a).deps.size(), 0u);
}

TEST(TaskStoreTest, RemoveSpliceMultipleParentsAllRewired) {
    // A → B → C  and  Y → B.  Splicing B: both A and Y should depend on C.
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned y = makeTask(store, "Y");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.get(a).deps.pushBack(b);
    store.get(y).deps.pushBack(b);
    store.get(b).deps.pushBack(c);

    store.removeSplice(b);

    EXPECT_EQ(store.size(), 3u);
    EXPECT_EQ(store.get(a).deps.size(), 1u);
    EXPECT_EQ(store.get(a).deps[0], c);
    EXPECT_EQ(store.get(y).deps.size(), 1u);
    EXPECT_EQ(store.get(y).deps[0], c);
}


// ── nextId ────────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, NextIdAlwaysExceedsAllIds) {
    TaskStore store;
    unsigned a = makeTask(store);
    unsigned b = makeTask(store);
    unsigned c = makeTask(store);
    store.removeSplice(b);
    // next id should be > max remaining id
    unsigned next = makeTask(store);
    EXPECT_GT(next, a);
    EXPECT_GT(next, c);
}

TEST(TaskStoreTest, RecalcNextIdAfterLoad) {
    TaskStore store;
    makeTask(store); // id 0
    makeTask(store); // id 1
    makeTask(store); // id 2
    store.recalcNextId();
    unsigned next = makeTask(store);
    EXPECT_EQ(next, 3u);
}

TEST(TaskStoreTest, RecalcNextIdOnEmptyStore) {
    TaskStore store;
    store.recalcNextId();
    unsigned next = makeTask(store);
    EXPECT_EQ(next, 0u);
}
