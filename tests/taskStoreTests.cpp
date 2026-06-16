#include <gtest/gtest.h>
#include <ctime>
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


// ── wouldCycle ────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, WouldCycle_NoCycle) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    EXPECT_FALSE(store.wouldCycle(a, b));
}

TEST(TaskStoreTest, WouldCycle_DirectCycle_Detected) {
    // A → B; adding B → A would cycle
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.addDep(a, b);
    EXPECT_TRUE(store.wouldCycle(b, a));
}

TEST(TaskStoreTest, WouldCycle_TransitiveCycle_Detected) {
    // A → B → C; adding C → A would cycle
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.addDep(a, b);
    store.addDep(b, c);
    EXPECT_TRUE(store.wouldCycle(c, a));
}

TEST(TaskStoreTest, WouldCycle_SelfLoop_Detected) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    EXPECT_TRUE(store.wouldCycle(a, a));
}


// ── addDep ────────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, AddDep_ValidDep_UpdatesTask) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.addDep(a, b);
    EXPECT_EQ(store.get(a).deps.size(), 1u);
    EXPECT_EQ(store.get(a).deps[0], b);
}

TEST(TaskStoreTest, AddDep_DirectCycle_Throws) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.addDep(a, b);
    EXPECT_THROW(store.addDep(b, a), std::invalid_argument);
}

TEST(TaskStoreTest, AddDep_TransitiveCycle_Throws) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    unsigned c = makeTask(store, "C");
    store.addDep(a, b);
    store.addDep(b, c);
    EXPECT_THROW(store.addDep(c, a), std::invalid_argument);
}

TEST(TaskStoreTest, AddDep_Duplicate_Ignored) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.addDep(a, b);
    store.addDep(a, b); // duplicate
    EXPECT_EQ(store.get(a).deps.size(), 1u);
}


// ── removeDep ─────────────────────────────────────────────────────────────────

TEST(TaskStoreTest, RemoveDep_RemovesFromDeps) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.addDep(a, b);
    store.removeDep(a, b);
    EXPECT_EQ(store.get(a).deps.size(), 0u);
}

TEST(TaskStoreTest, RemoveDep_NonexistentDep_NoOp) {
    TaskStore store;
    unsigned a = makeTask(store, "A");
    unsigned b = makeTask(store, "B");
    store.removeDep(a, b); // b was never a dep — should not throw
    EXPECT_EQ(store.get(a).deps.size(), 0u);
}

TEST(TaskStoreTest, RemoveDep_UnknownTask_Throws) {
    TaskStore store;
    EXPECT_THROW(store.removeDep(999, 1), std::out_of_range);
}


// ── search — title ────────────────────────────────────────────────────────────

TEST(TaskStoreTest, Search_EmptyQuery_ReturnsAll) {
    TaskStore store;
    store.create("alpha", "", 0, 0, -1);
    store.create("beta",  "", 0, 0, -1);
    auto r = store.search("", 0, -1, -1);
    EXPECT_EQ(r.size(), 2u);
}

TEST(TaskStoreTest, Search_TitleSubstring_MatchesOnly) {
    TaskStore store;
    store.create("fix login bug", "", 0, 0, -1);
    store.create("add tests",     "", 0, 0, -1);
    store.create("fix crash",     "", 0, 0, -1);
    auto r = store.search("fix", 0, -1, -1);
    EXPECT_EQ(r.size(), 2u);
}

TEST(TaskStoreTest, Search_TitleCaseInsensitive) {
    TaskStore store;
    store.create("Fix Login", "", 0, 0, -1);
    store.create("add tests",  "", 0, 0, -1);
    auto r = store.search("fix", 0, -1, -1);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_TitleNoMatch_ReturnsEmpty) {
    TaskStore store;
    store.create("alpha", "", 0, 0, -1);
    auto r = store.search("zzz", 0, -1, -1);
    EXPECT_EQ(r.size(), 0u);
}

TEST(TaskStoreTest, Search_EmptyStore_ReturnsEmpty) {
    TaskStore store;
    auto r = store.search("", 0, -1, -1);
    EXPECT_EQ(r.size(), 0u);
}


// ── search — priority filter ──────────────────────────────────────────────────

TEST(TaskStoreTest, Search_PriorityFilter_ReturnsMatchingOnly) {
    TaskStore store;
    store.create("low task",  "", 1, 0, -1);
    store.create("high task", "", 3, 0, -1);
    store.create("high task2","", 3, 0, -1);
    auto r = store.search("", 0, 3, -1);
    EXPECT_EQ(r.size(), 2u);
}

TEST(TaskStoreTest, Search_PriorityAllFilter_ReturnsAll) {
    TaskStore store;
    store.create("a", "", 0, 0, -1);
    store.create("b", "", 2, 0, -1);
    auto r = store.search("", 0, -1, -1);
    EXPECT_EQ(r.size(), 2u);
}

TEST(TaskStoreTest, Search_PriorityFilter_NoMatch_ReturnsEmpty) {
    TaskStore store;
    store.create("a", "", 0, 0, -1); // priority 0
    auto r = store.search("", 0, 3, -1); // looking for priority 3
    EXPECT_EQ(r.size(), 0u);
}


// ── search — status filter ────────────────────────────────────────────────────

TEST(TaskStoreTest, Search_StatusFilter_ReturnsMatchingOnly) {
    TaskStore store;
    store.create("open task",     "", 0, 0, -1); // status 0 = open
    store.create("done task",     "", 0, 2, -1); // status 2 = done
    store.create("wontfix task",  "", 0, 3, -1); // status 3 = wontfix
    auto r = store.search("", 0, -1, 0);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_StatusAllFilter_ReturnsAll) {
    TaskStore store;
    store.create("a", "", 0, 0, -1);
    store.create("b", "", 0, 1, -1);
    store.create("c", "", 0, 2, -1);
    auto r = store.search("", 0, -1, -1);
    EXPECT_EQ(r.size(), 3u);
}


// ── search — date filter ──────────────────────────────────────────────────────

// Compute start-of-today timestamp (mirrors the implementation)
static int64_t startOfToday() {
    time_t now = std::time(nullptr);
    tm t = *std::localtime(&now);
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    return static_cast<int64_t>(std::mktime(&t));
}

TEST(TaskStoreTest, Search_DateFilter_NoDate_ReturnsNoDueDateTasks) {
    TaskStore store;
    store.create("no date",  "", 0, 0, -1);
    store.create("has date", "", 0, 0, startOfToday() - 86400); // yesterday
    auto r = store.search("", 4, -1, -1); // dateFilter=4: no due date
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_DateFilter_Overdue_ReturnsOverdueTasks) {
    TaskStore store;
    int64_t yesterday = startOfToday() - 86400;
    int64_t tomorrow  = startOfToday() + 86400;
    store.create("overdue",  "", 0, 0, yesterday);
    store.create("future",   "", 0, 0, tomorrow);
    store.create("no date",  "", 0, 0, -1);
    auto r = store.search("", 1, -1, -1); // dateFilter=1: overdue
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_DateFilter_DueToday_ReturnsTodayTasks) {
    TaskStore store;
    int64_t midday    = startOfToday() + 12 * 3600; // noon today
    int64_t yesterday = startOfToday() - 86400;
    int64_t nextWeek  = startOfToday() + 8 * 86400;
    store.create("today",     "", 0, 0, midday);
    store.create("yesterday", "", 0, 0, yesterday);
    store.create("next week", "", 0, 0, nextWeek);
    store.create("no date",   "", 0, 0, -1);
    auto r = store.search("", 2, -1, -1); // dateFilter=2: due today
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_DateFilter_DueThisWeek_ReturnsTasksDueWithinWeek) {
    TaskStore store;
    int64_t today     = startOfToday() + 12 * 3600; // noon today
    int64_t tomorrow  = startOfToday() + 86400;
    int64_t in3Days   = startOfToday() + 3 * 86400;
    int64_t in8Days   = startOfToday() + 8 * 86400;
    int64_t yesterday = startOfToday() - 86400;
    store.create("today",     "", 0, 0, today);     // excluded: today belongs to "due today"
    store.create("tomorrow",  "", 0, 0, tomorrow);  // included: first day of "this week"
    store.create("in 3 days", "", 0, 0, in3Days);  // included
    store.create("in 8 days", "", 0, 0, in8Days);  // excluded: outside 7-day window
    store.create("yesterday", "", 0, 0, yesterday); // excluded: overdue
    store.create("no date",   "", 0, 0, -1);        // excluded: no due date
    // dateFilter=3: due this week means [tomorrow, today+7d)
    auto r = store.search("", 3, -1, -1);
    EXPECT_EQ(r.size(), 2u);
}

TEST(TaskStoreTest, Search_DateFilter_DueThisWeek_ExcludesToday) {
    TaskStore store;
    int64_t noon = startOfToday() + 12 * 3600;
    store.create("today", "", 0, 0, noon);
    auto r = store.search("", 3, -1, -1); // "this week" must not include today
    EXPECT_EQ(r.size(), 0u);
}

TEST(TaskStoreTest, Search_DateFilter_DueThisWeek_IncludesTomorrow) {
    TaskStore store;
    int64_t tomorrow = startOfToday() + 86400;
    store.create("tomorrow", "", 0, 0, tomorrow);
    auto r = store.search("", 3, -1, -1);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_DateAllFilter_IncludesAllDueDates) {
    TaskStore store;
    store.create("no date",  "", 0, 0, -1);
    store.create("past",     "", 0, 0, startOfToday() - 86400);
    store.create("future",   "", 0, 0, startOfToday() + 86400);
    auto r = store.search("", 0, -1, -1); // dateFilter=0: all
    EXPECT_EQ(r.size(), 3u);
}


// ── search — multi-param ──────────────────────────────────────────────────────

TEST(TaskStoreTest, Search_TitleAndPriority_Intersection) {
    TaskStore store;
    store.create("fix bug high",  "", 3, 0, -1);
    store.create("fix bug low",   "", 1, 0, -1);
    store.create("add feature",   "", 3, 0, -1);
    // "fix" AND priority=3 → only "fix bug high"
    auto r = store.search("fix", 0, 3, -1);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_TitleAndStatus_Intersection) {
    TaskStore store;
    store.create("login open",    "", 0, 0, -1); // status=0 open
    store.create("login done",    "", 0, 2, -1); // status=2 done
    store.create("signup open",   "", 0, 0, -1);
    // "login" AND status=0 → only "login open"
    auto r = store.search("login", 0, -1, 0);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_PriorityAndStatus_Intersection) {
    TaskStore store;
    store.create("a", "", 3, 0, -1); // high+open
    store.create("b", "", 3, 1, -1); // high+in-progress
    store.create("c", "", 1, 0, -1); // low+open
    // priority=3 AND status=0 → only "a"
    auto r = store.search("", 0, 3, 0);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_TitlePriorityStatus_AllThree) {
    TaskStore store;
    store.create("deploy prod",  "", 3, 1, -1); // high, in-progress
    store.create("deploy dev",   "", 3, 0, -1); // high, open
    store.create("deploy stage", "", 1, 1, -1); // low,  in-progress
    store.create("review PR",    "", 3, 1, -1); // high, in-progress (no "deploy")
    // "deploy" AND priority=3 AND status=1 → only "deploy prod"
    auto r = store.search("deploy", 0, 3, 1);
    EXPECT_EQ(r.size(), 1u);
}

TEST(TaskStoreTest, Search_MultiParam_NoMatch_ReturnsEmpty) {
    TaskStore store;
    store.create("alpha", "", 0, 0, -1);
    // title matches but priority doesn't
    auto r = store.search("alpha", 0, 3, -1);
    EXPECT_EQ(r.size(), 0u);
}

TEST(TaskStoreTest, Search_ResultsInIdOrder) {
    TaskStore store;
    unsigned a = store.create("fix a", "", 0, 0, -1);
    unsigned b = store.create("fix b", "", 0, 0, -1);
    unsigned c = store.create("fix c", "", 0, 0, -1);
    auto r = store.search("fix", 0, -1, -1);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], a);
    EXPECT_EQ(r[1], b);
    EXPECT_EQ(r[2], c);
}
