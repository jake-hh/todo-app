#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>
#include "../data/TaskStore.h"


/**
 * @brief Modal overlay for adding and removing task dependencies.
 *
 * Shows all tasks except the current one. Checked rows are existing deps.
 * Rows that would create a cycle are dimmed and non-interactive.
 * j/k or arrows navigate; Space/Enter toggles; Esc closes.
 *
 * Usage: call openForTask() to show, isOpen() to query, render()/onEvent()
 * each frame (same pattern as Dialog). All events are consumed while open.
 */
class DepSelector {
    TaskStore& _store;
    std::function<void()> _onMutation;
    bool _open = false;
    unsigned _taskId = 0;
    int _cursor = 0;
    int _offset = 0;
    std::vector<std::pair<unsigned, std::string>> _candidates; // {id, title} in store order
    std::vector<ftxui::Box> _rowBoxes; // screen boxes for visible rows, parallel to rendered rows
    ftxui::Box _dialogBox;             // screen box of the bordered dialog (for click-outside-to-close)

    void buildCandidates();
    void toggleCurrentDep();
    void clampScroll();

public:
    static constexpr int MAX_VISIBLE = 8;

    DepSelector(TaskStore& store, std::function<void()> onMutation);

    /** @brief Opens the selector for the given task. */
    void openForTask(unsigned taskId);

    bool isOpen() const { return _open; }
    void close() { _open = false; }

    ftxui::Element render();

    /** @brief Handles keyboard/mouse events. Always returns true (consumes all). */
    bool onEvent(ftxui::Event e);
};
