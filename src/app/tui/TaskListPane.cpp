#include "TaskListPane.h"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>


ftxui::Component MakeTaskListPane(std::vector<std::string>& labels, int& selected, int& focusedEntry) {
    using namespace ftxui;

    // Menu handles arrow key navigation and updates `selected` in place.
    // It takes raw pointers so changes to `labels` are reflected immediately.
    // focused_entry tracks the keyboard cursor independently of the selected item,
    // allowing the caller to sync both when changing selection externally (e.g. 'n').
    MenuOption opt = MenuOption::Vertical();
    opt.focused_entry = &focusedEntry;
    auto menu = Menu(&labels, &selected, opt);

    // Wrap in a Renderer that applies yframe so the selected entry scrolls
    // into view when the pane is shorter than the full list.
    auto scrollable = Renderer(menu, [menu] {
        return menu->Render() | vscroll_indicator | yframe | flex;
    });

    // CatchEvent intercepts j/k before Menu sees them and re-dispatches as
    // arrow keys, adding vim-style navigation without replacing the default behaviour.
    return CatchEvent(scrollable, [menu](Event e) {
        if (e == Event::Character('j')) return menu->OnEvent(Event::ArrowDown);
        if (e == Event::Character('k')) return menu->OnEvent(Event::ArrowUp);
        return false;
    });
}
