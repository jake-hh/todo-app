#include "App.h"

#include <set>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include "TaskListPane.h"
#include "DetailPane.h"


App::App() {
    // Hardcode seed tasks with stub deps to demonstrate the tree view.
    {
        unsigned id0 = _store.create("Fix login bug",
                                     "Users cannot log in when using OAuth",
                                     3, 0, 1800000000LL);

        unsigned id1 = _store.create("Write unit tests",
                                     "Cover TaskStore CRUD operations",
                                     2, 1, -1LL);

        unsigned id2 = _store.create("Update documentation",
                                     "Align README with new API",
                                     1, 0, 1802000000LL);

        unsigned id3 = _store.create("Refactor data layer",
                                     "Extract serialization into FileIO",
                                     2, 2, -1LL);

        unsigned id4 = _store.create("Deploy to staging",
                                     "Push latest build to staging environment",
                                     3, 3, -1LL);

        unsigned id5 = _store.create("Provision server",
                                     "Spin up staging VM and configure firewall",
                                     2, 0, -1LL);

        unsigned id6 = _store.create("Set up CI pipeline",
                                     "Configure GitHub Actions for staging deploys",
                                     2, 1, -1LL);

        unsigned id7 = _store.create("Obtain SSL certificate",
                                     "Request cert from CA for staging domain",
                                     1, 0, -1LL);

        // Stub deps: id0 blocked by id1, id1 blocked by id2,
        // id3 blocked by both id1 and id4 (demonstrates a duplicate node).
        // id4 blocked by id5, id5 blocked by id6, id6 blocked by id7
        _store.get(id0).deps.pushBack(id1);
        _store.get(id1).deps.pushBack(id2);
        _store.get(id3).deps.pushBack(id1);
        _store.get(id3).deps.pushBack(id4);
        _store.get(id4).deps.pushBack(id5);
        _store.get(id5).deps.pushBack(id6);
        _store.get(id6).deps.pushBack(id7);
    }

    rebuildTree();
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
    _labels.push_back(prefix + t.statusSymbol() + " " + t.title);

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
    auto list_pane  = MakeTaskListPane(_labels, _selected);
    DetailPane detail_pane(_store, _ids, _selected, [this]{ rebuildTree(); });
    auto detail_component = detail_pane.component();

    // Container::Horizontal routes keyboard focus between the two panes
    // and handles focus cycling.
    auto layout = Container::Horizontal({list_pane, detail_component});

    Box detail_box;

    // Helpers shared by the renderer and event handler to avoid duplicating
    // the clamped-selection and store-lookup logic.
    auto selIdx  = [&]() { return std::max(0, std::min(_selected, static_cast<int>(_ids.size()) - 1)); };
    auto selTask = [&]() -> const ::Task& { return _store.get(_ids[selIdx()]); };

    // Builds the delete dialog element for the current selection.
    auto renderDialog = [&]() -> Element {
        const ::Task& t = selTask();
        bool hasDeps = !t.deps.isEmpty();

        auto btn = [](const std::string& label, bool focused) -> Element {
            auto e = text("  " + label + "  ") | border;
            return focused ? e | inverted : e;
        };

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

    // Renderer wraps the layout to add the border/title chrome around each pane.
    // Terminal::Size() is queried each frame so the split tracks terminal resizes.
    // When the delete dialog is open it is layered on top via dbox.
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
        if (!_delDialogOpen || _ids.empty()) return main;
        return dbox({main, renderDialog()});
    });

    // Single CatchEvent handles all keyboard input. When the delete dialog is
    // open it intercepts every event first and consumes them all (returns true),
    // preventing the underlying panes from acting on stray keypresses.
    auto root = CatchEvent(renderer, [&](Event e) {
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
                if (!hasDeps) {
                    if (_delFocus == 0) _store.removeSplice(tid);
                    // _delFocus == 1: Cancel — no action
                } else {
                    if (_delFocus == 0)      _store.removeCascade(tid);
                    else if (_delFocus == 1) _store.removeSplice(tid);
                    // _delFocus == 2: Cancel — no action
                }
                _delDialogOpen = false;
                rebuildTree();
                return true;
            }
            if (e == Event::Escape || e == Event::Character('q')) {
                _delDialogOpen = false;
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

        if (e == Event::Character('q')) {
            screen.ExitLoopClosure()();
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
