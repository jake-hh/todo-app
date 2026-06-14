cmake_minimum_required(VERSION 3.20)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(CTest)
enable_testing()

add_executable(smartArrayTests smartArrayTests.cpp)
target_link_libraries(smartArrayTests GTest::gtest_main SmartArray)

add_executable(taskStoreTests taskStoreTests.cpp ../src/app/data/TaskStore.cpp)
target_include_directories(taskStoreTests PRIVATE ../src/smartArray)
target_link_libraries(taskStoreTests GTest::gtest_main SmartArray)

add_executable(fileIOTests
    fileIOTests.cpp
    ../src/app/data/FileIO.cpp
    ../src/app/data/TaskStore.cpp
    ../src/app/data/Task.cpp
)
target_include_directories(fileIOTests PRIVATE ../src/smartArray ../src/app/data)
target_link_libraries(fileIOTests GTest::gtest_main SmartArray)

add_executable(parseDueDateTests
    parseDueDateTests.cpp
    ../src/app/data/Task.cpp
)
target_include_directories(parseDueDateTests PRIVATE ../src/smartArray ../src/app/data)
target_link_libraries(parseDueDateTests GTest::gtest_main SmartArray)

include(GoogleTest)
gtest_discover_tests(smartArrayTests)
gtest_discover_tests(taskStoreTests)
gtest_discover_tests(fileIOTests)
gtest_discover_tests(parseDueDateTests)
