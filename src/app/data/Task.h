#pragma once

#include <optional>
#include <string>
#include <cstdint>
#include "SmartArray.h"

/**
 * @brief Represents a single task in the todo manager.
 */
struct Task {
    unsigned id;          ///< Unique auto-assigned ID
    std::string title;
    std::string description;
    int priority;         ///< 0=wishlist, 1=low, 2=medium, 3=high
    int status;           ///< 0=open, 1=in-progress, 2=done, 3=wontfix
    int64_t createdAt;    ///< Unix epoch seconds
    int64_t dueDate;      ///< Unix epoch seconds, or -1 if none
    SmartArray<unsigned> deps; ///< Depencies / subtasks (IDs of tasks this task is blocked by)

    /** @brief Returns a name for the status value. */
    std::string statusStr() const;

    /** @brief Returns a unicode symbol for the status value. */
    std::string statusSymbol() const;

    /** @brief Returns a name for the priority value. */
    std::string priorityStr() const;

    /** @brief Returns the due date formatted as DD/MM/YY, or "none" if unset. */
    std::string dueDateStr() const;

    /** @brief Returns the creation date formatted as DD/MM/YY, or "none" if unset. */
    std::string createdAtStr() const;
};

/**
 * @brief Parses a due date string in DD/MM/YY format.
 *
 * Returns -1 for empty string (no due date), the Unix epoch seconds for a
 * valid date, or nullopt for invalid or impossible dates (e.g. "31/02/26").
 */
std::optional<int64_t> parseDueDate(const std::string& s);
