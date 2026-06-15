#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <ftxui/component/component.hpp>
#include "../data/TaskStore.h"


/**
 * @brief Focusable detail view for the selected task.
 *
 * Renders fields in order: ID, title, status, priority, due, description,
 * created, deps. Focusable hover fields cover each editable attribute.
 * Field navigation uses j/k or arrow keys.
 */
class DetailPane {
private:
    TaskStore& _store;
    const std::vector<unsigned>& _ids;
    int& _selected;
    std::shared_ptr<std::function<void()>> _cancelEdit;
    ftxui::Component _component;
    ftxui::Component _titleField; // kept for takeFocus()

public:
    /**
     * @param onMutation    Called after any successful field save so the caller
     *                      can rebuild derived state (e.g. list pane labels).
     * @param onManageDeps  Called when the user activates the Deps field to open
     *                      the dependency selector overlay.
     */
    DetailPane(TaskStore& store, const std::vector<unsigned>& ids, int& selected,
               std::function<void()> onMutation,
               std::function<void()> onManageDeps);

    /**
     * @brief Returns the ftxui component to place in the layout.
     * */
    ftxui::Component component() const { return _component; }

    /**
     * @brief Moves focus to the title field.
     * */
    void takeFocus() { _titleField->TakeFocus(); }

    /**
     * @brief Returns true while a text field is being edited.
     * std::function is falsy when empty (no edit), truthy when holding a callable
     * */
    bool isEditing() const { return _cancelEdit && bool(*_cancelEdit); }

    /**
     * @brief Cancels any active field edit, reverting to hover state.
     * */
    void cancelEdit() { if (_cancelEdit && *_cancelEdit) (*_cancelEdit)(); }
};
