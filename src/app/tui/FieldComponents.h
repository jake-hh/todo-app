#pragma once

#include <functional>
#include <memory>
#include <string>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>


/**
 * @brief Base for all detail-pane field components.
 *
 * Handles focusability and click-to-focus so subclasses only need to
 * implement OnRender. Calls the shared cancelEdit hook before taking focus
 * so that any active TextField edit is discarded before the new field gains
 * focus — regardless of DOM order.
 */
class FieldBase : public ftxui::ComponentBase {
protected:
    ftxui::Box _box;
    std::shared_ptr<std::function<void()>> _cancelEdit;
    std::string _label;
    std::function<std::string()> _getValue;

    ftxui::Element renderHoverRow() const;

    FieldBase(std::string label, std::function<std::string()> getValue)
        : _label(std::move(label)), _getValue(std::move(getValue)) {}

public:
    bool Focusable() const override { return true; }
    bool OnEvent(ftxui::Event e) override;
};


/**
 * @brief Focusable text field with hover and edit states.
 *
 * Hover: highlights when focused. Enter activates edit mode.
 * Edit: shows manual cursor with gray background. Enter saves via trySetValue,
 * Esc discards. All keys are consumed in edit mode to block global shortcuts.
 */
class TextField : public FieldBase {
private:
    std::function<bool(const std::string&)> _trySetValue;
    std::string _errorMsg;
    std::function<std::string()> _getEditValue;

    bool _editing = false;
    std::string _editStr;
    int _cursor = 0;
    bool _showError = false;

    void enterEdit();
    void confirmEdit();
    void discardEdit();

public:
    /**
     * @param label       Field label shown before the value.
     * @param getValue    Returns current store value for display.
     * @param trySetValue Called on Enter; returns false to reject and stay in edit.
     * @param errorMsg    Shown in red below the field on rejection.
     * @param cancelEdit   Shared hook; set to this field's discardEdit while editing,
     *                     cleared on confirm/discard. FieldBase::OnEvent calls it before
     *                     TakeFocus so DOM-order-independent cancel works correctly.
     * @param getEditValue Optional; returns initial edit buffer content. Defaults to getValue.
     */
    TextField(std::string label,
              std::function<std::string()> getValue,
              std::function<bool(const std::string&)> trySetValue,
              std::string errorMsg,
              std::shared_ptr<std::function<void()>> cancelEdit,
              std::function<std::string()> getEditValue = {});

    bool OnEvent(ftxui::Event e) override;

    ftxui::Element OnRender() override;
};


/**
 * @brief Focusable cycle field. Highlights when focused.
 *
 * Enter advances through the field's allowed values and saves to the store
 * immediately via onCycle.
 */
class CycleField : public FieldBase {
private:
    std::function<void()> _onCycle;
    std::function<std::string()> _getWarning; ///< Returns warning text, or "" for none.

public:
    CycleField(std::string label, std::function<std::string()> getValue,
               std::function<void()> onCycle,
               std::shared_ptr<std::function<void()>> cancelEdit,
               std::function<std::string()> getWarning = {});

    bool OnEvent(ftxui::Event e) override;
    ftxui::Element OnRender() override;
};
