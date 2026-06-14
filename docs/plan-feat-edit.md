# Plan: Create and Edit Tasks

> Source PRD: docs/prd-edit-create-tasks.md

## Architectural decisions

- **`DetailPane`**: refactored from a free function to a class that owns all edit state (buffers, snapshot, date validity). App holds a `DetailPane` instance and calls its public interface.
- **Mode derivation**: no explicit mode enum. List mode = list pane focused. Detail mode = detail pane focused, no text input active. Edit mode = detail pane focused, a text input active (tracked internally by `DetailPane`).
- **Snapshot**: `std::optional<Task> _snapshot` inside `DetailPane`. Nullopt = new task. Has value = editing existing task. Set by `load()` on every j/k selection change.
- **Buffers**: `_bufTitle`, `_bufDescription`, `_bufDueDate` (string), `_bufPriority`, `_bufStatus` (int). The detail pane always renders from buffers — identical in list mode and detail mode.
- **New task selection encoding**: `_selected = -(old + 1)` when creating a new task. Negative `_selected` means deselected list. Restore with `-(_selected + 1)` on cancel/save.
- **Date parsing**: manual parse (split on `/`, validate ranges) + `mktime()`. No `strptime` (POSIX-only, unavailable on MSVC). `parseDueDate(string) -> std::optional<int64_t>` is a free function.
- **Save/Cancel visibility**: `Maybe()` ftxui decorator. Save visible when `isDirty() && isDateValid()`. Cancel visible when `isDirty()`.

---

## Phase 1: Date parsing free function

**User stories**: 16, 17, 18, 19

### What to build

Extract date parsing into a standalone free function `parseDueDate(string) -> std::optional<int64_t>`. Empty string returns -1. Valid DD/MM/YY returns epoch seconds. Invalid format or impossible date returns nullopt. Write GTest tests covering all cases.

### Acceptance criteria

- [ ] Valid "13/06/26" parses to correct epoch value
- [ ] Empty string returns -1
- [ ] Garbage input returns nullopt
- [ ] Impossible date "31/02/26" returns nullopt
- [ ] Round-trip: format a known epoch with `dueDateStr()`, parse back, get same day
- [ ] All tests pass

---

## Phase 2: DetailPane refactored to class

**User stories**: 1, 2, 6, 7, 8

### What to build

Replace `MakeDetailPane()` free function with a `DetailPane` class. The class owns buffers and snapshot. `load(const Task&)` sets both buffers and snapshot. App calls `load()` on every j/k selection change. The detail pane renders from buffers — visually identical to the current read-only view. Public interface `isDirty()`, `isDateValid()`, `cancel()` implemented but not yet wired to any key.

### Acceptance criteria

- [ ] App compiles and runs with `DetailPane` class replacing the free function
- [ ] Detail pane display is visually unchanged from current behaviour
- [ ] `load()` correctly populates buffers from a task
- [ ] `isDirty()` returns false immediately after `load()`
- [ ] `isDateValid()` returns true for a task with a valid due date and for -1

---

## Phase 3: Enter detail mode and field navigation

**User stories**: 1, 6, 7, 8

### What to build

`l` or `Enter` on the list pane moves focus to the detail pane and hovers the first editable field. `j`/`k` navigate between editable fields in rendered order (title, status, priority, due date, description). Non-editable fields (ID, created, deps) are skipped. `Esc` in detail mode returns focus to the list pane. No editing yet — fields render as plain highlighted text when hovered.

### Acceptance criteria

- [ ] `l` (ftxui native) and `Enter` (wired via CatchEvent) from list pane move focus to detail pane
- [ ] First editable field (title) is highlighted on entry
- [ ] `j`/`k` move focus down/up through editable fields only
- [ ] `Esc` returns focus to list pane with same selection
- [ ] Global shortcuts (`n`, `d`, `q`) still work in detail mode

---

## Phase 4: Text field hover/edit wrapper

**User stories**: 9, 10, 11, 12, 13, 16, 17, 18, 19

### What to build

Title, description, and due date fields get a two-state wrapper component. In hover state: plain highlighted text, no cursor. In edit state: active text input with visible cursor and light gray background, all global shortcuts blocked. `Enter` in hover → edit. `Enter` in edit → accept value, return to hover. `Esc` in edit → revert field to its pre-edit value, return to hover. After exiting edit mode on due date, `_dateValid` is recomputed and an inline red "Invalid date" message appears via `Maybe()` when invalid.

### Acceptance criteria

- [ ] Pressing `Enter` on a text field activates edit mode (cursor visible, gray background)
- [ ] Typing in edit mode updates the buffer
- [ ] `Enter` in edit mode confirms and returns to hover
- [ ] `Esc` in edit mode reverts the field and returns to hover
- [ ] "Invalid date" error appears below due date field when value is invalid
- [ ] "Invalid date" error disappears when value is corrected or cleared
- [ ] All global shortcuts are blocked while in edit mode

---

## Phase 5: Cycle fields (priority and status)

**User stories**: 14, 15

### What to build

Priority and status fields are cycle components. Pressing `Enter` when a cycle field is hovered increments the value mod the number of options (wraps around). No edit mode — the change is immediate and reflected in the buffer. Priority order: wishlist → low → medium → high → wishlist. Status order: open → in-progress → done → wontfix → open.

### Acceptance criteria

- [ ] `Enter` on priority cycles through all four values and wraps
- [ ] `Enter` on status cycles through all four values and wraps
- [ ] Cycled value is reflected immediately in the rendered field
- [ ] `isDirty()` returns true after cycling away from the snapshot value
- [ ] `isDirty()` returns false after cycling back to the snapshot value

---

## Phase 6: Save and Cancel

**User stories**: 20, 21, 22, 23, 24, 25

### What to build

Save and Cancel buttons appear below the fields when the form is dirty. Save is additionally hidden when the due date is invalid. Both are navigable with `j`/`k`. Pressing Save calls `_detailPane.save(_store)`, rebuilds the tree, selects the saved task in the list, and returns focus to the list pane. Pressing Cancel or `Esc` in detail mode calls `_detailPane.cancel()`, restores buffers from snapshot, and returns focus to the list pane.

### Acceptance criteria

- [ ] Save and Cancel buttons are not visible when form is not dirty
- [ ] Save and Cancel buttons appear when any field is changed
- [ ] Save button is hidden when due date is invalid
- [ ] Pressing Save persists the task in the store
- [ ] After Save, list pane has focus and saved task is selected
- [ ] Pressing Cancel discards changes and returns focus to list pane
- [ ] `Esc` in detail mode discards all dirty changes (restores buffers from snapshot) and returns focus to list pane

---

## Phase 7: New task flow

**User stories**: 3, 4, 5, 29

### What to build

Pressing `n` from list mode calls `_detailPane.startNew()` (clears buffers to defaults: title/description/dueDate empty, priority 2, status 0, snapshot nullopt), encodes the current `_selected` as `-(old + 1)`, and moves focus to the detail pane. The list pane renders no highlighted item while `_selected < 0`. On Save, the new task is created, tree is rebuilt, and the new task is selected. On Cancel or `Esc`, `_selected` is restored from the encoding and `_detailPane.load()` is called with that task.

### Acceptance criteria

- [ ] `n` from list mode opens the detail pane with empty/default fields
- [ ] No item is highlighted in the list pane during new task creation
- [ ] New task defaults: priority medium, status open, empty title/description/due date
- [ ] Saving creates a new task with a unique ID and correct `createdAt`
- [ ] New task appears in the list and is selected after save
- [ ] Canceling restores focus to the previously selected task

---

## Phase 8: Guards (n, d, q when dirty or in edit mode)

**User stories**: 26, 27, 28

### What to build

In detail mode when dirty: `n` and `d` are blocked (no-op). `q` is blocked. In edit mode: `n`, `d`, `q`, and all non-text-input keys are blocked.

### Acceptance criteria

- [ ] `n` is a no-op in detail mode when dirty
- [ ] `d` is a no-op in detail mode when dirty
- [ ] `q` is a no-op in detail mode when dirty
- [ ] `q` is a no-op in edit mode
- [ ] All of the above work correctly when form is not dirty (allowed)
