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


void TextField::enterEdit() {
    _editStr = _getEditValue();
    _cursor = static_cast<int>(_editStr.size());
    _editing = true;
    *_editingFlag = true;
}


void TextField::confirmEdit() {
    if (_trySetValue(_editStr)) {
        _editing = false;
        _showError = false;
        *_editingFlag = false;
    } else {
        _showError = true;
    }
}


void TextField::discardEdit() {
    _editStr.clear();
    _editing = false;
    _showError = false;
    *_editingFlag = false;
}


TextField::TextField(std::string label,
                     std::function<std::string()> getValue,
                     std::function<bool(const std::string&)> trySetValue,
                     std::string errorMsg,
                     std::shared_ptr<bool> editingFlag,
                     std::function<std::string()> getEditValue)
    : _label(std::move(label))
    , _getValue(std::move(getValue))
    , _trySetValue(std::move(trySetValue))
    , _errorMsg(std::move(errorMsg))
    , _editingFlag(std::move(editingFlag))
    , _getEditValue(getEditValue ? std::move(getEditValue) : _getValue)
{}


bool TextField::OnEvent(Event e) {
    if (_editing) {
        if (e.is_mouse()) {
            // Press outside the box: discard and let click reach its target.
            if (e.mouse().motion == Mouse::Pressed && !_box.Contain(e.mouse().x, e.mouse().y)) {
                discardEdit();
                return false;
            }
            return false;
        }
        if (e == Event::Return) {
            confirmEdit();
            return true;
        }
        if (e == Event::Escape) {
            discardEdit();
            return true;
        }
        if (e == Event::Backspace) {
            _showError = false;
            if (_cursor > 0) {
                _editStr.erase(_cursor - 1, 1);
                _cursor--;
            }
            return true;
        }
        if (e == Event::ArrowLeft) {
            if (_cursor > 0) _cursor--;
            return true;
        }
        if (e == Event::ArrowRight) {
            if (_cursor < static_cast<int>(_editStr.size())) _cursor++;
            return true;
        }
        if (e.is_character()) {
            _showError = false;
            _editStr.insert(_cursor, e.character());
            _cursor += static_cast<int>(e.character().size());
            return true;
        }
        // Consume all other keys — blocks j/k/n/d/q etc.
        return true;
    }

    // Hover mode: Enter activates edit.
    if (Focused() && e == Event::Return) {
        enterEdit();
        return true;
    }
    return FieldBase::OnEvent(e);
}


Element TextField::OnRender() {
    if (_editing) {
        std::string before = _editStr.substr(0, _cursor);
        std::string at     = _cursor < static_cast<int>(_editStr.size())
                             ? std::string(1, _editStr[_cursor]) : " ";
        std::string after  = _cursor < static_cast<int>(_editStr.size())
                             ? _editStr.substr(_cursor + 1) : "";
        Element editRow = hbox({text(_label) | bold, text(before), text(at) | inverted, text(after)})
                          | bgcolor(Color::GrayDark);
        if (_showError && !_errorMsg.empty())
            return vbox({editRow, text(_errorMsg) | color(Color::Red)}) | reflect(_box);
        return editRow | reflect(_box);
    }

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
