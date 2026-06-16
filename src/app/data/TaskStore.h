#pragma once

#include <map>
#include <stdexcept>
#include <ctime>
#include "Task.h"


/**
 * @brief Owns and manages the collection of tasks.
 *
 * Primary store is a std::map for O(log n) ID-based access and
 * deterministic iteration in ID order.
 */
class TaskStore {
private:
    std::map<unsigned, Task> _tasks; // primary store; iterates in ID order
    unsigned _nextId = 0;            // next ID to assign; not persisted, re-derived on load

public:
    /**
     * @brief Creates a new task and assigns it a unique ID.
     * @param title       Task title.
     * @param description Task description.
     * @param priority    0–3 (wishlist/low/medium/high).
     * @param status      0–3 (open/in-progress/done/wontfix).
     * @param dueDate     Unix epoch seconds, or -1 for none.
     * @return The ID assigned to the new task.
     */
    unsigned create(const std::string& title,
                    const std::string& description,
                    int priority,
                    int status,
                    int64_t dueDate);

    /**
     * @brief Inserts a task at its existing id. Used by FileIO::load.
     * (!) You have to call recalcNextId after inserting tasks (!)
     */
    void insert(Task t);

    /**
     * @brief Returns a reference to the task with the given ID.
     * @throws std::out_of_range if the ID does not exist.
     */
    Task& get(unsigned id);

    /** @brief Const overload of get(). */
    const Task& get(unsigned id) const;

    /**
     * @brief Replaces the task at the given ID with the provided task.
     * @throws std::out_of_range if the ID does not exist.
     */
    void update(unsigned id, Task updated);

    /**
     * @brief Deletes @p id and all tasks transitively reachable via its deps,
     *        then removes any dangling references to deleted IDs from other tasks.
     * @throws std::out_of_range if the ID does not exist.
     */
    void removeCascade(unsigned id);

    /**
     * @brief Deletes @p id and splices its deps into every task that depended on it.
     *        Tasks that had @p id as a dep now depend directly on @p id's deps.
     * @throws std::out_of_range if the ID does not exist.
     */
    void removeSplice(unsigned id);

    /**
     * @brief Returns a const reference to the underlying map for iteration.
     */
    const std::map<unsigned, Task>& getTasks() const;

    /**
     * @brief Returns the number of tasks in the store.
     */
    unsigned size() const;

    /**
     * @brief Re-derives nextId as max(existing id) + 1.
     * Call after loading tasks from disk.
     */
    void recalcNextId();

    /**
     * @brief Returns true if @p depId is already a direct dependency of @p taskId.
     * Returns false if @p taskId does not exist.
     */
    bool hasDep(unsigned taskId, unsigned depId) const;

    /**
     * @brief Returns true if adding @p depId as a dependency of @p taskId would
     *        create a cycle in the dependency graph.
     */
    bool wouldCycle(unsigned taskId, unsigned depId) const;

    /**
     * @brief Adds @p depId as a dependency of @p taskId.
     * @throws std::out_of_range if either ID does not exist.
     * @throws std::invalid_argument if the dep would create a cycle.
     * No-op if the dependency already exists.
     */
    void addDep(unsigned taskId, unsigned depId);

    /**
     * @brief Removes @p depId from the dependency list of @p taskId.
     * @throws std::out_of_range if @p taskId does not exist.
     * No-op if @p depId is not in the dependency list.
     */
    void removeDep(unsigned taskId, unsigned depId);

    /**
     * @brief Returns IDs of all tasks matching the given filters, in ID order.
     *
     * @param titleQuery    Case-insensitive substring to match against title. Empty = match all.
     * @param dateFilter    0=all, 1=overdue, 2=due-today, 3=due-this-week, 4=no-date.
     * @param priorityFilter -1=all, 0–3=exact match.
     * @param statusFilter   -1=all, 0–3=exact match.
     */
    SmartArray<unsigned> search(const std::string& titleQuery,
                                int dateFilter,
                                int priorityFilter,
                                int statusFilter) const;
};
