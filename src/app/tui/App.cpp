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
#include "Dialog.h"
#include "DepSelector.h"


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

    bool hasFilter = !_searchQuery.empty() || _dateFilter != 0 || _priorityFilter != -1 || _statusFilter != -1;

    if (hasFilter) {
        // Flat list of matching tasks in ID order
        SmartArray<unsigned> matches = _store.search(_searchQuery, _dateFilter, _priorityFilter, _statusFilter);

        for (size_t i = 0; i < matches.size(); i++) {
            const Task& t = _store.get(matches[i]);
            _ids.push_back(matches[i]);
            _labels.push_back(t.statusSymbol() + " " + (t.title.empty() ? "New task" : t.title));
        }
    } else {
        // Full dependency tree (existing logic)
        std::set<unsigned> allDeps;
        for (auto& [id, task] : _store.getTasks())
            for (size_t i = 0; i < task.deps.size(); i++)
                allDeps.insert(task.deps[i]);

        for (auto& [id, task] : _store.getTasks())
            if (!allDeps.count(id))
                buildTreeFromTask(id, 0);
    }

    // Clamp selection to valid range.
    if (_ids.empty())
        _selected = 0;
    else if (_selected >= static_cast<int>(_ids.size()))
        _selected = static_cast<int>(_ids.size()) - 1;
}


void App::buildTreeFromTask(unsigned id, int depth) {
    const Task& t = _store.get(id);

    std::string prefix;
    if (depth > 0)
        prefix = std::string(3 * (depth-1) + 0, ' ') + "└─ ";

    _ids.push_back(id);
    _labels.push_back(prefix + t.statusSymbol() + " " + (t.title.empty() ? "New task" : t.title));

    // DFS on each tasks deps (blockers).
    // Duplicate tasks appear under each parent — no visited guard needed.
    for (size_t i = 0; i < t.deps.size(); i++)
        buildTreeFromTask(t.deps[i], depth + 1);
}


void App::run() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    auto selIdx = [&]() {
        return std::max(0, std::min(_selected, static_cast<int>(_ids.size()) - 1));
    };

    DepSelector depSelector(_store, [this]{ writeSwap(); rebuildTree(); });

    // Both panes share _selected by reference so navigating the list
    // automatically updates the detail view without any explicit sync.
    TaskListPane taskListPane(
        _searchQuery,
        _dateFilter,
        _priorityFilter,
        _statusFilter,
        _labels,
        _selected,
        _focusedEntry,
        [this]{ rebuildTree(); }
    );
    auto list_pane = taskListPane.getComponent();
    DetailPane detail_pane(_store, _ids, _selected,
        [this]{ writeSwap(); rebuildTree(); },
        [&]{
            if (!_ids.empty())
                depSelector.openFor(_ids[selIdx()]);
        });
    auto detail_component = detail_pane.getComponent();

    // Container::Horizontal routes keyboard focus between the two panes
    // and handles focus cycling.
    auto layout = Container::Horizontal({list_pane, detail_component});

    Box detail_box;
    Dialog delDialog, recoverDialog, quitDialog;

    // Helper shared by the renderer and event handler.
    auto selTask = [&]() -> const ::Task& { return _store.get(_ids[selIdx()]); };

    recoverDialog.open(
        {"Recover", "Discard"},
        {
            [&]{ try { FileIO::load(_swapPath, _store); } catch (...) {}
                 rebuildTree(); _recoverDialogOpen = false; },
            [&]{ std::remove(_swapPath.c_str());
                 try { FileIO::load(_filePath, _store); } catch (...) {}
                 rebuildTree(); _recoverDialogOpen = false; },
        }
    );

    quitDialog.open(
        {"Save [Y]", "Discard [n]"},
        {
            [&]{ FileIO::save(_filePath, _store); std::remove(_swapPath.c_str()); screen.ExitLoopClosure()(); },
            [&]{ std::remove(_swapPath.c_str()); screen.ExitLoopClosure()(); },
        }
    );

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
        if (_recoverDialogOpen) {
            return dbox({main, recoverDialog.render({
                text(" Unsaved changes found."),
                text(" A swap file exists from a previous session."),
            })});
        }
        if (_quitDialogOpen) {
            return dbox({main, quitDialog.render({
                text(" Save changes before quitting?"),
            })});
        }
        if (depSelector.isOpen())
            return dbox({main, depSelector.render()});
        if (!_delDialogOpen || _ids.empty()) return main;
        const ::Task& t = selTask();
        Elements body;
        body.push_back(text(" \"" + t.title + "\""));
        if (!t.deps.isEmpty())
            body.push_back(text(" This task has " + std::to_string(t.deps.size()) +
                                " direct " + (t.deps.size() == 1 ? "dependency." : "dependencies.")));
        return dbox({main, delDialog.render(std::move(body))});
    });

    // Single CatchEvent handles all keyboard input. When a dialog is open it
    // intercepts every event first and consumes them all (returns true),
    // preventing the underlying panes from acting on stray keypresses.
    auto root = CatchEvent(renderer, [&](Event e) {
        if (_recoverDialogOpen) {
            recoverDialog.onEvent(e);
            return true;
        }

        if (depSelector.isOpen())
            return depSelector.onEvent(e);

        // Block mouse interactions that bypass TextField's own event handler.
        // Container routes mouse events by position, so clicks/hovers in the
        // list pane never reach the focused TextField in the detail pane.
        if (detail_pane.isEditing() && e.is_mouse()) {
            if (e.mouse().motion == Mouse::Moved)
                return true;  // Bug 1: consume hover so Menu can't TakeFocus
            if (e.mouse().motion == Mouse::Pressed && !detail_box.Contain(e.mouse().x, e.mouse().y))
                detail_pane.cancelEdit();  // Bug 2: cancel only for clicks outside detail pane
        }

        // When the search input has focus, ftxui's Input can eat mouse clicks
        // before they reach the detail pane. Transfer focus to the menu first so
        // the click propagates correctly.
        if (taskListPane.isSearchFocused() && e.is_mouse()
                && e.mouse().motion == Mouse::Pressed
                && detail_box.Contain(e.mouse().x, e.mouse().y)) {
            taskListPane.takeFocus();
            return false;
        }

        if (_delDialogOpen && !_ids.empty()) {
            if (e == Event::Escape || e == Event::Character('q')) { _delDialogOpen = false; return true; }
            delDialog.onEvent(e);
            return true;
        }

        if (_quitDialogOpen) {
            if (e == Event::Escape)                                              { _quitDialogOpen = false;  return true; }
            if (e == Event::Character('y') || e == Event::Character('Y'))        { quitDialog.trigger(0);    return true; }
            if (e == Event::Character('n') || e == Event::Character('N'))        { quitDialog.trigger(1);    return true; }
            quitDialog.onEvent(e);
            return true;
        }

        // Pane focus switching.
        // l/Enter from list pane → enter detail pane (title field).
        // Esc/h from detail pane → return to list pane.
        if ((e == Event::Character('l') || e == Event::Return || e == Event::ArrowRight)
                && list_pane->Focused() && !taskListPane.isSearchFocused()) {
            detail_pane.takeFocus();
            return true;
        }
        if ((e == Event::Escape || e == Event::Character('h'))
                && detail_component->Focused() && !detail_pane.isEditing()) {
            taskListPane.takeFocus();
            return true;
        }

        if (e == Event::Character('n') && !detail_pane.isEditing() && !taskListPane.isSearchFocused()) {
            unsigned newId = _store.create("", "", 2, 0, -1LL);
            writeSwap();
            rebuildTree();
            for (int i = 0; i < static_cast<int>(_ids.size()); i++) {
                if (_ids[i] == newId) { _selected = _focusedEntry = i; break; }
            }
            detail_pane.takeFocus();
            return true;
        }
        if (e == Event::Character('q') && !detail_pane.isEditing() && !taskListPane.isSearchFocused()) {
            if (!std::filesystem::exists(_swapPath)) {
                screen.ExitLoopClosure()();
            } else {
                quitDialog.focus = 0;
                _quitDialogOpen = true;
            }
            return true;
        }
        if (e == Event::Character('d') && !_ids.empty() && !detail_pane.isEditing() && !taskListPane.isSearchFocused()) {
            const ::Task& t = selTask();
            bool hasDeps = !t.deps.isEmpty();
            unsigned tid = _ids[selIdx()];
            if (hasDeps) {
                delDialog.open(
                    {"Delete All", "Delete Selected", "Cancel"},
                    {
                        [&, tid]{ _store.removeCascade(tid); writeSwap(); rebuildTree(); _delDialogOpen = false; },
                        [&, tid]{ _store.removeSplice(tid);  writeSwap(); rebuildTree(); _delDialogOpen = false; },
                        [&]     { _delDialogOpen = false; },
                    },
                    1  // default: "Delete Selected" (safer than cascade)
                );
            } else {
                delDialog.open(
                    {"Delete", "Cancel"},
                    {
                        [&, tid]{ _store.removeSplice(tid); writeSwap(); rebuildTree(); _delDialogOpen = false; },
                        [&]     { _delDialogOpen = false; },
                    }
                );
            }
            _delDialogOpen = true;
            return true;
        }
        return false;
    });

    screen.Loop(root);
}
