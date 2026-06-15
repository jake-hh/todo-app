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
    App app(resolveFilePath(argc, argv));
    app.run();
    return 0;
}
