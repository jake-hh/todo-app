# Plan: Create and Edit Tasks

> Source PRD: docs/feat-edit/prd.md

## Architectural decisions

- **`DetailPane`**: refactored from a free function to a class. Takes `TaskStore&`, `const std::vector<unsigned>& ids`, and `int& selected` by reference — looks up the selected task via `store.get(ids[selected])` each frame. No explicit `load()` call needed.
- **No dirty tracking**: edits are saved to the store immediately on `Enter`. No buffers, no snapshot, no Save/Cancel buttons. The store is always the source of truth.
- **Mode derivation**: no explicit mode enum. List mode = list pane focused. Detail mode = detail pane focused, no text input active. Edit mode = detail pane focused, a text input active (tracked internally by `DetailPane` via `_editingField`).
- **Edit state**: `_editStr` (temporary string for active edit) and `_editingField` (which field is being edited, -1 = none). That's it.
- **New task flow**: `n` creates a task in the store immediately with defaults, sets `_selected` to the new task's index. No deferred creation, no negative encoding.
- **Date parsing**: manual parse (split on `/`, validate ranges) + `mktime()`. No `strptime`. `parseDueDate(string) -> std::optional<int64_t>` is a free function (already implemented in Phase 1).
- **Persistence**: main file written on quit only. Swap file (`.tasks.bin.swp`) written after every store mutation for crash recovery.

---

## Phase 1: Date parsing free function ✅

**Already implemented** (commit `729ae95`).

`parseDueDate(string) -> std::optional<int64_t>` with GTest coverage.

---

## Phase 2: DetailPane class with store-driven rendering

**User stories**: 1, 2, 6, 7, 8

### Open question — edit mode coordination

`App` owns global shortcuts (`n`, `d`, `q`) but `DetailPane` owns `_editingField`. Need to decide how `App` knows edit mode is active (e.g. `DetailPane::isEditing()` method). Decide before implementing.

### What to build

Replace `MakeDetailPane()` free function with a `DetailPane` class. Constructor takes `TaskStore&`, `const std::vector<unsigned>& ids`, `int& selected`. The pane looks up the selected task via `store.get(ids[selected])` each frame and renders from it — no buffers. Field layout: read-only header (ID, created, deps) as non-focusable `Renderer`, then focusable wrapper components for title, status, priority, due date, description. `Container::Vertical` handles `j`/`k` navigation natively.

### Acceptance criteria

- [ ] App compiles and runs with `DetailPane` class replacing the free function
- [ ] Detail pane display is visually unchanged from current behaviour
- [ ] `l`/`Enter` from list pane moves focus to detail pane
- [ ] First editable field (title) is highlighted on entry
- [ ] `j`/`k` move focus down/up through editable fields only
- [ ] `Esc` / `h` / `ArrowLeft` returns focus to list pane with same selection
- [ ] Global shortcuts (`n`, `d`, `q`) still work in detail mode

---

## Phase 3: Text field edit mode (title, description, due date)

**User stories**: 9, 10, 11, 12, 13, 16, 17, 18, 19

### What to build

Title, description, and due date fields get a two-state wrapper component. Hover state: plain highlighted text. Edit state: active `Input()` with cursor and gray background, all global shortcuts blocked. `Enter` in hover → copy store value to `_editStr`, switch to edit. `Enter` in edit → write `_editStr` back to store, return to hover. `Esc` in edit → discard `_editStr`, return to hover.

Due date validation: `Enter` in edit mode calls `parseDueDate()`. If invalid, reject — stay in edit mode, show red "Invalid date" error via `Maybe()`. If valid (or empty), save and return to hover.

Title validation: `Enter` in edit mode rejects empty `_editStr` — stay in edit mode.

### Acceptance criteria

- [ ] `Enter` on a text field activates edit mode (cursor visible, gray background)
- [ ] Typing in edit mode updates `_editStr`
- [ ] `Enter` in edit mode saves to store and returns to hover
- [ ] `Esc` in edit mode discards edit and returns to hover
- [ ] Due date: invalid value rejected, "Invalid date" error shown, stays in edit mode
- [ ] Due date: valid value or empty string saved correctly
- [ ] Title: empty value rejected, stays in edit mode
- [ ] Description field uses single-line input; `Enter` confirms (no newlines)
- [ ] All global shortcuts blocked while in edit mode

---

## Phase 4: Cycle fields (priority and status)

**User stories**: 14, 15

### What to build

Priority and status fields are cycle components. `Enter` when hovered cycles the value and saves to the store immediately. No edit mode — the change is instant. Priority order: wishlist → low → medium → high → wishlist. Status order: open → in-progress → done → wontfix → open.

### Acceptance criteria

- [ ] `Enter` on priority cycles through all four values and wraps
- [ ] `Enter` on status cycles through all four values and wraps
- [ ] Cycled value is saved to store immediately
- [ ] Cycled value is reflected in the rendered field

---

## Phase 5: New task flow

**User stories**: 3, 4, 5

### Open question — list position of new task

Since `std::map` orders by ID and `nextId` is always max+1, new tasks appear at the bottom. Decide before implementing whether this needs scroll-to-selection behavior or is fine as-is.

### What to build

`n` from list mode (or detail mode) calls `_store.create(...)` with defaults (empty title, empty description, priority 2, status 0, dueDate -1), rebuilds the tree, sets `_selected` to the new task's index, and moves focus to the detail pane with the title field focused. The list pane shows the new task selected, displayed as "New task" (empty-title placeholder). No cancel flow — new tasks persist in the store like any other task.

### Acceptance criteria

- [ ] `n` creates a new task in the store with correct defaults
- [ ] New task appears in the list and is selected
- [ ] Detail pane focuses the title field
- [ ] List pane shows "New task" placeholder for empty title
- [ ] `Esc` in detail mode returns to list pane — task remains in store
- [ ] `n` is blocked in edit mode

---

## Phase 6: Swap file (crash recovery)

**User stories**: 26

### What to build

After every store mutation (field save, cycle, new task, delete), write the full store to a swap file (`.tasks.bin.swp`, derived from main file path). On startup, if a swap file exists, prompt the user to recover or discard. On clean quit, delete the swap file.

### Acceptance criteria

- [ ] Swap file is written after every store mutation
- [ ] On startup with swap file present, recovery prompt appears
- [ ] Choosing recover loads from swap file
- [ ] Choosing discard deletes swap file, loads from main file
- [ ] Clean quit deletes the swap file

---

## Phase 7: Quit confirmation and main file save

**User stories**: 24, 25

### What to build

`q` shows a confirmation dialog: "Save changes? [Y/n]". `Y`/`Enter` saves to main file, deletes swap file, exits. `n`/`Esc` deletes swap file, exits without saving. `q` is blocked in edit mode.

### Acceptance criteria

- [ ] `q` shows confirmation dialog
- [ ] `Y`/`Enter` saves and exits
- [ ] `n`/`Esc` exits without saving
- [ ] `q` is blocked in edit mode
- [ ] Swap file is deleted on either exit path
