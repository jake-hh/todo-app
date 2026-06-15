#include "TaskStore.h"

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


const std::map<unsigned, Task>& TaskStore::tasks() const {
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
