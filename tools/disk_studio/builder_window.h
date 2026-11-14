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
#include <set>
#include <map>
#include "hfs_filesystem.h"

#define WM_BUILD_LOG_LINE    (WM_USER + 101)
#define WM_BUILD_COMPLETED   (WM_USER + 102)
#define WM_FLOPPY_COMPLETED  (WM_USER + 103)
#define WM_SETUP_COMPLETED   (WM_USER + 104)

// Build toolchain state: PlusOS builds exclusively with its own native P68 toolchain
// (toolchain\bin\p68-cc.exe, p68-as.exe, p68-ld.exe, p68-ar.exe, p68-make.exe). No WSL,
// no MSYS2, no Linux. The tools are built once with MSVC (toolchain\build.cmd).
enum BuildBackend {
    BB_NONE = 0,       // the P68 tools are not built yet — run Setup (toolchain\build.cmd)
    BB_P68_NATIVE,     // toolchain\bin\*.exe present and ready
};

class ExplorerWindow;

struct DiscoveredApp {
    std::string name;
    std::wstring path;
    uint64_t size = 0;
};

// A source-tree app: apps/<category>/<name>/. Gathered by scan_source_directory.
struct SourceApp {
    std::string name;         // uppercase, as installed into bin/
    std::string category;     // "System", "Tools", "Games", "Dev", "Network"
    std::wstring source_dir;  // absolute path to apps/<cat>/<name>/
};

// Optional kernel modules (must match KMODS_ALL in the Makefile / kernel/core/kconfig.h)
enum { KMOD_COUNT = 11 };

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
    std::set<std::string> included_apps;
    std::map<std::string, std::wstring> custom_apps;
    std::vector<MultiBootEntry> boot_entries;

    // Kernel build settings (saved to kernel.cfg, read by the Makefile)
    int  kernel_profile = 0;      // 0: full, 1: lean, 2: fast, 3: tiny, 4: custom
    bool kmod_on[KMOD_COUNT];     // which optional kernel modules are built in
    BuilderConfig() { for (bool& b : kmod_on) b = true; }
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
    IDB_APPS_LISTVIEW,
    IDB_APPS_SELECT_ALL_BTN,
    IDB_APPS_SELECT_NONE_BTN,
    IDB_APPS_SYS_ONLY_BTN,
    IDB_APPS_GAMES_ONLY_BTN,
    IDB_APPS_ADD_BTN,
    IDB_APPS_REMOVE_BTN,
    IDB_BASE_SYSTEM_STATIC,
    IDB_APPS_COUNT_STATIC,
    IDB_TOTAL_DISK_STATIC,
    IDB_APPS_FLOPPY_STATUS_STATIC,
    IDB_APPS_FLOPPY_TIP_STATIC,
    IDB_BUILD_BTN,
    IDB_UPDATE_BTN,
    IDB_PUSH_UPDATE_BTN,
    IDB_BUILD_FLOPPY_BTN,
    IDB_OPEN_EXPLORER_BTN,
    IDB_SAVE_CFG_BTN,
    IDB_INJECT_BTN,
    IDB_SETUP_ENV_BTN,
    IDB_BOOT_VM_BTN,
    IDB_BUILD_LOG_EDIT,
    IDB_KPROFILE_COMBO,
    IDB_KMOD_FIRST,                       // 11 checkboxes, one per optional kernel module
    IDB_KMOD_LAST = IDB_KMOD_FIRST + 10
};

class BuilderWindow {
public:
    BuilderWindow();
    ~BuilderWindow();

    bool create(HINSTANCE hInstance, int nCmdShow, ExplorerWindow* explorerWin);
    HWND get_hwnd() const { return m_hWnd; }
    std::wstring get_repo_root() const { return m_repoRoot; }
    void show();
    void hide();

    void load_config_from_file();
    bool save_config_to_file();
    void append_log(const std::wstring& text);
    void action_build_image();
    void action_update_image();
    void action_build_floppy();
    void action_push_update();        // admin: compile + publish a release to the OTA server
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

    HWND m_hListViewApps;
    HWND m_hBtnAppsSelectAll;
    HWND m_hBtnAppsSelectNone;
    HWND m_hBtnAppsSysOnly;
    HWND m_hBtnAppsGamesOnly;
    HWND m_hBtnAppsAdd;
    HWND m_hBtnAppsRemove;
    HWND m_hStaticBaseSystem;
    HWND m_hStaticAppCount;
    HWND m_hStaticTotalDisk;
    HWND m_hStaticFloppyStatus;
    HWND m_hStaticFloppyTip;

    HWND m_hBtnBuildImage;
    HWND m_hBtnUpdate;
    HWND m_hBtnPushUpdate;
    HWND m_hBtnBuildFloppy;
    bool m_otaAdmin = false;          // true once the admin password was entered this session
    HWND m_hBtnOpenExplorer;
    HWND m_hBtnSaveConfig;
    HWND m_hBtnInjectApp;
    HWND m_hBtnSetupEnv;
    HWND m_hBtnBootVM;
    HWND m_hGroupLog;
    HWND m_hEditBuildLog;
    HFONT m_hConsoleFont;
    HFONT m_hBoldFont;
    HFONT m_hHeaderFont;
    HWND m_hComboKProfile = nullptr;
    HWND m_hCheckKMod[KMOD_COUNT] = {};
    bool m_populatingApps = false;
    bool m_isBuilding = false;
    bool m_isUpdate = false;         // the in-progress build is an in-place update (keeps user files)
    std::wstring m_repoRoot;
    std::wstring m_blueScsiDrive;
    std::wstring m_lastBuiltImage;   // local path of the image the last build produced
    std::wstring m_cardTarget;       // SD card destination to copy it to (empty = none)
    std::vector<DiscoveredApp> m_discoveredApps;
    std::vector<SourceApp>     m_sourceApps;      // every apps/<cat>/<name>/ found on disk

    // Native P68 toolchain
    BuildBackend m_backend     = BB_NONE;    // BB_P68_NATIVE once the tools are built
    std::wstring m_p68Dir;                    // path to toolchain\bin\ holding the *.exe tools

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void create_controls();
    void layout_controls(int width, int height);
    void update_controls_from_cfg();
    void update_cfg_from_controls();

    // Kernel build settings
    void on_kprofile_changed();           // preset picked: tick the matching modules
    void on_kmod_toggled();               // a module box clicked: preset becomes "Custom"
    bool save_kernel_cfg();               // writes kernel.cfg (make syntax) in the repo root
    void load_kernel_cfg();               // reads it back at startup
    std::wstring kernel_summary() const;  // one-line description for the build log

    void populate_boot_list();
    void on_boot_add();
    void on_boot_edit();
    void on_boot_remove();
    void on_boot_up();
    void on_boot_down();

    void populate_apps_list();
    void update_apps_count_label();
    void on_apps_select_all();
    void on_apps_select_none();
    void on_apps_sys_only();
    void on_apps_games_only();
    void on_apps_add_custom();
    void on_apps_remove_selected();
    void sync_included_apps_from_list();
    void scan_compile_directory();
    void scan_source_directory();      // walk apps/<category>/<name>/ in the repo
    uint64_t get_app_size_bytes(const std::string& name);
    uint64_t get_total_selected_size_bytes();
    uint64_t get_base_system_bytes() const;
    uint64_t get_total_bootable_size_bytes();
    void on_build_finished(DWORD exitCode);
    void on_floppy_finished(DWORD exitCode);

    void action_browse_image();
    void action_setup_environment();
    void action_boot_vm();
    void detect_and_init_environment();
    void detect_build_backend();               // locate toolchain\bin\*.exe (the P68 tools)
    std::wstring build_make_command(const std::wstring& target, const std::wstring& extraFlags) const;
    std::wstring find_boot_image() const;
    void action_inject_external();
};
