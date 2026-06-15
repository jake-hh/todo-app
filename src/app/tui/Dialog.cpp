#include "Dialog.h"

#include <ftxui/dom/elements.hpp>

using namespace ftxui;


void Dialog::open(std::vector<std::string> lbls,
                  std::vector<std::function<void()>> acts,
                  int defaultFocus) {
    labels  = std::move(lbls);
    actions = std::move(acts);
    boxes.assign(labels.size(), Box{});
    focus = defaultFocus;
}


Element Dialog::render(Elements body) {
    // Re-apply hover: if mouse is over a button, override keyboard-driven focus.
    // Uses previous frame's box positions, so this is always one frame fresh.
    for (int i = 0; i < (int)boxes.size(); i++)
        if (boxes[i].Contain(mouseX, mouseY)) { focus = i; break; }

    auto btn = [](const std::string& label, bool focused) -> Element {
        auto e = text("  " + label + "  ") | border;
        return focused ? e | inverted : e;
    };
    Elements btns;
    for (int i = 0; i < (int)labels.size(); i++) {
        if (i > 0) btns.push_back(text("  "));
        btns.push_back(btn(labels[i], focus == i) | reflect(boxes[i]));
    }
    body.push_back(separator());
    body.push_back(hbox(std::move(btns)) | center);
    return vbox(std::move(body)) | border | clear_under | center;
}


void Dialog::trigger(int idx) {
    if (idx >= 0 && idx < (int)actions.size())
        actions[idx]();
}


void Dialog::onEvent(Event e) {
    int n = (int)labels.size();
    if (e.is_mouse()) {
        mouseX = e.mouse().x;
        mouseY = e.mouse().y;
        for (int i = 0; i < n; i++)
            if (boxes[i].Contain(mouseX, mouseY)) { focus = i; break; }
        if (e.mouse().motion == Mouse::Pressed) trigger(focus);
        return;
    }
    if (e == Event::ArrowLeft  || e == Event::Character('h')) { if (focus > 0)     focus--;         return; }
    if (e == Event::ArrowRight || e == Event::Character('l')) { if (focus < n - 1) focus++;         return; }
    if (e == Event::Tab)        { focus = (focus + 1) % n;     return; }
    if (e == Event::TabReverse) { focus = (focus - 1 + n) % n; return; }
    if (e == Event::Return)     { trigger(focus);               return; }
}
