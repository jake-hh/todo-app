#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include "tui/App.h"


// Default: ~/.local/share/todo-app/tasks.bin. Override with argv[1].
static std::string resolveFilePath(int argc, char* argv[]) {
    if (argc > 1) return argv[1];
    const char* home = std::getenv("HOME");
    std::filesystem::path dir = home
        ? std::filesystem::path(home) / ".local" / "share" / "todo-app"
        : std::filesystem::current_path();
    std::filesystem::create_directories(dir);
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
