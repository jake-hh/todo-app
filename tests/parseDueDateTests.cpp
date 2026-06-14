#include <gtest/gtest.h>
#include <ctime>
#include "Task.h"


TEST(ParseDueDate, EmptyStringReturnsMinusOne) {
    auto result = parseDueDate("");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, -1);
}

TEST(ParseDueDate, ValidDateReturnsEpoch) {
    // 13/06/26 = June 13 2026, midnight local time
    std::tm t  = {};
    t.tm_mday  = 13;
    t.tm_mon   = 5;   // June
    t.tm_year  = 126; // 2026
    t.tm_isdst = -1;
    int64_t expected = static_cast<int64_t>(std::mktime(&t));

    auto result = parseDueDate("13/06/26");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected);
}

TEST(ParseDueDate, GarbageInputReturnsNullopt) {
    EXPECT_FALSE(parseDueDate("not-a-date").has_value());
    EXPECT_FALSE(parseDueDate("abc").has_value());
    EXPECT_FALSE(parseDueDate("//").has_value());
    EXPECT_FALSE(parseDueDate("1/2").has_value());
    EXPECT_FALSE(parseDueDate("1/2/3/4").has_value());
}

TEST(ParseDueDate, ImpossibleDateReturnsNullopt) {
    EXPECT_FALSE(parseDueDate("31/02/26").has_value());
    EXPECT_FALSE(parseDueDate("00/06/26").has_value());
    EXPECT_FALSE(parseDueDate("01/13/26").has_value());
    EXPECT_FALSE(parseDueDate("32/01/26").has_value());
}

TEST(ParseDueDate, BoundaryYears) {
    // YY=00 should map to 2000, not 1900
    std::tm t = {};
    t.tm_mday  = 1;
    t.tm_mon   = 0;   // January
    t.tm_year  = 100; // 2000
    t.tm_isdst = -1;
    int64_t expected2000 = static_cast<int64_t>(std::mktime(&t));

    auto result = parseDueDate("01/01/00");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected2000);

    // YY=99 should map to 2099
    t          = {};
    t.tm_mday  = 31;
    t.tm_mon   = 11;  // December
    t.tm_year  = 199; // 2099
    t.tm_isdst = -1;
    int64_t expected2099 = static_cast<int64_t>(std::mktime(&t));

    result = parseDueDate("31/12/99");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected2099);
}


TEST(ParseDueDate, RoundTrip) {
    // Format a known epoch with dueDateStr(), parse it back, check same day.
    std::tm t  = {};
    t.tm_mday  = 7;
    t.tm_mon   = 3;   // April
    t.tm_year  = 125; // 2025
    t.tm_isdst = -1;
    int64_t epoch = static_cast<int64_t>(std::mktime(&t));

    Task task{};
    task.dueDate = epoch;
    std::string formatted = task.dueDateStr(); // "07/04/25"

    auto result = parseDueDate(formatted);
    ASSERT_TRUE(result.has_value());

    // Both epochs represent the same calendar day (may differ by DST seconds at most).
    time_t a = static_cast<time_t>(epoch);
    time_t b = static_cast<time_t>(*result);
    std::tm tmA = *std::localtime(&a);
    std::tm tmB = *std::localtime(&b);
    EXPECT_EQ(tmA.tm_year,  tmB.tm_year);
    EXPECT_EQ(tmA.tm_mon,   tmB.tm_mon);
    EXPECT_EQ(tmA.tm_mday,  tmB.tm_mday);
}
