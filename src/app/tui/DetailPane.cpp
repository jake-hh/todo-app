#include "DetailPane.h"

#include <algorithm>
#include <memory>
#include <string>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "FieldComponents.h"
#include "../data/Task.h"


DetailPane::DetailPane(TaskStore& store,
                       const std::vector<unsigned>& ids,
                       int& selected,
                       std::function<void()> onMutation)
    : _store(store), _ids(ids), _selected(selected)
    , _cancelEdit(std::make_shared<std::function<void()>>())
{
    using namespace ftxui;

    auto ef = _cancelEdit;

    // Safe task lookup — returns nullptr when the list is empty.
    auto getTask = [&store = _store, &ids = _ids, &sel = _selected]() -> const Task* {
        if (ids.empty()) return nullptr;
        int i = std::max(0, std::min(sel, static_cast<int>(ids.size()) - 1));
        return &store.get(ids[i]);
    };

    auto row = [](const std::string& label, const std::string& val) -> Element {
        return hbox({text(label) | bold, text(val)});
    };

    // Non-focusable read-only fields.
    auto id_header = Renderer([getTask, row] {
        const Task* t = getTask();
        if (!t) return text(" (no tasks)");
        return row(" ID: ", std::to_string(t->id));
    });

    auto footer = Renderer([getTask, row] {
        const Task* t = getTask();
        if (!t) return text("");

        std::string deps;
        if (t->deps.isEmpty()) {
            deps = "none";
        } else {
            for (size_t i = 0; i < t->deps.size(); i++) {
                if (i > 0) deps += ", ";
                deps += std::to_string(t->deps[i]);
            }
        }

        return vbox({
            row(" Created: ", t->createdAtStr()),
            row(" Deps: ",    deps),
        });
    });

    // Setter helpers — capture refs and onMutation by value.
    auto setTitle = [&store = _store, &ids = _ids, &sel = _selected, onMutation]
                    (const std::string& val) -> bool {
        if (val.empty()) return false;
        if (ids.empty()) return false;
        int i = std::max(0, std::min(sel, static_cast<int>(ids.size()) - 1));
        Task updated = store.get(ids[i]);
        updated.title = val;
        store.update(updated.id, updated);
        onMutation();
        return true;
    };

    auto setDesc = [&store = _store, &ids = _ids, &sel = _selected, onMutation]
                   (const std::string& val) -> bool {
        if (ids.empty()) return false;
        int i = std::max(0, std::min(sel, static_cast<int>(ids.size()) - 1));
        Task updated = store.get(ids[i]);
        updated.description = val;
        store.update(updated.id, updated);
        onMutation();
        return true;
    };

    auto setDueDate = [&store = _store, &ids = _ids, &sel = _selected, onMutation]
                      (const std::string& val) -> bool {
        auto parsed = parseDueDate(val);
        if (!parsed) return false;
        if (ids.empty()) return false;
        int i = std::max(0, std::min(sel, static_cast<int>(ids.size()) - 1));
        Task updated = store.get(ids[i]);
        updated.dueDate = *parsed;
        store.update(updated.id, updated);
        onMutation();
        return true;
    };

    // For due date: edit buffer starts as "" (not "none") when there is no due date.
    auto getDueDateForEdit = [getTask]() -> std::string {
        const Task* t = getTask();
        if (!t || t->dueDate == -1) return "";
        return t->dueDateStr();
    };

    auto title_f    = Make<TextField>(" Title: ",
                          [getTask]() -> std::string { const Task* t = getTask(); return t ? t->title        : ""; },
                          setTitle, "Title cannot be empty", ef);

    auto cycleStatus = [&store = _store, &ids = _ids, &sel = _selected, onMutation]() {
        if (ids.empty()) return;
        int i = std::max(0, std::min(sel, static_cast<int>(ids.size()) - 1));
        Task updated = store.get(ids[i]);
        updated.status = (updated.status + 1) % 4;
        store.update(updated.id, updated);
        onMutation();
    };

    auto cyclePriority = [&store = _store, &ids = _ids, &sel = _selected, onMutation]() {
        if (ids.empty()) return;
        int i = std::max(0, std::min(sel, static_cast<int>(ids.size()) - 1));
        Task updated = store.get(ids[i]);
        updated.priority = (updated.priority + 1) % 4;
        store.update(updated.id, updated);
        onMutation();
    };

    auto status_f   = Make<CycleField>(" Status: ",
                          [getTask]() -> std::string { const Task* t = getTask(); return t ? t->statusStr()  : ""; },
                          cycleStatus, ef);

    auto priority_f = Make<CycleField>(" Priority: ",
                          [getTask]() -> std::string { const Task* t = getTask(); return t ? t->priorityStr(): ""; },
                          cyclePriority, ef);

    auto duedate_f  = Make<TextField>(" Due: ",
                          [getTask]() -> std::string { const Task* t = getTask(); return t ? t->dueDateStr() : ""; },
                          setDueDate, "Invalid date", ef, getDueDateForEdit);

    auto desc_f     = Make<TextField>(" Description: ",
                          [getTask]() -> std::string { const Task* t = getTask(); return t ? t->description  : ""; },
                          setDesc, "", ef);

    _titleField = title_f;

    auto container = Container::Vertical({
        id_header,
        title_f,
        status_f,
        priority_f,
        duedate_f,
        desc_f,
        footer,
    });

    // Remap j/k to arrow keys only when not in edit mode.
    // In edit mode, j/k must pass through as characters to the active TextField.
    _component = CatchEvent(container, [container, ef](Event e) {
        if (*ef) return false;  // *ef is std::function — truthy when an edit is active
        if (e == Event::Character('j')) return container->OnEvent(Event::ArrowDown);
        if (e == Event::Character('k')) return container->OnEvent(Event::ArrowUp);
        return false;
    });
}
