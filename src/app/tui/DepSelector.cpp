#include "DepSelector.h"

#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;


DepSelector::DepSelector(TaskStore& store, std::function<void()> onMutation)
    : _store(store), _onMutation(std::move(onMutation)) {}


void DepSelector::buildCandidates() {
    _candidates.clear();
    for (auto& [id, task] : _store.getTasks())
        if (id != _taskId)
            _candidates.push_back({id, task.title});
}



void DepSelector::toggleCurrentDep() {
    if (_candidates.empty()) return;
    unsigned depId = _candidates[_cursor].first;
    if (_store.wouldCycle(_taskId, depId)) return;

    if (_store.hasDep(_taskId, depId))
        _store.removeDep(_taskId, depId);
    else
        _store.addDep(_taskId, depId);

    _onMutation();
}


void DepSelector::openForTask(unsigned taskId) {
    _taskId = taskId;
    _cursor = 0;
    buildCandidates();
    _open = true;
}


Element DepSelector::render() {
    Elements body;
    body.push_back(text(" Space/Enter: toggle    Esc: close") | dim);
    body.push_back(separator());

    if (_candidates.empty()) {
        body.push_back(text(" (no other tasks)") | dim);
        return vbox(std::move(body)) | border | clear_under | reflect(_dialogBox) | center;
    }

    Elements rows;
    _rowBoxes.resize(_candidates.size());
    for (int i = 0; i < static_cast<int>(_candidates.size()); i++) {
        auto [cid, ctitle] = _candidates[i];

        bool isDep = _store.hasDep(_taskId, cid);
        bool forbidden = _store.wouldCycle(_taskId, cid);

        std::string check = isDep ? "[x] " : "[ ] ";
        std::string title = ctitle.empty() ? "New task" : ctitle;
        Element row = text(check + "#" + std::to_string(cid) + "  " + title);

        if (forbidden)
            row = row | dim;
        else if (i == _cursor)
            row = row | inverted | focus;

        rows.push_back(row | reflect(_rowBoxes[i]));
    }

    body.push_back(vbox(std::move(rows)) | vscroll_indicator | yframe | size(HEIGHT, LESS_THAN, MAX_VISIBLE));

    return vbox(std::move(body)) | border | clear_under | reflect(_dialogBox) | center;
}


bool DepSelector::onEvent(Event e) {
    int n = static_cast<int>(_candidates.size());
    if (e == Event::Escape) {
        close();
        return true;
    }
    if ((e == Event::ArrowUp || e == Event::Character('k')) && _cursor > 0) {
        _cursor--;
        return true;
    }
    if ((e == Event::ArrowDown || e == Event::Character('j')) && _cursor < n - 1) {
        _cursor++;
        return true;
    }
    if (e == Event::Character('g') && n > 0) {
        _cursor = 0;
        return true;
    }
    if (e == Event::Character('G') && n > 0) {
        _cursor = n - 1;
        return true;
    }
    if (e == Event::Character(' ') || e == Event::Return) {
        toggleCurrentDep();
        return true;
    }
    if (e.is_mouse()) {
        auto& m = e.mouse();
        if (m.button == Mouse::WheelUp && _cursor > 0) {
            _cursor--;
            return true;
        }
        if (m.button == Mouse::WheelDown && _cursor < n - 1) {
            _cursor++;
            return true;
        }
        if (m.button == Mouse::Left && m.motion == Mouse::Pressed) {
            bool insideDialog = m.x >= _dialogBox.x_min && m.x <= _dialogBox.x_max &&
                                m.y >= _dialogBox.y_min && m.y <= _dialogBox.y_max;
            if (!insideDialog) {
                close();
                return true;
            }
            for (int i = 0; i < static_cast<int>(_rowBoxes.size()); i++) {
                auto& box = _rowBoxes[i];
                if (m.y >= box.y_min && m.y <= box.y_max) {
                    _cursor = i;
                    toggleCurrentDep();
                    return true;
                }
            }
        }
        return true;
    }
    return true; // consume all events while open
}
