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

class BuilderWindow;

// Explorer action IDs
enum {
    IDE_SWITCH_BUILDER = 4001,
    IDE_OPEN_IMAGE,
    IDE_IMPORT_FILES,
    IDE_EXPORT_FILE,
    IDE_DELETE_ITEM,
    IDE_INJECT_RETURN_APP,
    IDE_REFRESH,
    IDE_BTN_UP,
    IDE_TREEVIEW,
    IDE_LISTVIEW_FILES,
    IDE_TOOLBAR,
    IDE_STATUSBAR,
    IDE_STATIC_PATH
};

class ExplorerWindow {
public:
    ExplorerWindow();
    ~ExplorerWindow();

    bool create(HINSTANCE hInstance, BuilderWindow* builderWin);
    HWND get_hwnd() const { return m_hWnd; }
    void show(const std::wstring& imagePath = L"");
    void hide();

    void open_image(const std::wstring& path);

private:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    BuilderWindow* m_builderWin;

    HWND m_hToolbar;
    HWND m_hStatusBar;
    HWND m_hStaticPath;
    HWND m_hBtnUp;
    HWND m_hTreeView;
    HWND m_hListViewFiles;

    DiskImage m_disk;
    HFSFileSystem m_fs;
    uint32_t m_current_folder_cnid;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void create_controls();
    void layout_controls(int width, int height);
    void update_status_bar();
    void populate_tree();
    void populate_files_list(uint32_t folder_cnid);

    void on_tree_selection_changed();
    void on_drop_files(HDROP hDrop);
    void navigate_to_folder(uint32_t folder_cnid);
    std::wstring get_folder_path(uint32_t cnid) const;

    void action_open_image();
    void action_switch_to_builder();
    void action_inject_current();
    void action_import_files();
    void action_export_selected_file();
    void action_delete_selected();
    void action_refresh();
    void action_preview_file(uint32_t cnid);
};
