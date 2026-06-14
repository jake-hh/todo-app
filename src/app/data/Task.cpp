#include "Task.h"

#include <ctime>
#include <stdexcept>


// file-local bc static
static std::string formatDate(int64_t epoch) {
    if (epoch == -1)
        return "none";

    time_t t = static_cast<time_t>(epoch);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%d/%m/%y", std::localtime(&t));
    return buf;
}


std::string Task::statusStr() const {
    switch (status) {
        case 0:  return "open";
        case 1:  return "in-progress";
        case 2:  return "done";
        case 3:  return "wontfix";
        default: return "?";
    }
}


std::string Task::statusSymbol() const {
    switch (status) {
        case 0:  return "☐";
        case 1:  return "🌟";  // ✯
        case 2:  return "✔";
        case 3:  return "✗";
        default: return "?";
    }
}


std::string Task::priorityStr() const {
    switch (priority) {
        case 0:  return "wishlist";
        case 1:  return "low";
        case 2:  return "medium";
        case 3:  return "high";
        default: return "?";
    }
}


std::string Task::dueDateStr() const {
    return formatDate(dueDate);
}


std::string Task::createdAtStr() const {
    return formatDate(createdAt);
}


std::optional<int64_t> parseDueDate(const std::string& s) {
    if (s.empty()) return -1;

    size_t first = s.find('/');
    if (first == std::string::npos) return std::nullopt;
    size_t second = s.find('/', first + 1);
    if (second == std::string::npos) return std::nullopt;
    if (s.find('/', second + 1) != std::string::npos) return std::nullopt;

    std::string dayStr  = s.substr(0, first);
    std::string monStr  = s.substr(first + 1, second - first - 1);
    std::string yearStr = s.substr(second + 1);

    if (dayStr.empty() || monStr.empty() || yearStr.empty()) return std::nullopt;

    int day, mon, year;
    try {
        size_t pos;
        day  = std::stoi(dayStr,  &pos); if (pos != dayStr.size())  return std::nullopt;
        mon  = std::stoi(monStr,  &pos); if (pos != monStr.size())  return std::nullopt;
        year = std::stoi(yearStr, &pos); if (pos != yearStr.size()) return std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }

    if (day < 1 || day > 31) return std::nullopt;
    if (mon < 1 || mon > 12) return std::nullopt;
    if (year < 0 || year > 99) return std::nullopt;

    std::tm t  = {};
    t.tm_mday  = day;
    t.tm_mon   = mon - 1;    // 0-indexed
    t.tm_year  = year + 100; // years since 1900 (YY 00-99 → 2000-2099)
    t.tm_isdst = -1;

    std::tm orig = t;
    time_t epoch = std::mktime(&t);
    if (epoch == static_cast<time_t>(-1)) return std::nullopt;

    // mktime normalises impossible dates (e.g. Feb 31 → Mar 3); detect that.
    if (t.tm_mday != orig.tm_mday || t.tm_mon != orig.tm_mon || t.tm_year != orig.tm_year)
        return std::nullopt;

    return static_cast<int64_t>(epoch);
}
