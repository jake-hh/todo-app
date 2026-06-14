# PRD: Create and Edit Tasks

## Problem Statement

The app has no way to create new tasks or edit existing ones at runtime. Tasks are currently hardcoded as seed data in `App`. Users need a way to add, edit, and manage tasks through the TUI without recompiling.

## Solution

Extend the existing `DetailPane` to serve three purposes: read-only display (list mode), field navigation (detail mode), and inline text editing (edit mode). No separate form component is introduced. The right pane transforms in place based on user input, keeping the layout identical across all modes and making the UX feel cohesive.

## User Stories

1. As a user, I want to press `l` or `Enter` on the list pane to enter detail mode, so that I can inspect and edit the selected task.
2. As a user, I want the detail pane to look identical in list mode and detail mode, so that the transition feels seamless.
3. As a user, I want to press `n` from the list pane to open a blank task form in detail mode, so that I can create a new task.
4. As a user, I want the list pane to show no selected item while creating a new task, so that it is visually clear the task does not exist yet.
5. As a user, I want new tasks to default to priority medium (2) and status open (0) with empty title, description, and due date, so that sensible defaults reduce my input burden.
6. As a user, I want to navigate between editable fields in detail mode using `j` and `k`, so that I can move around without a mouse.
7. As a user, I want the field navigation order to match the rendered order, so that movement feels predictable.
8. As a user, I want non-editable fields (ID, created date, dependencies) to be skipped during field navigation, so that I only land on fields I can change.
9. As a user, I want to press `Enter` on a text field (title, description, due date) to enter edit mode for that field, so that I can type new content.
10. As a user, I want edit mode to show a visible cursor placed after the last character and a light gray background, so that I know I am actively editing.
11. As a user, I want all global shortcuts (`j`, `k`, `n`, `d`, `q`, etc.) to be blocked while in edit mode, so that my keystrokes go into the text field.
12. As a user, I want to press `Enter` in edit mode to confirm the value and return to detail mode, so that I can accept my changes.
13. As a user, I want to press `Esc` in edit mode to discard my typed changes and revert the field to its previous value, then return to detail mode, so that I can undo a mistake without affecting the rest of the form.
14. As a user, I want pressing `Enter` on the priority field to cycle through wishlist → low → medium → high → wishlist, so that I can change priority with a single keystroke.
15. As a user, I want pressing `Enter` on the status field to cycle through open → in-progress → done → wontfix → open, so that I can update status quickly.
16. As a user, I want the due date field to accept free text in DD/MM/YY format, so that I can type dates naturally.
17. As a user, I want an empty due date field to be stored as -1 (no due date), so that I can clear the due date.
18. As a user, I want to see an inline error "Invalid date" in red below the due date field when the value is not valid DD/MM/YY, so that I know what to fix.
19. As a user, I want date validation to run every time I exit edit mode on the due date field (via `Enter` or `Esc`), so that I get immediate feedback.
20. As a user, I want Save and Cancel buttons to appear only when the form is dirty (at least one field differs from the saved state), so that I am not shown irrelevant actions.
21. As a user, I want the Save button to appear only when the form is dirty AND the due date is valid (or empty), so that I cannot save a task with a broken date.
22. As a user, I want to navigate to Save and Cancel with `j`/`k` just like any other field, so that I can confirm or discard without leaving the keyboard flow.
23. As a user, I want pressing Save to persist the task and return focus to the list pane with the saved task selected, so that I can continue working.
24. As a user, I want pressing Cancel to discard all changes and return focus to the list pane, so that I can abandon edits easily.
25. As a user, I want pressing `Esc` in detail mode to discard all changes and return to list mode, so that I have a quick escape that mirrors Cancel.
26. As a user, I want `n` and `d` to be blocked in detail mode when the form is dirty, so that I do not accidentally discard unsaved changes.
27. As a user, I want `q` to quit the app from detail mode only when the form is not dirty, so that I cannot accidentally lose unsaved work.
28. As a user, I want `q` to be blocked in edit mode, so that the quit shortcut cannot fire while I am mid-keystroke.
29. As a user, I want canceling a new task (with no saved snapshot) to restore focus to the previously selected item in the list, so that the list position is not lost.

## Implementation Decisions

### Modules

**`DetailPane` (refactored from free function to class)**

The existing `MakeDetailPane()` free function is replaced with a `DetailPane` class. It owns all edit state internally (buffers, snapshot, date validity), exposing a minimal interface to `App`:

- `load(const Task&)` — called on every j/k selection change; copies editable fields to buffers and sets the snapshot. This is the only place the snapshot is set for existing tasks — `l`/Enter just moves focus, snapshot is already current.
- `startNew()` — called on `n`; clears buffers to defaults, sets snapshot to nullopt.
- `isDirty() const` — true if any buffer differs from snapshot, or snapshot is nullopt.
- `isDateValid() const` — true if `_bufDueDate` is empty or parses as a valid DD/MM/YY date.
- `save(TaskStore&)` — if snapshot has value: calls `update()`; if nullopt: calls `create()`. Returns the ID of the saved task.
- `cancel()` — restores buffers from snapshot (no-op if nullopt; App handles navigation).
- `component()` — returns the `ftxui::Component` to be placed in the layout.

Internal state:
- `std::optional<Task> _snapshot` — nullopt means new task; has-value means editing existing.
- `std::string _bufTitle, _bufDescription, _bufDueDate`
- `int _bufPriority, _bufStatus`
- `bool _dateValid`

**`App` (modified)**

- Replaces `MakeDetailPane()` call with a `DetailPane` instance.
- Calls `_detailPane.load()` on every selection change (j/k in list mode).
- Calls `_detailPane.startNew()` on `n`; encodes previous selection as `_selected = -(old + 1)`.
- Checks `_detailPane.isDirty()` before allowing `n`, `d`, `q` in detail mode.
- Calls `_detailPane.save()` / `_detailPane.cancel()` and handles navigation after.

### Mode Derivation

No explicit mode enum. Mode is derived:
- **List mode:** list pane has focus (`list_pane->Focused()`).
- **Detail mode:** detail pane has focus, no text input is active.
- **Edit mode:** detail pane has focus, a text input is active (tracked internally by `DetailPane`).

### Field Layout

The detail pane is a `Container::Vertical` of:
1. ID, created date, deps — read-only `Renderer`, non-focusable.
2. Title — wrapper component (hover/edit).
3. Status — cycle component.
4. Priority — cycle component.
5. Due date — wrapper component (hover/edit) + inline error via `Maybe()`.
6. Description — wrapper component (hover/edit).
7. Save button — `Maybe([&]{ return isDirty() && isDateValid(); })`.
8. Cancel button — `Maybe([&]{ return isDirty(); })`.

`Container::Vertical` natively handles `j`/`k` and arrow keys to move between focusable children. Non-focusable children (read-only fields) are skipped automatically.

### Text Field Wrapper

Each editable text field is a wrapper component with two visual states:
- **Hover:** renders the current buffer value as plain highlighted text; no cursor.
- **Edit:** renders an ftxui `Input()` component with a visible cursor and light gray background.

`Enter` in hover state → switch to edit. `Enter` in edit state → accept value, return to hover. `Esc` in edit state → revert buffer to the per-field snapshot value, return to hover.

After exiting edit mode on the due date field (either `Enter` or `Esc`), `_dateValid` is recomputed.

### New Task Selection Encoding

When `n` is pressed, `_selected` is encoded as `-(old + 1)`. This encodes the return position in a single variable: a negative `_selected` means "new task, deselected list." The list pane renders no highlight when `_selected < 0`. On cancel/save, the original index is recovered as `-(_selected + 1)`.

### Key Binding Table

| Key | List mode | Detail mode (not dirty) | Detail mode (dirty) | Edit mode |
|-----|-----------|------------------------|---------------------|-----------|
| j/k | move selection + load | move between fields | move between fields | blocked |
| l / Enter | focus detail pane | Enter: edit/cycle field | Enter: edit/cycle field | Enter: accept; blocked for l |
| Esc | — | discard + focus list | discard + focus list | revert field, return to detail |
| n | start new task | allowed | blocked | blocked |
| d | delete dialog | allowed | blocked | blocked |
| q | quit | quit | blocked | blocked |

### Save Flow

1. `_detailPane.save(_store)` is called.
2. Internally: parse `_bufDueDate` → `int64_t`; call `_store.create()` or `_store.update()`.
3. App calls `rebuildTree()`, sets `_selected` to the saved task's list index, returns focus to list pane.

### Cancel / Esc Flow

1. `_detailPane.cancel()` restores buffers from snapshot (if snapshot has value).
2. If `_snapshot == nullopt` (new task): App recovers prior selection from `_selected` encoding, calls `_detailPane.load()` with that task.
3. Focus returns to list pane.

### Date Parsing

- Format: DD/MM/YY parsed manually (split on `/`, validate ranges) then converted to `int64_t` via `mktime()`. No `strptime` — it is POSIX-only and unavailable on MSVC.
- Empty string → stored as -1 (no due date); always valid.
- Invalid format or impossible date (e.g. 31/02/26) → `_dateValid = false`.

## Testing Decisions

Good tests verify observable behavior through public interfaces only — no inspection of internal buffers or private fields.

**Date parsing** is the primary test target and should be extracted as a free function (`parseDueDate(std::string) -> std::optional<int64_t>`) so it can be tested without constructing any ftxui component:
- Valid date "13/06/26" → correct epoch value.
- Empty string → -1.
- Garbage input → nullopt.
- Impossible date "31/02/26" → nullopt.
- Round-trip: format a known epoch with `dueDateStr()`, parse it back, get the same day.

**`DetailPane`** testing is optional. Constructing a `DetailPane` in a test requires ftxui components, which may not behave correctly outside a running `ScreenInteractive`. If attempted, test only through the non-rendering public interface (`load`, `isDirty`, `isDateValid`, `save`, `cancel`) and skip any assertions that involve rendering. If it proves too brittle, omit and rely on the `parseDueDate` unit tests plus manual TUI testing.

Suggested `DetailPane` tests if feasible:
- `load()` then `isDirty()` returns false.
- `load()` then mutate a buffer, `isDirty()` returns true.
- `startNew()` then `isDirty()` returns true.
- `cancel()` after edits restores `isDirty()` to false.
- `save()` on a new task creates a task in the store with correct field values.
- `save()` on an existing task updates the correct task in the store.

**`TaskStore`** already has tests in `tests/taskStoreTests.cpp` — no new store tests needed for this feature.

**Not tested:** ftxui rendering, key binding wiring in `App`, visual hover/edit states.

## Out of Scope

- Editing dependencies (blocked-by relationships) from the detail pane.
- Undo/redo across multiple edits.
- Field-level validation beyond due date format.
- Confirmation dialog before discarding dirty changes.
- Sorting or reordering the task list after save.

## Further Notes

- The `_selected` negative encoding (`-(old + 1)`) is a deliberate design choice to avoid a second `_prevSelected` variable. Document this clearly in code comments.
- `Container::Vertical` j/k navigation is built into ftxui — no `CatchEvent` remapping needed for field navigation within the detail pane.
- The `Maybe()` component decorator is used to show/hide Save and Cancel buttons, which avoids the need for a "disabled button" concept that ftxui does not natively support.
