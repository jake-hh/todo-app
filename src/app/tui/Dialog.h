#pragma once

#include <functional>
#include <string>
#include <vector>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>


/**
 * @brief Generic modal dialog with keyboard and mouse button support.
 *
 * Call open() to configure button labels and per-button callbacks. Then call
 * render() from the renderer and onEvent() from the event handler each frame.
 * The caller is responsible for showing/hiding the dialog and handling Escape.
 */
struct Dialog {
    int focus = 0;
    int mouseX = -1, mouseY = -1;  // last known mouse position for hover tracking
    std::vector<ftxui::Box> boxes;
    std::vector<std::string> labels;
    std::vector<std::function<void()>> actions;

    void open(std::vector<std::string> lbls,
              std::vector<std::function<void()>> acts,
              int defaultFocus = 0);

    ftxui::Element render(ftxui::Elements body);

    void trigger(int idx);

    // Handles navigation (arrows, tab) and activation (Return, click).
    // Caller must return true to consume the event regardless.
    void onEvent(ftxui::Event e);
};
