#pragma once

#include <vector>
#include <ftxui/component/component.hpp>
#include "../data/TaskStore.h"


/**
 * @brief Focusable detail view for the selected task.
 *
 * Renders a non-focusable header (ID, created, deps) followed by
 * focusable hover fields for each editable attribute.
 * Field navigation uses j/k or arrow keys.
 */
class DetailPane {
private:
    TaskStore& _store;
    const std::vector<unsigned>& _ids;
    int& _selected;
    ftxui::Component _component;
    ftxui::Component _titleField; // kept for takeFocus()

public:
    DetailPane(TaskStore& store, const std::vector<unsigned>& ids, int& selected);

    /** @brief Returns the ftxui component to place in the layout. */
    ftxui::Component component() const { return _component; }

    /** @brief Moves focus to the title field. */
    void takeFocus() { _titleField->TakeFocus(); }

    /** @brief Returns true while a text field is being edited (Phase 3+). */
    bool isEditing() const { return false; }
};
