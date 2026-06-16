#include "TaskListPane.h"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>


static const char* DATE_LABELS[]     = {"f:all", "f:ovr", "f:tod", "f:wk", "f:no-dt"};
static const char* PRIORITY_LABELS[] = {"p:all", "p:wish", "p:low", "p:med", "p:hi"};
static const char* STATUS_LABELS[]   = {"s:all", "s:open", "s:prog", "s:done", "s:wonx"};

static constexpr int DATE_COUNT     = 5;
static constexpr int FILTER_CYCLE   = 5; // -1,0,1,2,3 → next = (v+2)%5-1


TaskListPane::TaskListPane(
    std::string& searchQuery,
    int& dateFilter,
    int& priorityFilter,
    int& statusFilter,
    std::vector<std::string>& labels,
    int& selected,
    int& focusedEntry,
    std::function<void()> onChange)
    : _searchQuery(searchQuery)
    , _dateFilter(dateFilter)
    , _priorityFilter(priorityFilter)
    , _statusFilter(statusFilter)
    , _labels(labels)
    , _selected(selected)
    , _focusedEntry(focusedEntry)
    , _onChange(std::move(onChange))
{
    using namespace ftxui;

    InputOption inputOpt;
    inputOpt.on_change = _onChange;
    _searchInput = Input(&_searchQuery, "search...", inputOpt);

    MenuOption menuOpt = MenuOption::Vertical();
    menuOpt.focused_entry = &_focusedEntry;
    _menu = Menu(&_labels, &_selected, menuOpt);

    auto scrollable = Renderer(_menu, [this] {
        if (_labels.empty())
            return text(" (no tasks)") | color(Color::GrayDark) | flex;
        return _menu->Render() | vscroll_indicator | yframe | flex;
    });

    // j/k vim nav + filter cycling + / to focus search
    _menuWithKeys = CatchEvent(scrollable, [this](Event e) {
        if (e == Event::Character('j')) return _menu->OnEvent(Event::ArrowDown);
        if (e == Event::Character('k')) return _menu->OnEvent(Event::ArrowUp);
        if (e == Event::Character('/')) { _searchInput->TakeFocus(); return true; }
        if (e == Event::Character('f')) {
            _dateFilter = (_dateFilter + 1) % DATE_COUNT;
            _onChange();
            return true;
        }
        if (e == Event::Character('p')) {
            _priorityFilter = (_priorityFilter + 2) % FILTER_CYCLE - 1;
            _onChange();
            return true;
        }
        if (e == Event::Character('s')) {
            _statusFilter = (_statusFilter + 2) % FILTER_CYCLE - 1;
            _onChange();
            return true;
        }
        return false;
    });

    auto container = Container::Vertical({_searchInput, _menuWithKeys});

    // Custom renderer: filter bar on top, separator, then the scrollable list
    auto withBar = Renderer(container, [this] {
        bool df = _dateFilter != 0;
        bool pf = _priorityFilter >= 0;
        bool sf = _statusFilter >= 0;

        auto bar = hbox({
            text("/") | color(Color::GrayDark),
            _searchInput->Render() | flex,
            text("  "),
            text(DATE_LABELS[_dateFilter])          | (df ? bold : dim),
            text("  "),
            text(PRIORITY_LABELS[_priorityFilter + 1]) | (pf ? bold : dim),
            text("  "),
            text(STATUS_LABELS[_statusFilter + 1])  | (sf ? bold : dim),
        });

        Element list;
        if (_labels.empty())
            list = text(" (no tasks)") | color(Color::GrayDark) | flex;
        else
            list = _menu->Render() | vscroll_indicator | yframe | flex;

        return vbox({bar, separator(), list});
    });

    // Escape or Enter from search input → return focus to menu
    _component = CatchEvent(withBar, [this](Event e) {
        if (_searchInput->Focused()) {
            if (e == Event::Escape || e == Event::Return) {
                _menuWithKeys->TakeFocus();
                return true;
            }
        }
        return false;
    });
}
