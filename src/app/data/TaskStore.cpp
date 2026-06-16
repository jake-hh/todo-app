#include "TaskStore.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <stack>


unsigned TaskStore::create(const std::string& title,
                           const std::string& description,
                           int priority,
                           int status,
                           int64_t dueDate) {
    Task t;
    t.id          = _nextId++;
    t.title       = title;
    t.description = description;
    t.priority    = priority;
    t.status      = status;
    t.createdAt   = static_cast<int64_t>(std::time(nullptr));
    t.dueDate     = dueDate;
    _tasks.emplace(t.id, std::move(t));
    return t.id;
}


void TaskStore::insert(Task t) {
    _tasks[t.id] = std::move(t);
}


Task& TaskStore::get(unsigned id) {
    auto it = _tasks.find(id);
    if (it == _tasks.end())
        throw std::out_of_range("Task not found");
    return it->second;
}


const Task& TaskStore::get(unsigned id) const {
    auto it = _tasks.find(id);
    if (it == _tasks.end())
        throw std::out_of_range("Task not found");
    return it->second;
}


void TaskStore::update(unsigned id, Task updated) {
    if (_tasks.find(id) == _tasks.end())
        throw std::out_of_range("Task not found");
    updated.id = id;
    _tasks[id] = std::move(updated);
}



void TaskStore::removeCascade(unsigned id) {
    if (_tasks.find(id) == _tasks.end())
        throw std::out_of_range("Task not found");

    // DFS to collect all transitively reachable dep IDs.
    std::set<unsigned> toDelete;
    std::stack<unsigned> stk;
    stk.push(id);
    while (!stk.empty()) {
        unsigned cur = stk.top(); stk.pop();
        if (!toDelete.insert(cur).second) continue;
        auto it = _tasks.find(cur);
        if (it != _tasks.end())
            for (size_t i = 0; i < it->second.deps.size(); i++)
                stk.push(it->second.deps[i]);
    }

    for (unsigned did : toDelete)
        _tasks.erase(did);

    // Remove dangling references to any deleted ID.
    for (auto& [tid, task] : _tasks) {
        SmartArray<unsigned> kept;
        for (size_t i = 0; i < task.deps.size(); i++)
            if (!toDelete.count(task.deps[i]))
                kept.pushBack(task.deps[i]);
        task.deps = std::move(kept);
    }
}


void TaskStore::removeSplice(unsigned id) {
    auto it = _tasks.find(id);
    if (it == _tasks.end())
        throw std::out_of_range("Task not found");

    SmartArray<unsigned> children = it->second.deps; // copy before erase
    _tasks.erase(it);

    // Replace every occurrence of id in deps with id's own children.
    // A seen-set prevents duplicate entries that arise when a child already
    // appears elsewhere in the parent's dep list.
    for (auto& [tid, task] : _tasks) {
        SmartArray<unsigned> newDeps;
        std::set<unsigned> seen;
        for (size_t i = 0; i < task.deps.size(); i++) {
            if (task.deps[i] == id) {
                for (size_t j = 0; j < children.size(); j++)
                    if (seen.insert(children[j]).second)
                        newDeps.pushBack(children[j]);
            } else {
                if (seen.insert(task.deps[i]).second)
                    newDeps.pushBack(task.deps[i]);
            }
        }
        task.deps = std::move(newDeps);
    }
}


const std::map<unsigned, Task>& TaskStore::getTasks() const {
    return _tasks;
}


unsigned TaskStore::size() const {
    return static_cast<unsigned>(_tasks.size());
}


void TaskStore::recalcNextId() {
    if (_tasks.empty())
        _nextId = 0;
    else
        _nextId = _tasks.rbegin()->first + 1;
}


bool TaskStore::hasDep(unsigned taskId, unsigned depId) const {
    auto it = _tasks.find(taskId);
    if (it == _tasks.end()) return false;
    const SmartArray<unsigned>& deps = it->second.deps;
    for (size_t i = 0; i < deps.size(); i++)
        if (deps[i] == depId) return true;
    return false;
}


bool TaskStore::wouldCycle(unsigned taskId, unsigned depId) const {
    // Cycle exists if taskId is reachable from depId via existing deps.
    std::set<unsigned> visited;
    std::stack<unsigned> stk;
    stk.push(depId);
    while (!stk.empty()) {
        unsigned cur = stk.top(); stk.pop();
        if (cur == taskId) return true;
        if (!visited.insert(cur).second) continue;
        auto it = _tasks.find(cur);
        if (it != _tasks.end())
            for (size_t i = 0; i < it->second.deps.size(); i++)
                stk.push(it->second.deps[i]);
    }
    return false;
}


void TaskStore::addDep(unsigned taskId, unsigned depId) {
    if (_tasks.find(taskId) == _tasks.end())
        throw std::out_of_range("Task not found");
    if (_tasks.find(depId) == _tasks.end())
        throw std::out_of_range("Dependency not found");
    if (wouldCycle(taskId, depId))
        throw std::invalid_argument("Dependency would create a cycle");
    Task& t = _tasks[taskId];
    for (size_t i = 0; i < t.deps.size(); i++)
        if (t.deps[i] == depId) return; // already present
    t.deps.pushBack(depId);
}


void TaskStore::removeDep(unsigned taskId, unsigned depId) {
    auto it = _tasks.find(taskId);
    if (it == _tasks.end())
        throw std::out_of_range("Task not found");
    Task& t = it->second;
    SmartArray<unsigned> kept;
    for (size_t i = 0; i < t.deps.size(); i++)
        if (t.deps[i] != depId)
            kept.pushBack(t.deps[i]);
    t.deps = std::move(kept);
}


SmartArray<unsigned> TaskStore::search(const std::string& titleQuery,
                                       int dateFilter,
                                       int priorityFilter,
                                       int statusFilter) const {
    // Pre-compute day boundaries only when a date-range filter is active (1–3).
    // dateFilter values: 0=all, 1=overdue, 2=due today, 3=due this week, 4=no due date.
    int64_t startOfToday = 0, startOfTomorrow = 0, startOfNextWeek = 0;
    if (dateFilter >= 1 && dateFilter <= 3) {
        time_t now = std::time(nullptr);
        tm t = *std::localtime(&now);
        t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;  // midnight = start of today
        startOfToday    = static_cast<int64_t>(std::mktime(&t));
        startOfTomorrow = startOfToday + 24LL * 3600;
        startOfNextWeek = startOfToday + 7LL * 24 * 3600;
    }

    // Lower-case once so per-task comparisons don't repeat the work.
    std::string queryLower = titleQuery;
    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    SmartArray<unsigned> result;
    for (auto& [id, t] : _tasks) {
        // Title substring match (case-insensitive); skip if query is empty.
        if (!queryLower.empty()) {
            std::string titleLower = t.title;
            std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (titleLower.find(queryLower) == std::string::npos) continue;
        }
        if (priorityFilter >= 0 && t.priority != priorityFilter) continue;
        if (statusFilter  >= 0 && t.status   != statusFilter)  continue;
        // dueDate == -1 means no due date; excluded from all date-range filters.
        if (dateFilter == 1 && (t.dueDate == -1 || t.dueDate >= startOfToday))      continue; // overdue: past midnight today
        if (dateFilter == 2 && (t.dueDate == -1 || t.dueDate <  startOfToday
                                                || t.dueDate >= startOfTomorrow))    continue; // due today: [today, tomorrow)
        if (dateFilter == 3 && (t.dueDate == -1 || t.dueDate <  startOfTomorrow
                                                || t.dueDate >= startOfNextWeek))    continue; // due this week: [tomorrow, +7 days)
        if (dateFilter == 4 &&  t.dueDate != -1)                                     continue; // no due date
        result.pushBack(id);
    }
    return result;
}
