#pragma once

#include <functional>
#include <string>
#include <vector>
#include <ftxui/component/component.hpp>


/**
 * @brief Task list pane with embedded search input and filter bar.
 *
 * Renders a text search input, date/priority/status filter toggles (cycled
 * with f/p/s), and a scrollable Menu of task labels. Filter changes call
 * onChange so the caller can rebuild the displayed list.
 *
 * Key bindings (when menu has focus): j/k navigate, / focuses search,
 * f cycles date filter, p cycles priority filter, s cycles status filter.
 * Escape or Enter from the search input returns focus to the menu.
 */
class ListPane {
private:
    std::string& _searchQuery;
    int& _dateFilter;
    int& _priorityFilter;
    int& _statusFilter;
    bool& _unblockedFilter;
    std::vector<std::string>& _labels;
    int& _selected;
    int& _focusedEntry;
    std::function<void()> _onChange;
    ftxui::Component _component;
    ftxui::Component _menu;
    ftxui::Component _menuWithKeys;
    ftxui::Component _searchInput;

public:
    ListPane(std::string& searchQuery,
                 int& dateFilter,
                 int& priorityFilter,
                 int& statusFilter,
                 bool& unblockedFilter,
                 std::vector<std::string>& labels,
                 int& selected,
                 int& focusedEntry,
                 std::function<void()> onChange);

    ftxui::Component getComponent() const { return _component; }

    /** @brief Gives keyboard focus to the task menu (not the search input). */
    void takeFocus() { _menuWithKeys->TakeFocus(); }

    /** @brief Returns true while the search input has keyboard focus. */
    bool isSearchFocused() const { return _searchInput && _searchInput->Focused(); }
};
