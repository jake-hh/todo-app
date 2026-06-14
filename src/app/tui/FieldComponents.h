#pragma once

#include <functional>
#include <string>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>


/**
 * @brief Focusable text field. Highlights when focused.
 *
 * Phase 3 will add edit mode: Enter activates an Input, Esc discards,
 * Enter again saves to the store.
 */
class TextField : public ftxui::ComponentBase {
private:
    std::string _label;
    std::function<std::string()> _getValue;

public:
    TextField(std::string label, std::function<std::string()> getValue);

    bool Focusable() const override { return true; }
    ftxui::Element OnRender() override;
};


/**
 * @brief Focusable cycle field. Highlights when focused.
 *
 * Phase 4 will add Enter-to-cycle behaviour that advances through the
 * field's allowed values and saves to the store immediately.
 */
class CycleField : public ftxui::ComponentBase {
private:
    std::string _label;
    std::function<std::string()> _getValue;

public:
    CycleField(std::string label, std::function<std::string()> getValue);

    bool Focusable() const override { return true; }
    ftxui::Element OnRender() override;
};
