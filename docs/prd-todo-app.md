# PRD: Terminal Todo / Ticket Manager

## Problem Statement

Managing everyday tasks and study work is difficult without a structured tool. Existing apps are either too heavy or not owned by the developer. A lightweight terminal app that models tasks with dependency relationships — where tasks can block other tasks — would allow natural planning ("finish X before starting Y") without forcing an artificial hierarchy like subtasks or projects. The app must also serve as a CS class submission, so it must satisfy specific academic requirements around data structures, file I/O, and TUI presentation.

## Solution

A TUI application written in C++ using ftxui that manages a list of tasks stored with a unique ID system. Tasks can depend on other tasks (be "blocked by" them), forming an arbitrary DAG. The UI shows a tree view of this dependency graph and provides search, filtering, and aggregate statistics. Data is persisted to a binary file. The app uses the custom `SmartArray<T>` library for dependency lists and ordered display, satisfying the academic requirement.

## User Stories

1. As a user, I want to create a new task with a title, description, priority, and optional due date, so that I can track what needs to be done.
2. As a user, I want each task to receive a unique auto-assigned numeric ID, so that I can reference tasks unambiguously.
3. As a user, I want to set a task's status to one of: open, in-progress, done, or wontfix, so that I can track its lifecycle.
4. As a user, I want to set a task's priority to one of: wishlist (0), low (1), medium (2), or high (3), so that I can communicate urgency.
5. As a user, I want to assign a due date to a task, so that I know when it needs to be completed.
6. As a user, I want to see the creation date on every task automatically, so that I have a record of when work was logged.
7. As a user, I want to add a dependency between two tasks (task A is blocked by task B), so that I can model prerequisite relationships.
8. As a user, I want the app to prevent me from creating circular dependencies (A → B → A), so that the dependency graph remains valid.
9. As a user, I want forbidden dependency targets to be grayed out and unselectable in the dependency picker, so that I cannot accidentally create a cycle.
10. As a user, I want to remove a dependency between two tasks, so that I can decouple tasks that no longer need to be ordered.
11. As a user, I want to edit any field of an existing task, so that I can correct mistakes or update status.
12. As a user, I want to delete a task, so that I can remove completed or irrelevant items.
13. As a user, I want to see a warning when marking a task as done if it still has unresolved (open or in-progress) dependencies, so that I don't accidentally close blocked work.
14. As a user, I want to confirm or cancel the "mark done despite unresolved deps" warning, so that I retain control.
15. As a user, I want tasks marked as done or wontfix to be treated as resolved for blocking purposes, so that finishing a dependency in any terminal state unblocks dependents.
16. As a user, I want to view all tasks in a tree layout showing dependency relationships, so that I can understand the structure of my work at a glance.
17. As a user, I want duplicate tasks in the MVP tree view to be visible (a task blocking multiple others appears multiple times), so that the tree renders without extra complexity.
18. As a user, I want duplicate tasks in the tree to be grayed out in a later polish pass, so that I can distinguish the canonical occurrence from references.
19. As a user, I want to toggle between tree view modes (all-tasks tree, open/in-progress-only tree, non-blocked tasks list), so that I can focus on what's actionable.
20. As a user, I want to see a detail view of a single task showing all its fields, dependency count, and a list of its direct dependencies, so that I have the full picture in one place.
21. As a user, I want to search tasks by title substring, so that I can find tasks quickly.
22. As a user, I want to filter tasks by date range (overdue, due today, due this week, no due date, all), so that I can focus on time-sensitive work.
23. As a user, I want to filter tasks by priority, so that I can focus on the most important items.
24. As a user, I want to filter tasks by status, so that I can view only open or in-progress tasks.
25. As a user, I want to combine search and filter fields simultaneously, so that I can narrow results precisely.
26. As a user, I want to see aggregate counters — total tasks by status, overdue count, today-due count, high-priority count — visible in the list pane, so that I have a dashboard at a glance.
27. As a user, I want to see a count of tasks transitively blocked by a selected task, so that I understand the blast radius of a blocker.
28. As a user, I want to save all tasks to a binary file, so that my data persists between sessions.
29. As a user, I want tasks to load automatically from the binary file on startup, so that I can resume where I left off.
30. As a user, I want the save file path to default to `tasks.bin` next to the executable, so that no configuration is needed out of the box.
31. As a user, I want to override the save file path via a CLI argument, so that I can maintain multiple task lists.
32. As a user, I want the layout to adapt to the terminal window size (side-by-side or stacked panes), so that the app is usable at any width.
33. As a user, I want the MVP to use a fixed layout, with responsive layout added as a polish step, so that the app ships functional first.

## Implementation Decisions

### Modules

**Data layer** (`src/app/data/`)

- `Task` — plain struct: `unsigned id`, `std::string title`, `std::string description`, `int priority` (0–3), `int status` (0–3 mapped to open/in-progress/done/wontfix), `int64_t createdAt`, `int64_t dueDate` (-1 = none), `SmartArray<unsigned> deps`. All fields documented with Doxygen.
- `TaskStore` — owns `std::map<unsigned, Task>` as primary store (O(log n) access, deterministic iteration in ID order). Exposes create, update, delete, and query operations. Manages `nextId` (derived as `max(id)+1` on load). Contains cycle-detection DFS, search/filter returning `SmartArray<unsigned>`, and all aggregate functions.
- `FileIO` — stateless functions for binary serialization/deserialization of the full task list. Dates stored as `int64_t`, strings as `uint32_t` length prefix + bytes, dep arrays as `uint32_t` count + array of `uint32_t`.

**TUI layer** (`src/app/tui/`)

- `App` — root ftxui component, owns `TaskStore`, wires all child components, handles global keyboard shortcuts.
- `TaskListPane` — renders the task tree, search bar, and stat counters. Owns current filter/search state.
- `DetailPane` — displays all fields of the selected task including dep count and dep list.
- `EditForm` — form component for creating and editing tasks.
- `DepSelector` — multi-select list for adding/removing dependencies; forbidden tasks (those that would cause a cycle) are rendered as dimmed and non-interactive.

**Entry point** (`src/app/main.cpp`) — parses optional CLI arg for file path, defaults to `tasks.bin` next to the executable, and passes the resolved path to `App`.

`App` owns the file path, calls `FileIO::load` in its constructor (missing file → empty store + UI notice; corrupt file → stderr + exit), and calls `FileIO::save` on quit. A swap file is written after every mutation for crash recovery (see `prd-feat-edit.md`).

### Key Architecture Decisions

- Primary task store is `std::map<unsigned, Task>` for O(log n) ID-based access and deterministic iteration in ID order. `SmartArray` is used for `deps` inside each `Task` and for search result sets.
- IDs are auto-incremented unsigned integers. `nextId` is not persisted; it is re-derived from `max(id)+1` after loading.
- Dates are `int64_t` seconds since Unix epoch in memory and in the file. User-facing format is DD/MM/YY using `<ctime>` (`localtime`, `strftime`, `mktime`). No external date library.
- Due-in-2-days check: `difftime(dueDate, time(nullptr)) <= 2*24*3600`.
- Cycle detection runs a DFS from the candidate dependency target; if the current task is reachable, the dependency is rejected.
- `wontfix` and `done` are both treated as "resolved" for blocking-status calculations.
- ftxui v6.1.9 added via CMake `FetchContent`.
- File splitting heuristic: aim for 50–300 LOC per file; split or merge accordingly during implementation.
- Doxygen comments on all source files (public interfaces at minimum).

### Tree View Modes (MVP first, rest later)

1. **MVP**: all tasks tree, duplicates shown as-is, sorted by ID ascending (std::map iteration order).
2. All tasks tree, duplicate nodes grayed.
3. Open + in-progress tasks only tree.
4. Non-blocked tasks only, sorted by priority descending then due date ascending.

## Testing Decisions

Good tests verify observable behavior through public interfaces only — no inspection of internal state, no mocking of `SmartArray` internals.

**Modules to test:**

- `TaskStore` — the richest test target: task CRUD, cycle detection (should reject), dependency resolution (blocked/unblocked status), search/filter combinations, aggregate functions, `nextId` derivation on load.
- `FileIO` — round-trip test: write a known set of tasks, reload, assert equality. Tests edge cases: empty store, task with no deps, task with multiple deps, task with no due date (-1).

**Prior art:** `tests/smartArrayTests.cpp` using GTest — follow the same file/target structure under `tests/`.

**Not tested:** TUI components (ftxui rendering is not unit-testable in isolation), `main.cpp` wiring.

## Out of Scope

- Due-time granularity below one day (no hour/minute on due dates).
- Recurring tasks or task templates.
- Multi-user or networked task sharing.
- Tags or labels beyond priority and status.
- Undo/redo.
- Task comments or attachments.
- Sorting the main list (beyond the toggled tree view modes).
- Configurable color themes.
- Generating Doxygen HTML (comments in code are sufficient per requirements).

## Further Notes

- The project must compile cleanly with the Visual Studio compiler (MSVC). Avoid GCC/Clang extensions. Use `int64_t` not `time_t` in binary file format to avoid platform size differences.
- The academic requirement is "at least one instance of SmartArray in the app" — the design uses it in two places (`deps` in each Task, search result sets).
- The `SmartArray` implementation may be replaced with `std::vector` after the assignment is submitted; the data layer is intentionally isolated so this swap touches only `Task.h` and `TaskStore`.
