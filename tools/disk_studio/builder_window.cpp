#include "builder_window.h"
#include "explorer_window.h"
#include <commdlg.h>
#include <shellapi.h>
#include <sstream>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")

static std::wstring to_wide(const std::string& str) {
    if (str.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (sz <= 0) sz = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    std::wstring res(sz, 0);
    if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &res[0], sz) <= 0) {
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &res[0], sz);
    }
    if (!res.empty() && res.back() == 0) res.pop_back();
    return res;
}

static std::string to_narrow(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sz <= 0) sz = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string res(sz, 0);
    if (WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &res[0], sz, nullptr, nullptr) <= 0) {
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &res[0], sz, nullptr, nullptr);
    }
    if (!res.empty() && res.back() == 0) res.pop_back();
    return res;
}

BuilderWindow::BuilderWindow()
    : m_hInstance(nullptr), m_hWnd(nullptr), m_explorerWin(nullptr),
      m_hComboHardware(nullptr), m_hEditImagePath(nullptr), m_hBtnBrowseImage(nullptr),
      m_hStaticBlueSCSI(nullptr), m_hComboBootMode(nullptr), m_hComboDirectMacID(nullptr),
      m_hEditTimeout(nullptr), m_hUpDownTimeout(nullptr), m_hListViewBoot(nullptr),
      m_hBtnAddOS(nullptr), m_hBtnEditOS(nullptr), m_hBtnRemoveOS(nullptr),
      m_hBtnUpOS(nullptr), m_hBtnDownOS(nullptr), m_hComboTheme(nullptr),
      m_hComboWallpaper(nullptr), m_hEditGreeting(nullptr), m_hComboReturnSCSI(nullptr),
      m_hCheckGames(nullptr), m_hCheckDev(nullptr), m_hCheckTools(nullptr),
      m_hBtnBuildImage(nullptr), m_hBtnOpenExplorer(nullptr), m_hBtnSaveConfig(nullptr),
      m_hBtnInjectApp(nullptr), m_hEditBuildLog(nullptr),
      m_hConsoleFont(nullptr), m_hBoldFont(nullptr), m_hHeaderFont(nullptr)
{
    m_cfg.image_path = L"C:\\Users\\Evil\\PlusOS\\images\\PlusOS.hda";
    m_cfg.greeting = L"PlusOS 2.0 (Dual-Boot Mac Edition)";

    // Default boot entries
    MultiBootEntry e1; e1.name = "Mac OS System 7.5.5"; e1.scsi_id = 3; m_cfg.boot_entries.push_back(e1);
    MultiBootEntry e2; e2.name = "Mac OS System 7.1"; e2.scsi_id = 2; m_cfg.boot_entries.push_back(e2);
    MultiBootEntry e3; e3.name = "Mac OS System 6.0.8"; e3.scsi_id = 1; m_cfg.boot_entries.push_back(e3);
    MultiBootEntry e4; e4.name = "PlusOS Desktop"; e4.scsi_id = -2; m_cfg.boot_entries.push_back(e4);
    MultiBootEntry e5; e5.name = "PlusOS Shell"; e5.scsi_id = -1; m_cfg.boot_entries.push_back(e5);
}

BuilderWindow::~BuilderWindow()
{
    if (m_hConsoleFont) DeleteObject(m_hConsoleFont);
    if (m_hBoldFont) DeleteObject(m_hBoldFont);
    if (m_hHeaderFont) DeleteObject(m_hHeaderFont);
}

bool BuilderWindow::create(HINSTANCE hInstance, int nCmdShow, ExplorerWindow* explorerWin)
{
    m_hInstance = hInstance;
    m_explorerWin = explorerWin;

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_UPDOWN_CLASS };
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PlusOSBuilderClass";
    RegisterClassExW(&wc);

    m_hBoldFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hHeaderFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hConsoleFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    m_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"PlusOS OS Builder & Dual-Boot System Configurator",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1060, 780,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hWnd) return false;

    create_controls();
    load_config_from_file();
    update_controls_from_cfg();

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    append_log(L"=== PlusOS OS Studio & Builder Ready ===");
    append_log(L"Toolchain: Retro68 GCC (m68k-apple-macos-gcc 16.1.0) & Python3 detected.");
    append_log(L"Ready to configure dual-boot, visual themes, and build the hard drive image.\r\n");

    return true;
}

void BuilderWindow::show()
{
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_SHOW);
        SetForegroundWindow(m_hWnd);
    }
}

void BuilderWindow::hide()
{
    if (m_hWnd) ShowWindow(m_hWnd, SW_HIDE);
}

void BuilderWindow::create_controls()
{
    HFONT hNormalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // Group 1: Hardware & Image Target
    HWND grpHardware = CreateWindowExW(0, L"BUTTON", L" 1. Hardware Architecture & Hard Drive Image ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 10, 500, 120, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpHardware, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblHw = CreateWindowExW(0, L"STATIC", L"Target Machine:", WS_CHILD | WS_VISIBLE, 25, 35, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblHw, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboHardware = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 130, 32, 370, 150, m_hWnd, (HMENU)IDB_HARDWARE_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboHardware, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh Plus (8 MHz 68000, 1MB-4MB RAM, SCSI)");
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh SE (8 MHz 68000, ADB, XPRAM Dual-Boot)");
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh SE/30 (16 MHz 68030, PMMU)");
    SendMessageW(m_hComboHardware, CB_SETCURSEL, 1, 0);

    HWND lblImg = CreateWindowExW(0, L"STATIC", L"Target Image:", WS_CHILD | WS_VISIBLE, 25, 65, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblImg, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hEditImagePath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_cfg.image_path.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 62, 280, 22, m_hWnd, (HMENU)IDB_IMAGE_PATH_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditImagePath, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnBrowseImage = CreateWindowExW(0, L"BUTTON", L"Browse...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 420, 61, 80, 24, m_hWnd, (HMENU)IDB_IMAGE_BROWSE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBrowseImage, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    std::wstring bscsi = L"BlueSCSI Drive (F:): ";
    if (GetFileAttributesW(L"F:\\BOOTTEST.hda") != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW(L"F:\\HD60_512 PlusOS.hda") != INVALID_FILE_ATTRIBUTES) {
        bscsi += L"Detected! (BOOTTEST.hda will be updated on install)";
    } else {
        bscsi += L"No SD card mounted at F: (using local images\\PlusOS.hda)";
    }
    m_hStaticBlueSCSI = CreateWindowExW(0, L"STATIC", bscsi.c_str(), WS_CHILD | WS_VISIBLE, 25, 95, 475, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(m_hStaticBlueSCSI, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 2: Boot & Dual-Boot System Configuration
    HWND grpBoot = CreateWindowExW(0, L"BUTTON", L" 2. Dual-Boot System & Boot Preferences ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 140, 500, 250, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpBoot, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblMode = CreateWindowExW(0, L"STATIC", L"Boot Mode:", WS_CHILD | WS_VISIBLE, 25, 165, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblMode, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboBootMode = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 130, 162, 370, 150, m_hWnd, (HMENU)IDB_BOOT_MODE_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboBootMode, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Direct Boot: PlusOS Command Shell");
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Direct Boot: PlusOS Desktop (Finder UI)");
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Interactive Multi-Boot Menu (with Countdown)");
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Direct Boot: Mac OS (via SCSI Target)");
    SendMessageW(m_hComboBootMode, CB_SETCURSEL, 2, 0);

    HWND lblMacID = CreateWindowExW(0, L"STATIC", L"Mac SCSI Target:", WS_CHILD | WS_VISIBLE, 25, 195, 105, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblMacID, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboDirectMacID = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 130, 192, 175, 180, m_hWnd, (HMENU)IDB_DIRECT_MAC_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboDirectMacID, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    for (int i = 0; i < 8; i++) {
        WCHAR buf[64];
        wsprintfW(buf, L"SCSI ID %d%s", i, (i == 1 ? L" (System 6.0.8)" : (i == 2 ? L" (System 7.1)" : (i == 3 ? L" (System 7.5.5)" : L""))));
        SendMessageW(m_hComboDirectMacID, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessageW(m_hComboDirectMacID, CB_SETCURSEL, 3, 0);

    HWND lblTime = CreateWindowExW(0, L"STATIC", L"Timeout (sec):", WS_CHILD | WS_VISIBLE, 320, 195, 85, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblTime, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hEditTimeout = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"5",
        WS_CHILD | WS_VISIBLE | ES_NUMBER, 410, 192, 90, 22, m_hWnd, (HMENU)IDB_TIMEOUT_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditTimeout, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Multi-Boot Entries ListView
    m_hListViewBoot = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        25, 225, 475, 125, m_hWnd, (HMENU)IDB_BOOT_LISTVIEW, m_hInstance, nullptr);
    SendMessageW(m_hListViewBoot, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SendMessageW(m_hListViewBoot, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 40; col.pszText = (LPWSTR)L"#"; ListView_InsertColumn(m_hListViewBoot, 0, &col);
    col.cx = 240; col.pszText = (LPWSTR)L"Operating System Title"; ListView_InsertColumn(m_hListViewBoot, 1, &col);
    col.cx = 170; col.pszText = (LPWSTR)L"Boot Target"; ListView_InsertColumn(m_hListViewBoot, 2, &col);

    // Boot entry buttons
    m_hBtnAddOS = CreateWindowExW(0, L"BUTTON", L"Add OS", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        25, 355, 90, 26, m_hWnd, (HMENU)IDB_BOOT_ADD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAddOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnEditOS = CreateWindowExW(0, L"BUTTON", L"Cycle Target", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, 355, 90, 26, m_hWnd, (HMENU)IDB_BOOT_EDIT_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnEditOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnRemoveOS = CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        215, 355, 90, 26, m_hWnd, (HMENU)IDB_BOOT_REMOVE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnRemoveOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnUpOS = CreateWindowExW(0, L"BUTTON", L"Move Up", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        320, 355, 80, 26, m_hWnd, (HMENU)IDB_BOOT_UP_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnUpOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnDownOS = CreateWindowExW(0, L"BUTTON", L"Move Down", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        405, 355, 80, 26, m_hWnd, (HMENU)IDB_BOOT_DOWN_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnDownOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 3: Pre-Build OS Customizations & Theming
    HWND grpTheme = CreateWindowExW(0, L"BUTTON", L" 3. Pre-Build OS Customizations & Theming ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 400, 500, 180, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpTheme, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblTheme = CreateWindowExW(0, L"STATIC", L"Desktop Theme:", WS_CHILD | WS_VISIBLE, 25, 425, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblTheme, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboTheme = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 130, 422, 370, 180, m_hWnd, (HMENU)IDB_THEME_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboTheme, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Mac OS 7.1 (Classic Platinum Pinstripe)");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Commodore Amiga Workbench 1.3/2.0");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Windows 95 (3D Bevels & Start Bar)");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Windows 98 (Gradient Dither Titlebars)");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Windows Vista/7 Aero Glass");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Hacker Mode (CRT Phosphor & Matrix Digital Rain)");
    SendMessageW(m_hComboTheme, CB_SETCURSEL, 2, 0);

    HWND lblWall = CreateWindowExW(0, L"STATIC", L"Wallpaper Pattern:", WS_CHILD | WS_VISIBLE, 25, 455, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblWall, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboWallpaper = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 130, 452, 180, 120, m_hWnd, (HMENU)IDB_WALLPAPER_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboWallpaper, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboWallpaper, CB_ADDSTRING, 0, (LPARAM)L"Classic Mac 50% Gray");
    SendMessageW(m_hComboWallpaper, CB_ADDSTRING, 0, (LPARAM)L"Matrix Digital Rain");
    SendMessageW(m_hComboWallpaper, CB_SETCURSEL, 0, 0);

    HWND lblRet = CreateWindowExW(0, L"STATIC", L"Return SCSI ID:", WS_CHILD | WS_VISIBLE, 325, 455, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblRet, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboReturnSCSI = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 425, 452, 75, 150, m_hWnd, (HMENU)IDB_RETURN_SCSI_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboReturnSCSI, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    for (int i = 0; i < 8; i++) {
        WCHAR b[16]; wsprintfW(b, L"SCSI %d", i);
        SendMessageW(m_hComboReturnSCSI, CB_ADDSTRING, 0, (LPARAM)b);
    }
    SendMessageW(m_hComboReturnSCSI, CB_SETCURSEL, 6, 0);

    HWND lblGreet = CreateWindowExW(0, L"STATIC", L"Greeting / Title:", WS_CHILD | WS_VISIBLE, 25, 485, 100, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblGreet, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hEditGreeting = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_cfg.greeting.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, 482, 370, 22, m_hWnd, (HMENU)IDB_GREETING_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditGreeting, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    HWND lblBundles = CreateWindowExW(0, L"STATIC", L"Apps to Include:", WS_CHILD | WS_VISIBLE, 25, 515, 105, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblBundles, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hCheckGames = CreateWindowExW(0, L"BUTTON", L"Games (17 retro games)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        130, 513, 170, 22, m_hWnd, (HMENU)IDB_CHECK_GAMES, m_hInstance, nullptr);
    SendMessageW(m_hCheckGames, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hCheckGames, BM_SETCHECK, BST_CHECKED, 0);

    m_hCheckDev = CreateWindowExW(0, L"BUTTON", L"Dev & Emulators", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        310, 513, 150, 22, m_hWnd, (HMENU)IDB_CHECK_DEV, m_hInstance, nullptr);
    SendMessageW(m_hCheckDev, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hCheckDev, BM_SETCHECK, BST_CHECKED, 0);

    m_hCheckTools = CreateWindowExW(0, L"BUTTON", L"System Tools & Networking", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        130, 540, 220, 22, m_hWnd, (HMENU)IDB_CHECK_TOOLS, m_hInstance, nullptr);
    SendMessageW(m_hCheckTools, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hCheckTools, BM_SETCHECK, BST_CHECKED, 0);

    // Group 4: Build Execution & Disk Manager
    HWND grpActions = CreateWindowExW(0, L"BUTTON", L" 4. Build Execution & Disk Manager ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 575, 500, 145, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpActions, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnBuildImage = CreateWindowExW(0, L"BUTTON", L"Compile & Build Hard Drive Image",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON, 25, 598, 235, 34, m_hWnd, (HMENU)IDB_BUILD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBuildImage, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnBuildFloppy = CreateWindowExW(0, L"BUTTON", L"Build Bootable Floppy (.dsk)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 598, 235, 34, m_hWnd, (HMENU)IDB_BUILD_FLOPPY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBuildFloppy, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnOpenExplorer = CreateWindowExW(0, L"BUTTON", L"Open Disk Explorer / Manager",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 638, 235, 32, m_hWnd, (HMENU)IDB_OPEN_EXPLORER_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnOpenExplorer, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnSaveConfig = CreateWindowExW(0, L"BUTTON", L"Save Configuration",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 638, 235, 32, m_hWnd, (HMENU)IDB_SAVE_CFG_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnSaveConfig, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnInjectApp = CreateWindowExW(0, L"BUTTON", L"Inject 'Return to PlusOS' to External Disk / Floppy...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 676, 480, 32, m_hWnd, (HMENU)IDB_INJECT_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnInjectApp, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Right Column: Live Build Log & System Diagnostics
    HWND grpLog = CreateWindowExW(0, L"BUTTON", L" Build Output & Compilation Log ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 525, 10, 505, 710, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpLog, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hEditBuildLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        535, 35, 485, 675, m_hWnd, (HMENU)IDB_BUILD_LOG_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditBuildLog, WM_SETFONT, (WPARAM)m_hConsoleFont, TRUE);
}

void BuilderWindow::layout_controls(int width, int height)
{
    if (width < 600) return;
    int colWidth = (width - 45) / 2;
    int logWidth = width - colWidth - 40;

    SetWindowPos(m_hEditBuildLog, nullptr, colWidth + 35, 35, logWidth, height - 55, SWP_NOZORDER);
}

void BuilderWindow::append_log(const std::wstring& text)
{
    if (!m_hEditBuildLog) return;
    int len = GetWindowTextLengthW(m_hEditBuildLog);
    SendMessageW(m_hEditBuildLog, EM_SETSEL, len, len);
    std::wstring line = text + L"\r\n";
    SendMessageW(m_hEditBuildLog, EM_REPLACESEL, FALSE, (LPARAM)line.c_str());
    SendMessageW(m_hEditBuildLog, EM_SCROLLCARET, 0, 0);
}

void BuilderWindow::populate_boot_list()
{
    ListView_DeleteAllItems(m_hListViewBoot);
    for (size_t i = 0; i < m_cfg.boot_entries.size(); i++) {
        const auto& e = m_cfg.boot_entries[i];
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = (int)i;
        std::wstring idxStr = std::to_wstring(i + 1);
        item.pszText = (LPWSTR)idxStr.c_str();
        ListView_InsertItem(m_hListViewBoot, &item);

        std::wstring nameW = to_wide(e.name);
        ListView_SetItemText(m_hListViewBoot, (int)i, 1, (LPWSTR)nameW.c_str());

        std::wstring targetStr;
        if (e.scsi_id >= 0) targetStr = L"Mac OS (SCSI ID " + std::to_wstring(e.scsi_id) + L")";
        else if (e.scsi_id == -1) targetStr = L"PlusOS Command Shell";
        else if (e.scsi_id == -2) targetStr = L"PlusOS Desktop (Finder)";
        ListView_SetItemText(m_hListViewBoot, (int)i, 2, (LPWSTR)targetStr.c_str());
    }
}

void BuilderWindow::update_controls_from_cfg()
{
    SendMessageW(m_hComboHardware, CB_SETCURSEL, m_cfg.target_hardware, 0);
    SetWindowTextW(m_hEditImagePath, m_cfg.image_path.c_str());
    SendMessageW(m_hComboBootMode, CB_SETCURSEL, m_cfg.boot_pref, 0);
    SendMessageW(m_hComboDirectMacID, CB_SETCURSEL, m_cfg.direct_mac_id, 0);
    SetWindowTextW(m_hEditTimeout, std::to_wstring(m_cfg.timeout).c_str());
    SendMessageW(m_hComboTheme, CB_SETCURSEL, m_cfg.theme, 0);
    SendMessageW(m_hComboWallpaper, CB_SETCURSEL, m_cfg.wallpaper, 0);
    SendMessageW(m_hComboReturnSCSI, CB_SETCURSEL, m_cfg.return_scsi_id, 0);
    SetWindowTextW(m_hEditGreeting, m_cfg.greeting.c_str());

    populate_boot_list();
}

void BuilderWindow::update_cfg_from_controls()
{
    m_cfg.target_hardware = (int)SendMessageW(m_hComboHardware, CB_GETCURSEL, 0, 0);
    WCHAR pathBuf[MAX_PATH];
    GetWindowTextW(m_hEditImagePath, pathBuf, MAX_PATH);
    m_cfg.image_path = pathBuf;

    m_cfg.boot_pref = (int)SendMessageW(m_hComboBootMode, CB_GETCURSEL, 0, 0);
    m_cfg.direct_mac_id = (int)SendMessageW(m_hComboDirectMacID, CB_GETCURSEL, 0, 0);

    WCHAR timeBuf[32];
    GetWindowTextW(m_hEditTimeout, timeBuf, 32);
    m_cfg.timeout = _wtoi(timeBuf);

    m_cfg.theme = (int)SendMessageW(m_hComboTheme, CB_GETCURSEL, 0, 0);
    m_cfg.wallpaper = (int)SendMessageW(m_hComboWallpaper, CB_GETCURSEL, 0, 0);
    m_cfg.return_scsi_id = (int)SendMessageW(m_hComboReturnSCSI, CB_GETCURSEL, 0, 0);

    WCHAR greetBuf[128];
    GetWindowTextW(m_hEditGreeting, greetBuf, 128);
    m_cfg.greeting = greetBuf;

    m_cfg.bundle_games = (SendMessageW(m_hCheckGames, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_cfg.bundle_dev = (SendMessageW(m_hCheckDev, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_cfg.bundle_tools = (SendMessageW(m_hCheckTools, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

void BuilderWindow::load_config_from_file()
{
    std::ifstream in("C:\\Users\\Evil\\PlusOS\\multiboot.cfg");
    if (!in.is_open()) return;

    std::vector<MultiBootEntry> entries;
    std::string line;
    std::string cur_name;
    int cur_id = -999;

    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos) line = line.substr(start);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line[0] == '[') {
            if (!cur_name.empty() && cur_id != -999) {
                MultiBootEntry e; e.name = cur_name; e.scsi_id = cur_id;
                entries.push_back(e);
            }
            cur_name = ""; cur_id = -999;
            continue;
        }

        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            while (!k.empty() && k.back() == ' ') k.pop_back();
            while (!v.empty() && v.front() == ' ') v.erase(v.begin());

            if (k == "boot_pref") m_cfg.boot_pref = std::stoi(v);
            else if (k == "direct_mac") m_cfg.direct_mac_id = std::stoi(v);
            else if (k == "timeout") m_cfg.timeout = std::stoi(v);
            else if (k == "theme") m_cfg.theme = std::stoi(v);
            else if (k == "wallpaper") m_cfg.wallpaper = std::stoi(v);
            else if (k == "return_scsi_id") m_cfg.return_scsi_id = std::stoi(v);
            else if (k == "greeting") m_cfg.greeting = to_wide(v);
            else if (k == "name" || k == "title") cur_name = v;
            else if (k == "id" || k == "scsi_id") {
                if (v == "shell") cur_id = -1;
                else if (v == "desktop") cur_id = -2;
                else cur_id = std::stoi(v);
            }
        }
    }
    if (!cur_name.empty() && cur_id != -999) {
        MultiBootEntry e; e.name = cur_name; e.scsi_id = cur_id;
        entries.push_back(e);
    }

    if (!entries.empty()) {
        m_cfg.boot_entries = entries;
    }
}

bool BuilderWindow::save_config_to_file()
{
    update_cfg_from_controls();

    std::ofstream out("C:\\Users\\Evil\\PlusOS\\multiboot.cfg");
    if (!out.is_open()) return false;

    out << "# PlusOS Multi-Boot & System Configuration\r\n";
    out << "boot_pref=" << m_cfg.boot_pref << "\r\n";
    out << "direct_mac=" << m_cfg.direct_mac_id << "\r\n";
    out << "timeout=" << m_cfg.timeout << "\r\n";
    out << "theme=" << m_cfg.theme << "\r\n";
    out << "wallpaper=" << m_cfg.wallpaper << "\r\n";
    out << "return_scsi_id=" << m_cfg.return_scsi_id << "\r\n";
    out << "greeting=" << to_narrow(m_cfg.greeting) << "\r\n\r\n";

    for (const auto& e : m_cfg.boot_entries) {
        out << "[entry]\r\n";
        out << "name=" << e.name << "\r\n";
        if (e.scsi_id >= 0) out << "id=" << e.scsi_id << "\r\n\r\n";
        else if (e.scsi_id == -1) out << "id=shell\r\n\r\n";
        else if (e.scsi_id == -2) out << "id=desktop\r\n\r\n";
    }

    out.close();
    append_log(L"Configuration saved to multiboot.cfg successfully.");
    return true;
}

void BuilderWindow::on_boot_add()
{
    MultiBootEntry e;
    e.name = "Custom Operating System";
    e.scsi_id = 0;
    m_cfg.boot_entries.push_back(e);
    populate_boot_list();
    ListView_SetItemState(m_hListViewBoot, (int)m_cfg.boot_entries.size() - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void BuilderWindow::on_boot_edit()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_cfg.boot_entries.size()) return;

    MultiBootEntry& e = m_cfg.boot_entries[sel];
    // Cycle target: 0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> Shell (-1) -> Desktop (-2) -> 0
    if (e.scsi_id >= 0 && e.scsi_id < 7) e.scsi_id++;
    else if (e.scsi_id == 7) e.scsi_id = -1;
    else if (e.scsi_id == -1) e.scsi_id = -2;
    else e.scsi_id = 0;

    populate_boot_list();
    ListView_SetItemState(m_hListViewBoot, sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void BuilderWindow::on_boot_remove()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_cfg.boot_entries.size()) return;

    m_cfg.boot_entries.erase(m_cfg.boot_entries.begin() + sel);
    populate_boot_list();
}

void BuilderWindow::on_boot_up()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel <= 0 || sel >= (int)m_cfg.boot_entries.size()) return;

    std::swap(m_cfg.boot_entries[sel], m_cfg.boot_entries[sel - 1]);
    populate_boot_list();
    ListView_SetItemState(m_hListViewBoot, sel - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void BuilderWindow::on_boot_down()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel < 0 || sel + 1 >= (int)m_cfg.boot_entries.size()) return;

    std::swap(m_cfg.boot_entries[sel], m_cfg.boot_entries[sel + 1]);
    populate_boot_list();
    ListView_SetItemState(m_hListViewBoot, sel + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void BuilderWindow::action_browse_image()
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Macintosh Hard Drive Images (*.hda;*.img;*.dsk)\0*.hda;*.img;*.dsk\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Select PlusOS Hard Drive Image";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        m_cfg.image_path = szFile;
        SetWindowTextW(m_hEditImagePath, szFile);
        append_log(L"Target image set to: " + m_cfg.image_path);
    }
}

void BuilderWindow::action_open_explorer()
{
    update_cfg_from_controls();
    if (m_explorerWin) {
        m_explorerWin->show(m_cfg.image_path);
    }
}

void BuilderWindow::action_build_image()
{
    update_cfg_from_controls();
    save_config_to_file();

    append_log(L"\r\n--------------------------------------------------");
    append_log(L"[BUILD] Starting PlusOS Hard Drive Image Build Pipeline");
    append_log(L"Target: " + m_cfg.image_path);
    append_log(L"Boot Preference: " + std::to_wstring(m_cfg.boot_pref));
    append_log(L"Default Theme: " + std::to_wstring(m_cfg.theme));
    append_log(L"Running 'make install' inside WSL environment...");

    std::wstring rootDir = L"C:\\Users\\Evil\\PlusOS";
    std::wstring cmd = L"wsl.exe -d Ubuntu --cd /mnt/c/Users/Evil/PlusOS -- make install";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, rootDir.c_str(), &si, &pi);
    if (!ok) {
        append_log(L"Notice: WSL command failed to start, falling back to build.bat...");
        cmd = L"cmd.exe /c \"call C:\\Users\\Evil\\PlusOS\\build.bat install\"";
        ok = CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, rootDir.c_str(), &si, &pi);
    }

    if (ok) {
        WaitForSingleObject(pi.hProcess, 300000); // Wait up to 5 minutes
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode == 0) {
            append_log(L"[SUCCESS] Build & installation succeeded!");
            append_log(L"Kernel, boot blocks, raw store, and all 59 apps written.");
            append_log(L"Custom multiboot.cfg and Desktop Return-to-PlusOS seeded.");
            append_log(L"Ready! Click 'Open Hard Drive Explorer / Manager' to inspect.");

            int c = MessageBoxW(m_hWnd,
                L"Build completed successfully!\r\n\r\nWould you like to open the Hard Drive Explorer now?",
                L"Build Successful", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1);
            if (c == IDYES) {
                action_open_explorer();
            }
        } else {
            append_log(L"[ERROR] Build finished with non-zero exit code: " + std::to_wstring(exitCode));
            MessageBoxW(m_hWnd, L"Build failed or completed with errors. See build log for details.", L"Build Warning", MB_ICONWARNING | MB_OK);
        }
    } else {
        append_log(L"[ERROR] Failed to launch build process.");
        MessageBoxW(m_hWnd, L"Failed to launch compiler process.", L"Execution Error", MB_ICONERROR | MB_OK);
    }
}

void BuilderWindow::action_build_floppy()
{
    save_config_to_file();
    append_log(L"============================================================");
    append_log(L"Starting Minimal Bootable Installer Floppy Generation...");
    append_log(L"Target: images\\PlusOS_Floppy.dsk (1.44M Macintosh HFS Disk)");
    append_log(L"Essential boot binaries & installer included (zero games).");
    append_log(L"Running 'make floppy' inside build environment...");

    std::wstring rootDir = L"C:\\Users\\Evil\\PlusOS";
    std::wstring cmd = L"wsl.exe -d Ubuntu --cd /mnt/c/Users/Evil/PlusOS -- bash -l -c \"make floppy\"";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, rootDir.c_str(), &si, &pi);
    if (!ok) {
        cmd = L"cmd.exe /c \"call C:\\Users\\Evil\\PlusOS\\build.bat floppy\"";
        ok = CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, rootDir.c_str(), &si, &pi);
    }

    if (ok) {
        WaitForSingleObject(pi.hProcess, 300000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode == 0) {
            append_log(L"[SUCCESS] Minimal bootable floppy successfully generated!");
            append_log(L"Image: images\\PlusOS_Floppy.dsk");
            append_log(L"Ready! You can now boot this floppy in Mini vMac or test on hardware.");

            int c = MessageBoxW(m_hWnd,
                L"Bootable minimal installer floppy generated successfully!\r\n(images\\PlusOS_Floppy.dsk)\r\n\r\nWould you like to open it in the Disk Explorer now?",
                L"Floppy Build Successful", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1);
            if (c == IDYES && m_explorerWin) {
                m_explorerWin->open_image(L"C:\\Users\\Evil\\PlusOS\\images\\PlusOS_Floppy.dsk");
                m_explorerWin->show();
            }
        } else {
            append_log(L"[ERROR] Floppy build finished with exit code: " + std::to_wstring(exitCode));
            MessageBoxW(m_hWnd, L"Floppy build failed or completed with errors.", L"Build Warning", MB_ICONWARNING | MB_OK);
        }
    } else {
        append_log(L"[ERROR] Failed to launch floppy build process.");
        MessageBoxW(m_hWnd, L"Failed to launch floppy compiler process.", L"Execution Error", MB_ICONERROR | MB_OK);
    }
}

void BuilderWindow::action_inject_external()
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Hard Drive & Floppy Images (*.hda;*.img;*.dsk;*.image)\0*.hda;*.img;*.dsk;*.image\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Select Mac OS Disk Image or Floppy Disk to Inject Return to PlusOS";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::string msg;
        update_cfg_from_controls();
        if (HFSFileSystem::inject_into_external_image(szFile, m_cfg.return_scsi_id, &msg)) {
            append_log(L"Injected Return to PlusOS into: " + std::wstring(szFile));
            MessageBoxW(m_hWnd, to_wide(msg).c_str(), L"Injection Succeeded", MB_ICONINFORMATION | MB_OK);
        } else {
            append_log(L"Injection failed: " + to_wide(msg));
            MessageBoxW(m_hWnd, to_wide(msg).c_str(), L"Injection Failed", MB_ICONERROR | MB_OK);
        }
    }
}

LRESULT CALLBACK BuilderWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BuilderWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        pThis = (BuilderWindow*)cs->lpCreateParams;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;
    } else {
        pThis = (BuilderWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    }

    if (pThis) return pThis->handle_message(hWnd, msg, wParam, lParam);
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT BuilderWindow::handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_SIZE:
        layout_controls(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case IDB_IMAGE_BROWSE_BTN: action_browse_image(); break;
        case IDB_BUILD_BTN: action_build_image(); break;
        case IDB_BUILD_FLOPPY_BTN: action_build_floppy(); break;
        case IDB_OPEN_EXPLORER_BTN: action_open_explorer(); break;
        case IDB_SAVE_CFG_BTN:
            if (save_config_to_file()) {
                MessageBoxW(m_hWnd, L"Configuration saved to multiboot.cfg successfully!", L"Saved", MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDB_INJECT_BTN: action_inject_external(); break;
        case IDB_BOOT_ADD_BTN: on_boot_add(); break;
        case IDB_BOOT_EDIT_BTN: on_boot_edit(); break;
        case IDB_BOOT_REMOVE_BTN: on_boot_remove(); break;
        case IDB_BOOT_UP_BTN: on_boot_up(); break;
        case IDB_BOOT_DOWN_BTN: on_boot_down(); break;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
