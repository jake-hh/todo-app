#include "FieldComponents.h"

#include <ftxui/dom/elements.hpp>


using namespace ftxui;


bool FieldBase::OnEvent(Event e) {
    if (e.is_mouse() && e.mouse().motion == Mouse::Pressed
            && _box.Contain(e.mouse().x, e.mouse().y) && CaptureMouse(e)) {
        TakeFocus();
        return true;
    }
    return false;
}


TextField::TextField(std::string label, std::function<std::string()> getValue)
    : _label(std::move(label)), _getValue(std::move(getValue)) {}


Element TextField::OnRender() {
    Element row = hbox({text(_label) | bold, text(_getValue())});
    if (Focused()) row = row | inverted | focus;
    return row | reflect(_box);
}


CycleField::CycleField(std::string label, std::function<std::string()> getValue)
    : _label(std::move(label)), _getValue(std::move(getValue)) {}


Element CycleField::OnRender() {
    Element row = hbox({text(_label) | bold, text(_getValue())});
    if (Focused()) row = row | inverted | focus;
    return row | reflect(_box);
}
