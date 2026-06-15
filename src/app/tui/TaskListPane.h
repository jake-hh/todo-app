#pragma once

#include <string>
#include <vector>
#include <ftxui/component/component.hpp>


/**
 * @brief Creates a scrollable task list component with keyboard navigation.
 * @param labels Display strings for each task (shared with caller).
 *                Non-const because ftxui::Menu takes a raw non-const pointer internally.
 * @param selected Index of the currently selected task (shared state).
 * @return ftxui component handling arrow keys and j/k navigation.
 */
ftxui::Component MakeTaskListPane(std::vector<std::string>& labels, int& selected, int& focusedEntry);
