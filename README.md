# Build & Run

## Release mode

optimised compilation

```sh
# initial setup (first time only)
mkdir .release
cmake -S . -B .release -DCMAKE_BUILD_TYPE=Release

# build app
cmake --build .release

# run app
.release/todo-app
```


## Development mode

add ASAN

```sh
# initial setup (first time only)
mkdir .debug
cmake -S . -B .debug -DCMAKE_BUILD_TYPE=Debug

# build app & tests
cmake --build .debug

# run app
.debug/todo-app

# run tests (single file)
.debug/tests/<test-file> --gtest_brief=1

# run all tests
ctest --test-dir .debug/tests | tail -4
```

First build will fetch FTXUI and GTest from the internet.
