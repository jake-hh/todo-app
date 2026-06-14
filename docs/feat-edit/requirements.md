# Edit Tasks Requirements

What I want is to use the same DetailPane for both viewing and editing and creating new tasks. That way the UX will be cleaner.

Only editable fields can be selected/hovered over:
- title - text field
- description - text field
- due date - ?
- priority - cycled field
- status - cycled field

## List mode

1. pressing [l] or [enter] enters detail mode

    - move focus to detail pane
    - hover over first editable field

2. pressing [n] enters detail mode but for a new task that doesn't exit yet

    - move focus to detail pane
    - hover over first editable field

3. pressing [d] triggers task deletion (already implemented)


## Detail mode

state of the text fields but also state of the app

- just hover over the field, don't edit it
- shortcuts like d, n, j, k, tab, etc. should normally work
- selection (white) should be normally visible
- no cursor or invisible
- show Save and Cancel buttons only if sth has changed - it is dirty

1. pressing [j]/[k] moves up and down the editable fields of the detail pane (moving order must reflect rendered order) also include the buttons if pressent

2. pressing [n]/[d] creates / deletes task - only if nothing in the task was changed: nothing is dirty

3. pressing [enter] on a text field switches it's state from hover to edit

4. pressing [enter] on a status/priority field cycles through its options (in a circle)

5. pressing [esc] should return to the list mode

    - move focus back to list pane
    - select the last selected task in the list (if necessary)


## Edit mode

state of the text fields but also state of the app

- edit the field, don't move anywhere - lock the selection in place
- all shortcuts d, n, j, k, tab, etc. stop working
- the background of the text field goes light gray + cursor is set behind the last char and is set visible

1. pressing [enter] - save changes and return to Detail mode

2. pressing [esc] - drop changes and return to Detail mode
