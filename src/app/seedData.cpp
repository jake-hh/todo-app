#include <cstdio>
#include "data/TaskStore.h"
#include "data/FileIO.h"


int main() {
    TaskStore store;

    unsigned id0 = store.create("Fix login bug",
                                "Users cannot log in when using OAuth",
                                3, 0, 1800000000LL);
    unsigned id1 = store.create("Write unit tests",
                                "Cover TaskStore CRUD operations",
                                2, 1, -1LL);
    unsigned id2 = store.create("Update documentation",
                                "Align README with new API",
                                1, 0, 1802000000LL);
    unsigned id3 = store.create("Refactor data layer",
                                "Extract serialization into FileIO",
                                2, 2, -1LL);
    unsigned id4 = store.create("Deploy to staging",
                                "Push latest build to staging environment",
                                3, 3, -1LL);
    unsigned id5 = store.create("Provision server",
                                "Spin up staging VM and configure firewall",
                                2, 0, -1LL);
    unsigned id6 = store.create("Set up CI pipeline",
                                "Configure GitHub Actions for staging deploys",
                                2, 1, -1LL);
    unsigned id7 = store.create("Obtain SSL certificate",
                                "Request cert from CA for staging domain",
                                1, 0, -1LL);

    store.get(id0).deps.pushBack(id1);
    store.get(id1).deps.pushBack(id2);
    store.get(id3).deps.pushBack(id1);
    store.get(id3).deps.pushBack(id4);
    store.get(id4).deps.pushBack(id5);
    store.get(id5).deps.pushBack(id6);
    store.get(id6).deps.pushBack(id7);

    const char* path = "default.bin";
    FileIO::save(path, store);
    std::printf("Written: %s\n", path);
    return 0;
}
