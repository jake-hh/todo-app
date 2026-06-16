#include "FieldComponents.h"

#include <ftxui/dom/elements.hpp>


using namespace ftxui;


Element FieldBase::renderHoverRow() const {
    Element row = hbox({text(_label) | bold, text(_getValue())});
    if (Focused()) row = row | inverted | focus;
    return row;
}


bool FieldBase::OnEvent(Event e) {
    if (e.is_mouse() && e.mouse().motion == Mouse::Pressed
            && _box.Contain(e.mouse().x, e.mouse().y) && CaptureMouse(e)) {
        if (_cancelEdit && *_cancelEdit) (*_cancelEdit)();
        TakeFocus();
        return true;
    }
    return false;
}


// Populates the edit buffer from _getEditValue, places cursor at end, and
// registers discardEdit in the shared cancelEdit hook so other fields can
// cancel this edit without knowing which TextField is active.
void TextField::enterEdit() {
    _editStr = _getEditValue();
    _cursor = static_cast<int>(_editStr.size());
    _editing = true;
    if (_cancelEdit) *_cancelEdit = [this] { discardEdit(); };
}


// Calls trySetValue; on success exits edit mode and clears the cancel hook.
// On failure sets _showError so the next render shows the error message.
void TextField::confirmEdit() {
    if (_trySetValue(_editStr)) {
        _editing = false;
        _showError = false;
        if (_cancelEdit) *_cancelEdit = nullptr;
    } else {
        _showError = true;
    }
}


// Exits edit mode and clears the cancel hook without saving.
void TextField::discardEdit() {
    _editing = false;
    _showError = false;
    if (_cancelEdit) *_cancelEdit = nullptr;
}


TextField::TextField(std::string label,
                     std::function<std::string()> getValue,
                     std::function<bool(const std::string&)> trySetValue,
                     std::string errorMsg,
                     std::shared_ptr<std::function<void()>> cancelEdit,
                     std::function<std::string()> getEditValue)
    : FieldBase(std::move(label), std::move(getValue))
    , _trySetValue(std::move(trySetValue))
    , _errorMsg(std::move(errorMsg))
    // _getValue is in FieldBase (initialized before derived members) — safe to copy as fallback.
    , _getEditValue(getEditValue ? std::move(getEditValue) : _getValue)
{
    _cancelEdit = std::move(cancelEdit);
}


bool TextField::OnEvent(Event e) {
    if (_editing) {
        if (e.is_mouse() && e.mouse().motion == Mouse::Pressed) {
            if (_box.Contain(e.mouse().x, e.mouse().y)) {
                // Click on self while editing → save (same as Enter).
                CaptureMouse(e);
                confirmEdit();
                return true;
            } else {
                // Click elsewhere → discard and let the click reach its target.
                discardEdit();
                return false;
            }
        }
        if (e.is_mouse()) return false;

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

    // Hover mode: Enter or click-on-focused activates edit.
    if (Focused() && e == Event::Return) {
        enterEdit();
        return true;
    }
    if (e.is_mouse() && e.mouse().motion == Mouse::Pressed
            && _box.Contain(e.mouse().x, e.mouse().y) && Focused()) {
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
        Element editRow = hbox({
            text(_label) | bold,
            hbox({text(before), text(at) | inverted, text(after)}) | bgcolor(Color::GrayDark) | xflex,
        });
        if (_showError && !_errorMsg.empty())
            return vbox({editRow, text(" " + _errorMsg) | color(Color::Red)}) | reflect(_box);
        return editRow | reflect(_box);
    }

    return renderHoverRow() | reflect(_box);
}


CycleField::CycleField(std::string label, std::function<std::string()> getValue,
                       std::function<void()> onCycle,
                       std::shared_ptr<std::function<void()>> cancelEdit,
                       std::function<std::string()> getWarning)
    : FieldBase(std::move(label), std::move(getValue))
    , _onCycle(std::move(onCycle)), _getWarning(std::move(getWarning))
{
    _cancelEdit = std::move(cancelEdit);
}


bool CycleField::OnEvent(Event e) {
    if (Focused() && (e == Event::Return || e == Event::Character(' '))) {
        _onCycle();
        return true;
    }
    if (e.is_mouse() && e.mouse().motion == Mouse::Pressed
            && _box.Contain(e.mouse().x, e.mouse().y) && Focused()) {
        _onCycle();
        return true;
    }
    return FieldBase::OnEvent(e);
}


Element CycleField::OnRender() {
    Element row = renderHoverRow();

    std::string warning = _getWarning ? _getWarning() : "";
    if (!warning.empty())
        return vbox({row, text(" " + warning) | color(Color::Orange1)}) | reflect(_box);

    return row | reflect(_box);
}
