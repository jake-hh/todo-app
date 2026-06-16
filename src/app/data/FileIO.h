#pragma once

#include <string>

class TaskStore;


namespace FileIO {

/**
 * @brief Writes all tasks in @p store to a binary file at @p path.
 * @throws std::runtime_error if the file cannot be opened for writing.
 */
void save(const std::string& path, const TaskStore& store);

/**
 * @brief Loads tasks from a binary file at @p path into @p store.
 * @throws std::runtime_error if the file cannot be opened for reading.
 */
void load(const std::string& path, TaskStore& store);

} // namespace FileIO
