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
    int focus = 0;                              ///< Index of the focused button.
    int mouseX = -1, mouseY = -1;              ///< Last known mouse position for hover tracking.
    std::vector<ftxui::Box> boxes;             ///< Screen boxes for each button (populated by render).
    std::vector<std::string> labels;           ///< Button label strings.
    std::vector<std::function<void()>> actions; ///< Per-button callbacks invoked by trigger().

    /**
     * @brief Configures and opens the dialog.
     * @param lbls         Button labels shown left-to-right.
     * @param acts         Callbacks indexed in parallel with @p lbls.
     * @param defaultFocus Index of the button that starts focused.
     */
    void open(std::vector<std::string> lbls,
              std::vector<std::function<void()>> acts,
              int defaultFocus = 0);

    /**
     * @brief Renders the dialog frame with @p body in the content area.
     * @param body  Elements rendered inside the dialog border above the buttons.
     * @returns An ftxui::Element suitable for overlay composition.
     */
    ftxui::Element render(ftxui::Elements body);

    /**
     * @brief Invokes the callback at @p idx.
     * @param idx Button index (0-based).
     */
    void trigger(int idx);

    /**
     * @brief Handles navigation (arrows, tab) and activation (Return, click).
     * Caller must return true to consume the event regardless.
     */
    void onEvent(ftxui::Event e);
};
