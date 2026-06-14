@docs/feat-edit/prd.md 
@docs/feat-edit/plan.md 

# We implemented Phases 1-4, but encountered a problem

The app has accumulated a handful of small inconsistencies — redundant guards, duplicate registrations, a mislabeled UI string, and state being mutated in the wrong place — that each individually don't break anything but together signal the code is slightly ahead of its own housekeeping. The underlying complexity comes from ftxui's event-driven model requiring careful coordination between rendering, focus, and state, which creates subtle timing questions about when something gets reset and who is responsible for it.

I believe it is because of the complexity of remembering when the edited task is dirty what mode are we in and so on

# Solution

I want to simplify the app for now: remove the dirty stage altogether, every edit should be automatically saved in taskStore. The text edit fields having enter/esc functionality is fine, but now the dirty mode + buffs.

**Example**
start app -> list pane (first task) -> enter -> detail pane (title field) -> enter -> edit field...
2 options:
-> enter -> accept edits: save to taskStore
-> esc -> drop edits: load from taskStore

**Aditional functionality for later phase**
Save to binary file before exit, don't save in realtime
Show a dialog box for confirmation

# git log

a780595 (HEAD -> main) fix(tui): cancel detail pane edits on list mouse click
5f95011 feat(tui): implement text field edit mode (Phase 4)
13a3e17 refactor(tui): only call DetailPane::load() on selection change
ec4a63c feat(tui): implement detail pane field navigation (Phase 3)
f62b3b8 feat(tui): refactor DetailPane from free function to class
00c0d46 test(data): add boundary year tests for parseDueDate
729ae95 (origin/main) feat(data): add parseDueDate free function and tests
6f1527e docs: add requirements, PRD, and plan for task create/edit feature

Phase 1 was ok but later phases introduced overcomplicated slop

