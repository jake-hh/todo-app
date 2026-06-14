#include "DetailPane.h"

#include <algorithm>
#include <string>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "FieldComponents.h"


DetailPane::DetailPane(TaskStore& store,
                       const std::vector<unsigned>& ids,
                       int& selected)
    : _store(store), _ids(ids), _selected(selected)
{
    using namespace ftxui;

    // Safe task lookup — returns nullptr when the list is empty.
    auto getTask = [this]() -> const Task* {
        if (_ids.empty()) return nullptr;
        int sel = std::max(0, std::min(_selected, static_cast<int>(_ids.size()) - 1));
        return &_store.get(_ids[sel]);
    };

    // Non-focusable header: read-only fields that cannot be edited.
    auto header = Renderer([getTask] {
        const Task* t = getTask();
        if (!t) return text(" (no tasks)");

        std::string deps;
        if (t->deps.isEmpty()) {
            deps = "none";
        } else {
            for (size_t i = 0; i < t->deps.size(); i++) {
                if (i > 0) deps += ", ";
                deps += std::to_string(t->deps[i]);
            }
        }

        auto row = [](const std::string& label, const std::string& val) -> Element {
            return hbox({text(label) | bold, text(val)});
        };

        return vbox({
            row(" ID: ",      std::to_string(t->id)),
            row(" Created: ", t->createdAtStr()),
            row(" Deps: ",    deps),
        });
    });

    auto title_f    = Make<TextField>(" Title: ",       [getTask]() -> std::string { const Task* t = getTask(); return t ? t->title        : ""; });
    auto status_f   = Make<CycleField>(" Status: ",     [getTask]() -> std::string { const Task* t = getTask(); return t ? t->statusStr()  : ""; });
    auto priority_f = Make<CycleField>(" Priority: ",   [getTask]() -> std::string { const Task* t = getTask(); return t ? t->priorityStr(): ""; });
    auto duedate_f  = Make<TextField>(" Due: ",         [getTask]() -> std::string { const Task* t = getTask(); return t ? t->dueDateStr() : ""; });
    auto desc_f     = Make<TextField>(" Description: ", [getTask]() -> std::string { const Task* t = getTask(); return t ? t->description  : ""; });

    _titleField = title_f;

    auto container = Container::Vertical({
        header,
        title_f,
        status_f,
        priority_f,
        duedate_f,
        desc_f,
    });

    // Remap j/k to arrow keys so vim-style navigation works within the pane.
    _component = CatchEvent(container, [container](Event e) {
        if (e == Event::Character('j')) return container->OnEvent(Event::ArrowDown);
        if (e == Event::Character('k')) return container->OnEvent(Event::ArrowUp);
        return false;
    });
}
