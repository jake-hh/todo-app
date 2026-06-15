#include "App.h"

#include <cstdio>
#include <filesystem>
#include <set>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include "../data/FileIO.h"
#include "TaskListPane.h"
#include "DetailPane.h"


App::App(std::string filePath)
    : _filePath(std::move(filePath))
    , _swapPath(_filePath + ".swp")
{
    if (std::filesystem::exists(_swapPath)) {
        _recoverDialogOpen = true;
    } else {
        try { FileIO::load(_filePath, _store); } catch (...) {}
    }

    rebuildTree();
}


void App::writeSwap() {
    try { FileIO::save(_swapPath, _store); } catch (...) {}
}


void App::rebuildTree() {
    _ids.clear();
    _labels.clear();

    // Collect IDs of all tasks that are blockers (appear in any dep list).
    std::set<unsigned> allDeps;
    for (auto& [id, task] : _store.tasks())
        for (size_t i = 0; i < task.deps.size(); i++)
            allDeps.insert(task.deps[i]);

    // Start building on tree roots - tasks that no other task depend on.
    for (auto& [id, task] : _store.tasks())
        if (!allDeps.count(id))
            buildTreeFrom(id, 0);

    // Clamp selection to valid range.
    if (_ids.empty())
        _selected = 0;
    else if (_selected >= static_cast<int>(_ids.size()))
        _selected = static_cast<int>(_ids.size()) - 1;
}


void App::buildTreeFrom(unsigned id, int depth) {
    const Task& t = _store.get(id);

    std::string prefix;
    if (depth > 0)
        prefix = std::string(3 * (depth-1) + 0, ' ') + "└─ ";

    _ids.push_back(id);
    _labels.push_back(prefix + t.statusSymbol() + " " + (t.title.empty() ? "New task" : t.title));

    // DFS on each tasks deps (blockers).
    // Duplicate tasks appear under each parent — no visited guard needed.
    for (size_t i = 0; i < t.deps.size(); i++)
        buildTreeFrom(t.deps[i], depth + 1);
}


void App::run() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    // Both panes share _selected by reference so navigating the list
    // automatically updates the detail view without any explicit sync.
    auto list_pane  = MakeTaskListPane(_labels, _selected, _focusedEntry);
    DetailPane detail_pane(_store, _ids, _selected, [this]{ writeSwap(); rebuildTree(); });
    auto detail_component = detail_pane.component();

    // Container::Horizontal routes keyboard focus between the two panes
    // and handles focus cycling.
    auto layout = Container::Horizontal({list_pane, detail_component});

    Box detail_box;
    int _delFocus = 0;
    int _recoverFocus = 0;
    int _quitFocus = 0;

    // Helpers shared by the renderer and event handler to avoid duplicating
    // the clamped-selection and store-lookup logic.
    auto selIdx  = [&]() { return std::max(0, std::min(_selected, static_cast<int>(_ids.size()) - 1)); };
    auto selTask = [&]() -> const ::Task& { return _store.get(_ids[selIdx()]); };

    // Shared button renderer for all dialogs.
    auto btn = [](const std::string& label, bool focused) -> Element {
        auto e = text("  " + label + "  ") | border;
        return focused ? e | inverted : e;
    };

    // Builds the delete dialog element for the current selection.
    auto renderDialog = [&]() -> Element {
        const ::Task& t = selTask();
        bool hasDeps = !t.deps.isEmpty();

        Elements buttons;
        if (hasDeps) {
            buttons.push_back(btn("Delete All",      _delFocus == 0));
            buttons.push_back(text("  "));
            buttons.push_back(btn("Delete Selected", _delFocus == 1));
            buttons.push_back(text("  "));
            buttons.push_back(btn("Cancel",          _delFocus == 2));
        } else {
            buttons.push_back(btn("Delete", _delFocus == 0));
            buttons.push_back(text("  "));
            buttons.push_back(btn("Cancel", _delFocus == 1));
        }

        Elements lines;
        lines.push_back(text(" \"" + t.title + "\""));
        if (hasDeps)
            lines.push_back(text(" This task has " +
                                 std::to_string(t.deps.size()) +
                                 " direct " +
                                 (t.deps.size() == 1 ? "dependency." : "dependencies.")));
        lines.push_back(separator());
        lines.push_back(hbox(std::move(buttons)) | center);

        return vbox(std::move(lines)) | border | clear_under | center;
    };

    // Builds the swap-file recovery dialog element.
    auto renderRecoverDialog = [&]() -> Element {
        return vbox({
            text(" Unsaved changes found."),
            text(" A swap file exists from a previous session."),
            separator(),
            hbox({
                btn("Recover", _recoverFocus == 0),
                text("  "),
                btn("Discard", _recoverFocus == 1),
            }) | center,
        }) | border | clear_under | center;
    };

    // Builds the quit confirmation dialog element.
    auto renderQuitDialog = [&]() -> Element {
        return vbox({
            text(" Save changes before quitting?"),
            separator(),
            hbox({
                btn("Save [Y]",    _quitFocus == 0),
                text("  "),
                btn("Discard [n]", _quitFocus == 1),
            }) | center,
        }) | border | clear_under | center;
    };

    // Renderer wraps the layout to add the border/title chrome around each pane.
    // Terminal::Size() is queried each frame so the split tracks terminal resizes.
    // When a dialog is open it is layered on top via dbox.
    // Explicit size() avoids ftxui's flex distribution, which allocates space as
    // min_size + extra/2 per pane — causing widths to jump when detail content
    // changes natural width on each selection change.
    auto renderer = Renderer(layout, [&] {
        int w = Terminal::Size().dimx;
        int left_w  = w / 2;
        int right_w = w - left_w;
        auto main = hbox({
            vbox({
                text(" Tasks") | bold,
                separator(),
                list_pane->Render() | flex,
            }) | border | size(WIDTH, EQUAL, left_w),
            vbox({
                text(" Details") | bold,
                separator(),
                detail_component->Render() | flex,
            }) | border | size(WIDTH, EQUAL, right_w) | reflect(detail_box),
        });
        if (_recoverDialogOpen) return dbox({main, renderRecoverDialog()});
        if (_quitDialogOpen)    return dbox({main, renderQuitDialog()});
        if (!_delDialogOpen || _ids.empty()) return main;
        return dbox({main, renderDialog()});
    });

    // Single CatchEvent handles all keyboard input. When a dialog is open it
    // intercepts every event first and consumes them all (returns true),
    // preventing the underlying panes from acting on stray keypresses.
    auto root = CatchEvent(renderer, [&](Event e) {
        if (_recoverDialogOpen) {
            if (e == Event::ArrowLeft || e == Event::Character('h')) {
                if (_recoverFocus > 0) _recoverFocus--;
                return true;
            }
            if (e == Event::ArrowRight || e == Event::Character('l')) {
                if (_recoverFocus < 1) _recoverFocus++;
                return true;
            }
            if (e == Event::Tab || e == Event::TabReverse) {
                _recoverFocus = 1 - _recoverFocus;
                return true;
            }
            if (e == Event::Return) {
                if (_recoverFocus == 0) {
                    try { FileIO::load(_swapPath, _store); } catch (...) {}
                } else {
                    std::remove(_swapPath.c_str());
                    try { FileIO::load(_filePath, _store); } catch (...) {}
                }
                rebuildTree();
                _recoverDialogOpen = false;
                return true;
            }
            return true; // consume all events while recovery dialog is open
        }

        // Block mouse interactions that bypass TextField's own event handler.
        // Container routes mouse events by position, so clicks/hovers in the
        // list pane never reach the focused TextField in the detail pane.
        if (detail_pane.isEditing() && e.is_mouse()) {
            if (e.mouse().motion == Mouse::Moved)
                return true;  // Bug 1: consume hover so Menu can't TakeFocus
            if (e.mouse().motion == Mouse::Pressed && !detail_box.Contain(e.mouse().x, e.mouse().y))
                detail_pane.cancelEdit();  // Bug 2: cancel only for clicks outside detail pane
        }

        if (_delDialogOpen && !_ids.empty()) {
            const ::Task& t = selTask();
            bool hasDeps = !t.deps.isEmpty();
            int numBtns = hasDeps ? 3 : 2;

            if (e == Event::ArrowLeft || e == Event::Character('h')) {
                if (_delFocus > 0) _delFocus--;
                return true;
            }
            if (e == Event::ArrowRight || e == Event::Character('l')) {
                if (_delFocus < numBtns - 1) _delFocus++;
                return true;
            }
            if (e == Event::Tab) {
                _delFocus = (_delFocus + 1) % numBtns;
                return true;
            }
            if (e == Event::TabReverse) {
                _delFocus = (_delFocus - 1 + numBtns) % numBtns;
                return true;
            }
            if (e == Event::Return) {
                unsigned tid = _ids[selIdx()];
                bool mutated = false;
                if (!hasDeps) {
                    if (_delFocus == 0) { _store.removeSplice(tid); mutated = true; }
                    // _delFocus == 1: Cancel — no action
                } else {
                    if (_delFocus == 0)      { _store.removeCascade(tid); mutated = true; }
                    else if (_delFocus == 1) { _store.removeSplice(tid); mutated = true; }
                    // _delFocus == 2: Cancel — no action
                }
                _delDialogOpen = false;
                if (mutated) writeSwap();
                rebuildTree();
                return true;
            }
            if (e == Event::Escape || e == Event::Character('q')) {
                _delDialogOpen = false;
                return true;
            }
            return true; // consume all other events while dialog is open
        }

        if (_quitDialogOpen) {
            if (e == Event::ArrowLeft || e == Event::Character('h')) {
                if (_quitFocus > 0) _quitFocus--;
                return true;
            }
            if (e == Event::ArrowRight || e == Event::Character('l')) {
                if (_quitFocus < 1) _quitFocus++;
                return true;
            }
            if (e == Event::Tab || e == Event::TabReverse) {
                _quitFocus = 1 - _quitFocus;
                return true;
            }
            if (e == Event::Character('Y') || e == Event::Character('y') ||
                    (e == Event::Return && _quitFocus == 0)) {
                FileIO::save(_filePath, _store);
                std::remove(_swapPath.c_str());
                screen.ExitLoopClosure()();
                return true;
            }
            if (e == Event::Escape) {
                _quitDialogOpen = false;
                return true;
            }
            if (e == Event::Character('n') || e == Event::Character('N') ||
                    (e == Event::Return && _quitFocus == 1)) {
                std::remove(_swapPath.c_str());
                screen.ExitLoopClosure()();
                return true;
            }
            return true; // consume all other events while dialog is open
        }

        // Pane focus switching.
        // l/Enter from list pane → enter detail pane (title field).
        // Esc/h from detail pane → return to list pane.
        if ((e == Event::Character('l') || e == Event::Return) && list_pane->Focused()) {
            detail_pane.takeFocus();
            return true;
        }
        if ((e == Event::Escape || e == Event::Character('h'))
                && detail_component->Focused() && !detail_pane.isEditing()) {
            list_pane->TakeFocus();
            return true;
        }

        if (e == Event::Character('n') && !detail_pane.isEditing()) {
            unsigned newId = _store.create("", "", 2, 0, -1LL);
            writeSwap();
            rebuildTree();
            for (int i = 0; i < static_cast<int>(_ids.size()); i++) {
                if (_ids[i] == newId) { _selected = _focusedEntry = i; break; }
            }
            detail_pane.takeFocus();
            return true;
        }
        if (e == Event::Character('q') && !detail_pane.isEditing()) {
            if (!std::filesystem::exists(_swapPath)) {
                screen.ExitLoopClosure()();
            } else {
                _quitFocus = 0;
                _quitDialogOpen = true;
            }
            return true;
        }
        if (e == Event::Character('d') && !_ids.empty() && !detail_pane.isEditing()) {
            _delFocus = selTask().deps.isEmpty() ? 0 : 1;
            _delDialogOpen = true;
            return true;
        }
        return false;
    });

    screen.Loop(root);
}
