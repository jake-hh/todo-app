#include "FieldComponents.h"

#include <ftxui/dom/elements.hpp>


using namespace ftxui;


TextField::TextField(std::string label, std::function<std::string()> getValue)
    : _label(std::move(label)), _getValue(std::move(getValue)) {}


Element TextField::OnRender() {
    Element row = hbox({text(_label) | bold, text(_getValue())});
    return Focused() ? (row | inverted | focus) : row;
}


CycleField::CycleField(std::string label, std::function<std::string()> getValue)
    : _label(std::move(label)), _getValue(std::move(getValue)) {}


Element CycleField::OnRender() {
    Element row = hbox({text(_label) | bold, text(_getValue())});
    return Focused() ? (row | inverted | focus) : row;
}
