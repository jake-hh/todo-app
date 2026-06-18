#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include "tui/App.h"


static std::string resolveFilePath(int argc, char* argv[]) {
    if (argc > 1) return argv[1];
    std::filesystem::path dir = std::filesystem::path(argv[0]).parent_path();
    if (dir.empty()) return "tasks.bin";
    return (dir / "tasks.bin").string();
}


int main(int argc, char* argv[]) {
    // APP_VERSION is injected by CMake via target_compile_definitions
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-V") == 0 || std::strcmp(argv[i], "--version") == 0) {
            std::printf("todo-app %s\n", APP_VERSION);
            return 0;
        }
    }
    App app(resolveFilePath(argc, argv));
    app.run();
    return 0;
}
