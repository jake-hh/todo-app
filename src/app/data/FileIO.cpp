#include "FileIO.h"

#include <fstream>
#include <stdexcept>
#include "TaskStore.h"


namespace {

void writeU32(std::ofstream& f, uint32_t v) {
    f.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void writeI32(std::ofstream& f, int32_t v) {
    f.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void writeI64(std::ofstream& f, int64_t v) {
    f.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

void writeStr(std::ofstream& f, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    writeU32(f, len);
    f.write(s.data(), len);
}

uint32_t readU32(std::ifstream& f) {
    uint32_t v;
    f.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

int32_t readI32(std::ifstream& f) {
    int32_t v;
    f.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

int64_t readI64(std::ifstream& f) {
    int64_t v;
    f.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

std::string readStr(std::ifstream& f) {
    uint32_t len = readU32(f);
    std::string s(len, '\0');
    f.read(s.data(), len);
    return s;
}

} // namespace


namespace FileIO {

void save(const std::string& path, const TaskStore& store) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("FileIO::save: cannot open " + path);

    const auto& tasks = store.getTasks();
    writeU32(f, static_cast<uint32_t>(tasks.size()));

    for (const auto& [id, t] : tasks) {
        writeU32(f, static_cast<uint32_t>(t.id));
        writeStr(f, t.title);
        writeStr(f, t.description);
        writeI32(f, t.priority);
        writeI32(f, t.status);
        writeI64(f, t.createdAt);
        writeI64(f, t.dueDate);

        writeU32(f, static_cast<uint32_t>(t.deps.size()));
        for (unsigned i = 0; i < t.deps.size(); i++) {
            writeU32(f, static_cast<uint32_t>(t.deps[i]));
        }
    }
}


void load(const std::string& path, TaskStore& store) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("FileIO::load: cannot open " + path);

    uint32_t count = readU32(f);

    for (uint32_t i = 0; i < count; i++) {
        Task t;
        t.id          = readU32(f);
        t.title       = readStr(f);
        t.description = readStr(f);
        t.priority    = readI32(f);
        t.status      = readI32(f);
        t.createdAt   = readI64(f);
        t.dueDate     = readI64(f);

        uint32_t depCount = readU32(f);
        for (uint32_t d = 0; d < depCount; d++) {
            t.deps.pushBack(readU32(f));
        }

        store.insert(std::move(t));
    }
}

} // namespace FileIO
