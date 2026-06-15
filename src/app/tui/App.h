#pragma once

#include <string>
#include <vector>
#include "../data/TaskStore.h"


/**
 * @brief Root application component.
 *
 * Owns TaskStore, builds the two-pane layout, and runs the ftxui event loop.
 */
class App {
private:
    std::string _filePath;
    std::string _swapPath;
    TaskStore _store;
    std::vector<unsigned> _ids;        // cached task IDs in list order; index matches _labels
    std::vector<std::string> _labels; // cached strings shown in the list pane
    int _selected = 0;                 // index of the currently highlighted task
    int _focusedEntry = 0;             // Menu keyboard cursor; must be kept in sync with _selected on external changes
    bool _delDialogOpen = false;       // whether the delete confirmation dialog is visible
    int  _delFocus = 0;                // focused button index within the delete dialog
    bool _recoverDialogOpen = false;   // whether the swap-file recovery prompt is visible
    int  _recoverFocus = 0;            // focused button index within the recovery dialog (0=Recover, 1=Discard)
    bool _quitDialogOpen = false;      // whether the quit confirmation dialog is visible
    int  _quitFocus = 0;               // focused button index within the quit dialog (0=Save, 1=Discard)

    void buildTreeFrom(unsigned id, int depth);

    /** @brief Clears and rebuilds _ids/_labels from the current store state; clamps _selected. */
    void rebuildTree();

    /** @brief Writes the full store to the swap file; silently ignores write errors. */
    void writeSwap();

public:
    explicit App(std::string filePath);

    /** @brief Start the event loop. Blocks until the user quits. */
    void run();
};
