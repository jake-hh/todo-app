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
    bool _recoverDialogOpen = false;   // whether the swap-file recovery prompt is visible
    bool _quitDialogOpen = false;      // whether the quit confirmation dialog is visible

    // Search / filter state (0 / -1 / false = no filter active)
    std::string _searchQuery;
    int  _dateFilter      = 0;     // 0=all, 1=overdue, 2=today, 3=this-week, 4=no-date
    int  _priorityFilter  = -1;    // -1=all, 0-3=specific priority
    int  _statusFilter    = -1;    // -1=all, 0-3=specific status
    bool _unblockedFilter = false; // false=all, true=unblocked only
    std::string _saveError;         // non-empty if save failed; printed to stderr after screen exits

    /** @brief Recursively appends @p id and its deps to _ids/_labels at @p depth. */
    void buildTreeFromTask(unsigned id, int depth);

    /** @brief Clears and rebuilds _ids/_labels from the current store state; clamps _selected. */
    void rebuildTree();

    /** @brief Writes the full store to the swap file; silently ignores write errors. */
    void writeSwap();

public:
    /**
     * @brief Constructs App and loads tasks from disk.
     * @param filePath Path to the binary task file (created on first save).
     */
    explicit App(std::string filePath);

    /** @brief Start the event loop. Blocks until the user quits. */
    void run();
};
