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
#include <string>
#include <vector>
#include "hfs_filesystem.h"

class ExplorerWindow;

struct BuilderConfig {
    std::wstring image_path;
    int target_hardware = 1;      // 0: Mac Plus (68000), 1: Mac SE (ADB/XPRAM), 2: Mac SE/30 (68030)
    int boot_pref = 2;            // 0: Shell, 1: Desktop, 2: Boot Menu, 3: Direct Mac OS
    int direct_mac_id = 3;        // 0..7 (Mac OS SCSI target ID)
    int timeout = 5;              // 0..30 seconds
    int default_entry_idx = 0;    // Default selection in boot menu
    int theme = 2;                // 0: Mac OS 7.1, 1: Amiga WB, 2: Win95, 3: Win98, 4: Aero, 5: Hacker
    int wallpaper = 0;            // 0: Classic Gray, 1: Matrix Digital Rain
    std::wstring greeting;        // Welcome banner
    int return_scsi_id = 6;       // Target SCSI ID for "Return to PlusOS"
    bool bundle_games = true;
    bool bundle_dev = true;
    bool bundle_tools = true;
    std::vector<MultiBootEntry> boot_entries;
};

// Builder control IDs
enum {
    IDB_HARDWARE_COMBO = 3001,
    IDB_IMAGE_PATH_EDIT,
    IDB_IMAGE_BROWSE_BTN,
    IDB_BOOT_MODE_COMBO,
    IDB_DIRECT_MAC_COMBO,
    IDB_TIMEOUT_EDIT,
    IDB_BOOT_LISTVIEW,
    IDB_BOOT_ADD_BTN,
    IDB_BOOT_EDIT_BTN,
    IDB_BOOT_REMOVE_BTN,
    IDB_BOOT_UP_BTN,
    IDB_BOOT_DOWN_BTN,
    IDB_THEME_COMBO,
    IDB_WALLPAPER_COMBO,
    IDB_GREETING_EDIT,
    IDB_RETURN_SCSI_COMBO,
    IDB_CHECK_GAMES,
    IDB_CHECK_DEV,
    IDB_CHECK_TOOLS,
    IDB_BUILD_BTN,
    IDB_OPEN_EXPLORER_BTN,
    IDB_SAVE_CFG_BTN,
    IDB_INJECT_BTN,
    IDB_BUILD_LOG_EDIT
};

class BuilderWindow {
public:
    BuilderWindow();
    ~BuilderWindow();

    bool create(HINSTANCE hInstance, int nCmdShow, ExplorerWindow* explorerWin);
    HWND get_hwnd() const { return m_hWnd; }
    void show();
    void hide();

    void load_config_from_file();
    bool save_config_to_file();
    void append_log(const std::wstring& text);
    void action_build_image();
    void action_open_explorer();

private:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    ExplorerWindow* m_explorerWin;
    BuilderConfig m_cfg;

    // Controls
    HWND m_hComboHardware;
    HWND m_hEditImagePath;
    HWND m_hBtnBrowseImage;
    HWND m_hStaticBlueSCSI;

    HWND m_hComboBootMode;
    HWND m_hComboDirectMacID;
    HWND m_hEditTimeout;
    HWND m_hUpDownTimeout;
    HWND m_hListViewBoot;
    HWND m_hBtnAddOS;
    HWND m_hBtnEditOS;
    HWND m_hBtnRemoveOS;
    HWND m_hBtnUpOS;
    HWND m_hBtnDownOS;

    HWND m_hComboTheme;
    HWND m_hComboWallpaper;
    HWND m_hEditGreeting;
    HWND m_hComboReturnSCSI;
    HWND m_hCheckGames;
    HWND m_hCheckDev;
    HWND m_hCheckTools;

    HWND m_hBtnBuildImage;
    HWND m_hBtnOpenExplorer;
    HWND m_hBtnSaveConfig;
    HWND m_hBtnInjectApp;
    HWND m_hEditBuildLog;
    HFONT m_hConsoleFont;
    HFONT m_hBoldFont;
    HFONT m_hHeaderFont;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void create_controls();
    void layout_controls(int width, int height);
    void update_controls_from_cfg();
    void update_cfg_from_controls();

    void populate_boot_list();
    void on_boot_add();
    void on_boot_edit();
    void on_boot_remove();
    void on_boot_up();
    void on_boot_down();

    void action_browse_image();
    void action_inject_external();
};
