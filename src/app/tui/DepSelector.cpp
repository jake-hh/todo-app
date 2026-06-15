#include "DepSelector.h"

#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;


DepSelector::DepSelector(TaskStore& store, std::function<void()> onMutation)
    : _store(store), _onMutation(std::move(onMutation)) {}


void DepSelector::buildCandidates() {
    _candidates.clear();
    for (auto& [id, task] : _store.tasks())
        if (id != _taskId)
            _candidates.push_back({id, task.title});
}


void DepSelector::clampScroll() {
    if (_cursor < _offset)
        _offset = _cursor;
    if (_cursor >= _offset + MAX_VISIBLE)
        _offset = _cursor - MAX_VISIBLE + 1;
    int maxOff = std::max(0, static_cast<int>(_candidates.size()) - MAX_VISIBLE);
    _offset = std::max(0, std::min(_offset, maxOff));
}


void DepSelector::toggle() {
    if (_candidates.empty()) return;
    unsigned depId = _candidates[_cursor].first;
    if (_store.wouldCycle(_taskId, depId)) return;

    if (_store.hasDep(_taskId, depId))
        _store.removeDep(_taskId, depId);
    else
        _store.addDep(_taskId, depId);

    _onMutation();
}


void DepSelector::openFor(unsigned taskId) {
    _taskId = taskId;
    _cursor = 0;
    _offset = 0;
    buildCandidates();
    _open = true;
}


Element DepSelector::render() {
    Elements body;
    body.push_back(text(" Space/Enter: toggle    Esc: close") | dim);
    body.push_back(separator());

    if (_candidates.empty()) {
        body.push_back(text(" (no other tasks)") | dim);
        return vbox(std::move(body)) | border | clear_under | center;
    }

    if (_offset > 0)
        body.push_back(text("  \u2191 more") | dim);

    int end = std::min(_offset + MAX_VISIBLE, static_cast<int>(_candidates.size()));
    for (int i = _offset; i < end; i++) {
        auto [cid, ctitle] = _candidates[i];

        bool isDep = _store.hasDep(_taskId, cid);
        bool forbidden = _store.wouldCycle(_taskId, cid);

        std::string check = isDep ? "[x] " : "[ ] ";
        std::string title = ctitle.empty() ? "New task" : ctitle;
        Element row = text(check + "#" + std::to_string(cid) + "  " + title);

        if (forbidden)
            row = row | dim;
        else if (i == _cursor)
            row = row | inverted;

        body.push_back(row);
    }

    if (_offset + MAX_VISIBLE < static_cast<int>(_candidates.size()))
        body.push_back(text("  \u2193 more") | dim);

    return vbox(std::move(body)) | border | clear_under | center;
}


bool DepSelector::onEvent(Event e) {
    int n = static_cast<int>(_candidates.size());
    if (e == Event::Escape) {
        close();
        return true;
    }
    if ((e == Event::ArrowUp || e == Event::Character('k')) && _cursor > 0) {
        _cursor--;
        clampScroll();
        return true;
    }
    if ((e == Event::ArrowDown || e == Event::Character('j')) && _cursor < n - 1) {
        _cursor++;
        clampScroll();
        return true;
    }
    if (e == Event::Character(' ') || e == Event::Return) {
        toggle();
        return true;
    }
    return true; // consume all events while open
}
