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
 * implement OnRender. Phase 3/4 subclasses will extend OnEvent with
 * keyboard behaviour (edit mode, cycling).
 */
class FieldBase : public ftxui::ComponentBase {
protected:
    ftxui::Box _box;

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
    std::string _label;
    std::function<std::string()> _getValue;
    std::function<bool(const std::string&)> _trySetValue;
    std::string _errorMsg;
    std::shared_ptr<bool> _editingFlag;
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
     * @param editingFlag Shared flag set true while this field is in edit mode.
     * @param getEditValue Optional; returns initial edit buffer content. Defaults to getValue.
     */
    TextField(std::string label,
              std::function<std::string()> getValue,
              std::function<bool(const std::string&)> trySetValue,
              std::string errorMsg,
              std::shared_ptr<bool> editingFlag,
              std::function<std::string()> getEditValue = {});

    bool OnEvent(ftxui::Event e) override;
    ftxui::Element OnRender() override;
};


/**
 * @brief Focusable cycle field. Highlights when focused.
 *
 * Phase 4 will add Enter-to-cycle behaviour that advances through the
 * field's allowed values and saves to the store immediately.
 */
class CycleField : public FieldBase {
private:
    std::string _label;
    std::function<std::string()> _getValue;

public:
    CycleField(std::string label, std::function<std::string()> getValue);
    ftxui::Element OnRender() override;
};
