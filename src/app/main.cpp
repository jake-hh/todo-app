#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include "tui/App.h"


static std::filesystem::path baseDir() {
#ifdef _WIN32
    const char* base = std::getenv("LOCALAPPDATA");
    return base ? std::filesystem::path(base) / "todo-app"
                : std::filesystem::current_path();
#else
    const char* home = std::getenv("HOME");
    return home ? std::filesystem::path(home) / ".local" / "share" / "todo-app"
                : std::filesystem::current_path();
#endif
}


/*
    Resolve the save file path from argv[1]:
        (none)         -> <baseDir>/tasks.bin
        name           -> <baseDir>/name.bin
        name.bin       -> <baseDir>/name.bin
        /abs/path      -> /abs/path.bin    (no extension -> appends .bin)
        ~/homepath     -> ~/homepath.bin   (no extension -> appends .bin)
        ./local/path   -> ./local/path.bin (no extension -> appends .bin)
    Non-.bin extensions are rejected with an error.
*/
static std::string resolveFilePath(int argc, char* argv[]) {
    std::filesystem::path p;

    if (argc > 1) {
        p = std::filesystem::path(argv[1]);
        bool isLiteral = p.has_parent_path();

        if (p.has_extension() && p.extension() != ".bin") {
            std::cerr << "error: only .bin files are supported\n";
            std::exit(1);
        }

        if (!p.has_extension()) p += ".bin";

        if (!isLiteral) {
            std::filesystem::path dir = baseDir();
            std::filesystem::create_directories(dir);
            return (dir / p).string();
        }
    } else {
        std::filesystem::path dir = baseDir();
        std::filesystem::create_directories(dir);
        return (dir / "tasks.bin").string();
    }

    return p.string();
}


int main(int argc, char* argv[]) {
    // show version & quit if version flag is pressent
    // APP_VERSION is injected by CMake via target_compile_definitions
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-V") == 0 || std::strcmp(argv[i], "--version") == 0) {
            std::printf("todo-app %s\n", APP_VERSION);
            return 0;
        }
    }

    // launch the TUI App, pass the binary file path
    App app(resolveFilePath(argc, argv));
    app.run();
    return 0;
}
