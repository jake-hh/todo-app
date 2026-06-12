#include "TaskStore.h"


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


void TaskStore::remove(unsigned id) {
    if (_tasks.erase(id) == 0)
        throw std::out_of_range("Task not found");
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
