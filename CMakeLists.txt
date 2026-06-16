cmake_minimum_required(VERSION 3.20)
project(SmartArrayApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

add_library(SmartArray src/smartArray/SmartArray.cpp src/smartArray/SmartArray.h)
target_include_directories(SmartArray PUBLIC src/smartArray)

add_executable(SmartArrayApp
    src/app/main.cpp
    src/app/data/Task.cpp
    src/app/data/TaskStore.cpp
    src/app/data/FileIO.cpp
    src/app/tui/App.cpp
    src/app/tui/Dialog.cpp
    src/app/tui/ListPane.cpp
    src/app/tui/DetailPane.cpp
    src/app/tui/FieldComponents.cpp
    src/app/tui/DepSelector.cpp
)
target_link_libraries(SmartArrayApp ftxui::component SmartArray)

add_executable(SeedData
    src/app/seedData.cpp
    src/app/data/Task.cpp
    src/app/data/TaskStore.cpp
    src/app/data/FileIO.cpp
)
target_include_directories(SeedData PRIVATE src/app)
target_link_libraries(SeedData SmartArray)

add_executable(SmartArrayDemo src/app/smartArrayDemo.cpp)
target_link_libraries(SmartArrayDemo SmartArray)

add_subdirectory(tests)
