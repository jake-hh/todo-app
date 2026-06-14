#pragma once

#include <functional>
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
 * @brief Focusable text field. Highlights when focused.
 *
 * Phase 3 will add edit mode: Enter activates an Input, Esc discards,
 * Enter again saves to the store.
 */
class TextField : public FieldBase {
private:
    std::string _label;
    std::function<std::string()> _getValue;

public:
    TextField(std::string label, std::function<std::string()> getValue);
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
