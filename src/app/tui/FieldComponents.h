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
    ftxui::Box _box;                                    ///< Screen box updated each render for click-to-focus.
    std::shared_ptr<std::function<void()>> _cancelEdit; ///< Shared hook; set by the active TextField, cleared on save/discard.
    std::string _label;                                 ///< Text shown to the left of the value.
    std::function<std::string()> _getValue;             ///< Returns the current store value for display.

    /** @brief Renders the label + value row with focus highlight. */
    ftxui::Element renderHoverRow() const;

    /**
     * @param label    Text shown to the left of the value.
     * @param getValue Returns the current store value for display.
     */
    FieldBase(std::string label, std::function<std::string()> getValue)
        : _label(std::move(label)), _getValue(std::move(getValue)) {}

public:
    /** @returns true — all field types are keyboard-focusable. */
    bool Focusable() const override { return true; }

    /** @brief Handles click-to-focus; subclasses call this via override. */
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
    std::function<bool(const std::string&)> _trySetValue; ///< Returns false to reject and stay in edit mode.
    std::string _errorMsg;                                 ///< Shown in red below the field on rejection.
    std::function<std::string()> _getEditValue;            ///< Returns initial edit buffer; defaults to _getValue.

    bool _editing = false;       ///< True while in edit mode.
    std::string _editStr;        ///< In-progress edit buffer.
    int _cursor = 0;             ///< Byte offset of the text cursor in _editStr.
    bool _showError = false;     ///< True after a rejected trySetValue until the next keypress.

    /** @brief Enters edit mode; populates _editStr from _getEditValue and sets the cancel hook. */
    void enterEdit();

    /** @brief Calls trySetValue; stays in edit mode and shows error on failure. */
    void confirmEdit();

    /** @brief Exits edit mode, discards _editStr, and clears the cancel hook. */
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

    /** @brief Handles edit-mode key events; delegates to FieldBase for click-to-focus. */
    bool OnEvent(ftxui::Event e) override;

    /** @brief Renders hover or edit state depending on _editing. */
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
    std::function<void()> _onCycle;            ///< Advances and saves the value on Enter/Space/click.
    std::function<std::string()> _getWarning; ///< Returns warning text, or "" for none.

public:
    /**
     * @param label      Field label shown before the value.
     * @param getValue   Returns the current store value for display.
     * @param onCycle    Called on Enter to advance and save the value.
     * @param cancelEdit Shared cancel hook; called before this field takes focus.
     * @param getWarning Optional; returns a warning string shown below the field, or "" for none.
     */
    CycleField(std::string label, std::function<std::string()> getValue,
               std::function<void()> onCycle,
               std::shared_ptr<std::function<void()>> cancelEdit,
               std::function<std::string()> getWarning = {});

    /** @brief Handles Enter to cycle the value; delegates to FieldBase for click-to-focus. */
    bool OnEvent(ftxui::Event e) override;

    /** @brief Renders the hover row with an optional warning line below. */
    ftxui::Element OnRender() override;
};
