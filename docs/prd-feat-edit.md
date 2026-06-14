# PRD: Create and Edit Tasks

## Problem Statement

The app has no way to create new tasks or edit existing ones at runtime. Tasks are currently hardcoded as seed data in `App`. Users need a way to add, edit, and manage tasks through the TUI without recompiling.

## Solution

Extend the existing `DetailPane` to serve three purposes: read-only display (list mode), field navigation (detail mode), and inline text editing (edit mode). Edits are saved to the `TaskStore` immediately on confirmation — there is no dirty tracking, no buffer/snapshot diffing, and no Save/Cancel buttons. The store is always the source of truth. This keeps the state model simple: enter a field, edit it, press Enter to save or Esc to discard.

## User Stories

1. As a user, I want to press `l` or `Enter` on the list pane to enter detail mode, so that I can inspect and edit the selected task.
2. As a user, I want the detail pane to look identical in list mode and detail mode, so that the transition feels seamless.
3. As a user, I want to press `n` from the list pane to create a new task in the store immediately and enter detail mode with focus on the title field, so that I can start editing right away.
4. As a user, I want the list pane to select (but not focus) the newly created task, so that I can see it in the list while focus stays on the detail pane.
5. As a user, I want new tasks to default to priority medium (2) and status open (0) with empty title, description, and due date, so that sensible defaults reduce my input burden.
6. As a user, I want to navigate between editable fields in detail mode using `j` and `k`, so that I can move around without a mouse.
7. As a user, I want the field navigation order to match the rendered order, so that movement feels predictable.
8. As a user, I want non-editable fields (ID, created date, dependencies) to be skipped during field navigation, so that I only land on fields I can change.
9. As a user, I want to press `Enter` on a text field (title, description, due date) to enter edit mode for that field, so that I can type new content.
10. As a user, I want edit mode to show a visible cursor placed after the last character and a light gray background, so that I know I am actively editing.
11. As a user, I want all global shortcuts (`j`, `k`, `n`, `d`, `q`, etc.) to be blocked while in edit mode, so that my keystrokes go into the text field.
12. As a user, I want to press `Enter` in edit mode to confirm the value and save it to the store immediately, so that my change is persisted without a separate save step.
13. As a user, I want to press `Esc` in edit mode to discard my typed changes and reload the field from the store, so that I can undo a mistake.
14. As a user, I want pressing `Enter` on the priority field to cycle through wishlist → low → medium → high → wishlist and save immediately, so that I can change priority with a single keystroke.
15. As a user, I want pressing `Enter` on the status field to cycle through open → in-progress → done → wontfix → open and save immediately, so that I can update status quickly.
16. As a user, I want the due date field to accept free text in DD/MM/YY format, so that I can type dates naturally.
17. As a user, I want an empty due date field to be stored as -1 (no due date), so that I can clear the due date.
18. As a user, I want to see an inline error "Invalid date" in red below the due date field when the value is not valid DD/MM/YY, so that I know what to fix.
19. As a user, I want date validation to run when I confirm the due date field (via `Enter`), rejecting the edit and staying in edit mode if the date is invalid, so that I get immediate feedback.
20. As a user, I want pressing `Esc` in detail mode to return focus to the list pane, so that I have a quick way to go back.
21. As a user, I want `q` to quit the app from detail mode, so that I can exit without switching panes first.
22. As a user, I want `q` to be blocked in edit mode, so that the quit shortcut cannot fire while I am mid-keystroke.
23. As a user, I want pressing `Esc` in detail mode to return focus to the list pane without deleting the task, so that new and existing tasks are treated the same way.
24. As a user, I want the app to save all tasks to the binary file on quit, so that my edits persist across sessions.
25. As a user, I want to see a confirmation dialog when quitting ("Save changes? [Y/n]"), so that I don't accidentally lose or persist unwanted edits.
26. As a user, I want the app to write a swap file (e.g. `.tasks.bin.swp`) after every store mutation, so that an unexpected crash or kill does not lose my work — similar to how vim protects unsaved buffers.

## Implementation Decisions

### Modules

**`DetailPane` (refactored from free function to class)**

The existing `MakeDetailPane()` free function is replaced with a `DetailPane` class. It takes `TaskStore&`, `const std::vector<unsigned>& ids`, and `int& selected` in its constructor — the same shared references the current free function uses. The pane looks up the selected task via `store.get(ids[selected])` each frame, so no pointer or explicit `load()` call is needed. It owns minimal edit state — only a temporary edit string for the field currently being edited. The store is always the source of truth for display.

Public interface:
- Constructor takes `TaskStore&`, `const std::vector<unsigned>& ids`, `int& selected`.
- `component()` — returns the `ftxui::Component` to be placed in the layout.

Internal state:
- `std::string _editStr` — temporary string for the field currently in edit mode.
- `int _editingField` — which field is in edit mode (-1 = none).

**`App` (modified)**

- Replaces `MakeDetailPane()` call with a `DetailPane` instance, passing `_store`, `_ids`, `_selected`.
- On `n`: calls `_store.create(...)` with defaults, rebuilds tree, sets `_selected` to new task's index.
- Calls `FileIO::save()` on quit.

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

`Container::Vertical` natively handles `j`/`k` and arrow keys to move between focusable children. Non-focusable children (read-only fields) are skipped automatically.

### Text Field Wrapper

Each editable text field is a wrapper component with two visual states:
- **Hover:** renders the current store value as plain highlighted text; no cursor.
- **Edit:** renders an ftxui `Input()` component with a visible cursor and light gray background.

`Enter` in hover state → copy store value to `_editStr`, switch to edit. `Enter` in edit state → write `_editStr` back to store via `TaskStore::update()`, return to hover. `Esc` in edit state → discard `_editStr`, return to hover.

For the title field: `Enter` in edit mode rejects an empty `_editStr` — stay in edit mode (same pattern as invalid date). A task must always have a non-empty title.

For the due date field: `Enter` in edit mode validates `_editStr` with `parseDueDate()`. If invalid, reject the confirm — stay in edit mode and show the "Invalid date" error. If valid, save and return to hover.

For the description field: single-line only for now. `Enter` confirms (same as title). Multi-line editing (with `Ctrl+Enter` to confirm) is a future addition.

Display placeholder: when `task.title.empty()`, the detail pane and list pane render "New task" as a placeholder. Since empty titles are rejected on confirm, `title.empty()` reliably means "just created, never titled."

### New Task Flow

When `n` is pressed, a new task is created in the store immediately with defaults (empty title, empty description, priority 2, status 0, dueDate -1). `_selected` is set to the new task's index in the list. The detail pane loads the new task and focuses the title field. The list pane shows the new task selected, displayed as "New task" (the empty-title placeholder).

Pressing `Esc` in detail mode returns focus to the list pane — the task remains in the store. There is no cancel/delete flow; new tasks and existing tasks are treated identically.

### Key Binding Table

| Key                         | List mode             | Detail mode           | Edit mode                     |
|-----------------------------|-----------------------|-----------------------|-------------------------------|
| j / k / ArrowDown / ArrowUp | move selection + load | move between fields   | typed into field              |
| Enter                       | focus detail pane     | edit/cycle field      | save field to store           |
| l / ArrowRight              | focus detail pane     | -                     | typed into field              |
| Esc                         | —                     | focus list pane       | discard edit, return to hover |
| h / ArrowLeft               | —                     | focus list pane       | typed into field              |
| n                           | start new task        | start new task        | typed into field              |
| d                           | delete dialog         | delete dialog         | typed into field              |
| q                           | quit (confirm dialog) | quit (confirm dialog) | typed into field              |

### Mouse Interactions

| Click target                 | Always                            | List mode           | Detail mode                    | Edit mode                      |
|------------------------------|-----------------------------------|---------------------|--------------------------------|--------------------------------|
| Task in list pane            | Select task + load in detail pane | -                   | focus list mode                | Discard edit + focus list mode |
| Field in detail pane         | Focus that field                  | Focus detail pane   | -                              | Discard edit + return to hover |
| Already-focused field        | -                                 | -                   | Enter edit mode for that field | Save edit + return to hover    |

### Date Parsing

- Format: DD/MM/YY parsed manually (split on `/`, validate ranges) then converted to `int64_t` via `mktime()`. No `strptime` — it is POSIX-only and unavailable on MSVC.
- Empty string → stored as -1 (no due date); always valid.
- Invalid format or impossible date (e.g. 31/02/26) → rejected, stay in edit mode.

### Quit and Save Flow

The main file (`tasks.bin`) is only written on quit. The swap file (`.tasks.bin.swp`) is written after every store mutation for crash recovery. This means the store may contain changes not yet in the main file.

When `q` is pressed, a confirmation dialog appears: "Save changes? [Y/n]". Pressing `Y` or `Enter` calls `FileIO::save()` to the main file, deletes the swap file, then exits. Pressing `n` or `Esc` deletes the swap file and exits without saving to the main file — all edits since the last quit-save are lost.

### Swap File (Crash Recovery)

After every store mutation (field save, new task, delete), the full task store is written to a swap file (`.tasks.bin.swp`, derived from the main file path). On startup, if a swap file exists, the app prompts the user to recover from it or discard it — similar to vim's swap file recovery. On clean quit (either save or discard), the swap file is deleted.

## Testing Decisions

Good tests verify observable behavior through public interfaces only — no inspection of internal buffers or private fields.

**Date parsing** is the primary test target and is already implemented as a free function (`parseDueDate(std::string) -> std::optional<int64_t>`):
- Valid date "13/06/26" → correct epoch value.
- Empty string → -1.
- Garbage input → nullopt.
- Impossible date "31/02/26" → nullopt.
- Round-trip: format a known epoch with `dueDateStr()`, parse it back, get the same day.

**`TaskStore`** already has tests in `tests/taskStoreTests.cpp` — no new store tests needed for this feature.

**Not tested:** ftxui rendering, key binding wiring in `App`, visual hover/edit states.

## Out of Scope

- Dirty tracking, Save/Cancel buttons, buffer/snapshot diffing.
- Editing dependencies (blocked-by relationships) from the detail pane.
- Undo/redo across multiple edits.
- Field-level validation beyond due date format.
- Confirmation dialog before discarding field edits (detail mode Esc).
- Sorting or reordering the task list after save.
- Real-time main file persistence (the main file is only written on quit; the swap file handles crash recovery).

## Further Notes

- `Container::Vertical` handles arrow key navigation natively. A `CatchEvent` wrapper remaps `j`/`k` to `ArrowDown`/`ArrowUp` in list and detail modes. In edit mode, this remapping is disabled so that all letter keys (`j`, `k`, `n`, `d`, `l`, `h`, `q`, etc.) pass through to the text input.
- The simplification from the original PRD (removing dirty tracking) was motivated by accumulated state-coordination bugs in Phases 2-4 of the original implementation. Auto-save eliminates the entire category of buffer-vs-store consistency issues.
