#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include "disk_image.h"
#include "hfs_filesystem.h"

// Custom window messages and control IDs
enum {
    ID_FILE_OPEN_IMAGE = 1001,
    ID_FILE_CLOSE_IMAGE,
    ID_FILE_EXIT,
    ID_ACTION_INJECT_CURRENT,
    ID_ACTION_INJECT_EXTERNAL,
    ID_ACTION_IMPORT_FILES,
    ID_ACTION_EXPORT_FILE,
    ID_ACTION_DELETE,
    ID_ACTION_NEW_FOLDER,
    ID_ACTION_REFRESH,
    ID_ACTION_BUILD_IMAGE,
    ID_ACTION_START_OPTIONS,
    ID_VIEW_HEX,
    ID_VIEW_TEXT,
    ID_HELP_ABOUT,

    // Controls
    IDC_TOOLBAR = 2001,
    IDC_STATUSBAR,
    IDC_TREEVIEW,
    IDC_TABCONTROL,
    IDC_STATIC_PATH,
    IDC_BTN_UP,
    IDC_LISTVIEW_FILES,
    IDC_LISTVIEW_BOOT,
    IDC_BTN_BOOT_ADD,
    IDC_BTN_BOOT_EDIT,
    IDC_BTN_BOOT_REMOVE,
    IDC_BTN_BOOT_UP,
    IDC_BTN_BOOT_DOWN,
    IDC_BTN_BOOT_SAVE,
    IDC_COMBO_BOOT_PREF,
    IDC_EDIT_DIAGNOSTICS
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(HINSTANCE hInstance, int nCmdShow);
    int run();

    void open_image(const std::wstring& path);
    void show_start_options_dialog();
    void action_build_image();

private:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    HWND m_hToolbar;
    HWND m_hStatusBar;
    HWND m_hTreeView;
    HWND m_hTabControl;
    HWND m_hStaticPath;
    HWND m_hBtnUp;
    HWND m_hListViewFiles;
    HWND m_hListViewBoot;
    HWND m_hBtnBootAdd;
    HWND m_hBtnBootEdit;
    HWND m_hBtnBootRemove;
    HWND m_hBtnBootUp;
    HWND m_hBtnBootDown;
    HWND m_hBtnBootSave;
    HWND m_hComboBootPref;
    HWND m_hEditDiagnostics;

    DiskImage m_disk;
    HFSFileSystem m_fs;
    uint32_t m_current_folder_cnid;
    int m_current_tab;
    std::vector<MultiBootEntry> m_boot_entries;
    int m_boot_pref;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void create_controls();
    void layout_controls(int width, int height);
    void update_status_bar();
    void populate_tree();
    void populate_files_list(uint32_t folder_cnid);
    void populate_boot_tab();
    void populate_diagnostics_tab();

    void on_tree_selection_changed();
    void on_tab_changed();
    void on_drop_files(HDROP hDrop);

    void navigate_to_folder(uint32_t folder_cnid);
    std::wstring get_folder_path(uint32_t cnid) const;

    void action_open_image();
    void action_inject_current();
    void action_inject_external();
    void action_import_files();
    void action_export_selected_file();
    void action_delete_selected();
    void action_new_folder();
    void action_refresh();
    void action_view_file(bool hex_mode);

    void boot_add_entry();
    void boot_edit_entry();
    void boot_remove_entry();
    void boot_move_up();
    void boot_move_down();
    void boot_save_config();
};
