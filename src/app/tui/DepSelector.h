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
    std::function<void()> _onMutation;  ///< Called after each dep toggle.
    bool _open = false;                 ///< Whether the overlay is visible.
    unsigned _taskId = 0;               ///< ID of the task whose deps are being edited.
    int _cursor = 0;                    ///< Focused row index into _candidates.
    std::vector<std::pair<unsigned, std::string>> _candidates; ///< {id, title} for all tasks except _taskId.
    std::vector<ftxui::Box> _rowBoxes; ///< Screen boxes for each row, parallel to _candidates (set each render).
    ftxui::Box _dialogBox;             ///< Screen box of the dialog border (used to detect click-outside).

    /** @brief Populates _candidates from the store, excluding _taskId. */
    void buildCandidates();

    /** @brief Toggles the dep at _cursor; no-op if adding it would create a cycle. */
    void toggleCurrentDep();

public:
    static constexpr int MAX_VISIBLE = 8; ///< Max rows visible before scrolling.

    /**
     * @param store      Task store used to read and mutate dependencies.
     * @param onMutation Called after each dep add/remove so the caller can rebuild derived state.
     */
    DepSelector(TaskStore& store, std::function<void()> onMutation);

    /** @brief Opens the selector for the given task. */
    void openForTask(unsigned taskId);

    /** @brief Returns true while the overlay is visible. */
    bool isOpen() const { return _open; }

    /** @brief Closes the overlay. */
    void close() { _open = false; }

    /** @brief Returns the rendered overlay element. */
    ftxui::Element render();

    /** @brief Handles keyboard/mouse events. Always returns true (consumes all). */
    bool onEvent(ftxui::Event e);
};
