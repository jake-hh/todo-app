#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include "tui/App.h"

using namespace std;


static filesystem::path baseDir() {
#ifdef _WIN32
    const char* base = getenv("LOCALAPPDATA");
    return base ? filesystem::path(base) / "todo-app"
                : filesystem::current_path();
#else
    const char* home = getenv("HOME");
    return home ? filesystem::path(home) / ".local" / "share" / "todo-app"
                : filesystem::current_path();
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
static string resolveFilePath(int argc, char* argv[]) {
    filesystem::path p;

    if (argc > 1) {
        p = filesystem::path(argv[1]);
        bool isLiteral = p.has_parent_path();

        if (p.has_extension() && p.extension() != ".bin") {
            cerr << "error: only .bin files are supported\n";
            exit(1);
        }

        if (!p.has_extension()) p += ".bin";

        if (!isLiteral) {
            filesystem::path dir = baseDir();
            filesystem::create_directories(dir);
            return (dir / p).string();
        }
    } else {
        filesystem::path dir = baseDir();
        filesystem::create_directories(dir);
        return (dir / "tasks.bin").string();
    }

    return p.string();
}


int main(int argc, char* argv[]) {
    // show version & quit if version flag is pressent
    // APP_VERSION is injected by CMake via target_compile_definitions
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            cout << "todo-app " << APP_VERSION << endl;
            return 0;
        }
    }

    // launch the TUI App, pass the binary file path
    App app(resolveFilePath(argc, argv));
    app.run();
    return 0;
}
