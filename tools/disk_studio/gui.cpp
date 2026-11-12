#include "gui.h"
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")

static std::string format_size(uint32_t bytes) {
    if (bytes == 0) return "0 B";
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
        return std::string(buf);
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
    return std::string(buf);
}

static std::wstring to_wide(const std::string& str) {
    if (str.empty()) return L"";
    int sz = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    std::wstring res(sz, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &res[0], sz);
    if (!res.empty() && res.back() == 0) res.pop_back();
    return res;
}

static std::string to_narrow(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sz = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string res(sz, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &res[0], sz, nullptr, nullptr);
    if (!res.empty() && res.back() == 0) res.pop_back();
    return res;
}

MainWindow::MainWindow()
    : m_hInstance(nullptr), m_hWnd(nullptr), m_hToolbar(nullptr), m_hStatusBar(nullptr),
      m_hTreeView(nullptr), m_hTabControl(nullptr), m_hStaticPath(nullptr), m_hBtnUp(nullptr),
      m_hListViewFiles(nullptr), m_hListViewBoot(nullptr), m_hBtnBootAdd(nullptr),
      m_hBtnBootEdit(nullptr), m_hBtnBootRemove(nullptr), m_hBtnBootUp(nullptr),
      m_hBtnBootDown(nullptr), m_hBtnBootSave(nullptr), m_hComboBootPref(nullptr),
      m_hEditDiagnostics(nullptr), m_current_folder_cnid(2), m_current_tab(0), m_boot_pref(2)
{
}

MainWindow::~MainWindow()
{
}

bool MainWindow::create(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PlusOSDiskStudioClass";
    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        wc.lpszClassName,
        L"PlusOS Disk Studio - Macintosh & PlusOS Image Manager",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1020, 700,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hWnd) return false;

    create_controls();
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return true;
}

int MainWindow::run()
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        pThis = (MainWindow*)cs->lpCreateParams;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (MainWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->handle_message(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void MainWindow::create_controls()
{
    // Status Bar
    m_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hWnd, (HMENU)IDC_STATUSBAR, m_hInstance, nullptr);
    int sbParts[4] = { 280, 520, 760, -1 };
    SendMessageW(m_hStatusBar, SB_SETPARTS, 4, (LPARAM)sbParts);

    // Toolbar
    m_hToolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP,
        0, 0, 0, 0, m_hWnd, (HMENU)IDC_TOOLBAR, m_hInstance, nullptr);
    SendMessageW(m_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_ACTION_BUILD_IMAGE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 🔨 Build Image " },
        { I_IMAGENONE, ID_FILE_OPEN_IMAGE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 📂 Open Image " },
        { I_IMAGENONE, ID_ACTION_START_OPTIONS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" ⚡ Quick Start Options " },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_ACTION_INJECT_CURRENT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 💉 Inject Return to PlusOS " },
        { I_IMAGENONE, ID_ACTION_INJECT_EXTERNAL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 💾 Inject to Floppy/Image... " },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_ACTION_IMPORT_FILES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 📥 Import Files... " },
        { I_IMAGENONE, ID_ACTION_EXPORT_FILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 📤 Export Selected " },
        { I_IMAGENONE, ID_ACTION_DELETE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" ❌ Delete " },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_ACTION_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" 🔄 Refresh (F5) " },
    };
    SendMessageW(m_hToolbar, TB_ADDBUTTONSW, sizeof(buttons) / sizeof(buttons[0]), (LPARAM)buttons);
    SendMessageW(m_hToolbar, TB_AUTOSIZE, 0, 0);

    // Tree View (Folder Hierarchy)
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, 240, 400, m_hWnd, (HMENU)IDC_TREEVIEW, m_hInstance, nullptr);

    // Tab Control (File Explorer, Multi-Boot Config, Diagnostics)
    m_hTabControl = CreateWindowExW(0, WC_TABCONTROLW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
        245, 0, 750, 400, m_hWnd, (HMENU)IDC_TABCONTROL, m_hInstance, nullptr);

    TCITEMW tcItem = {};
    tcItem.mask = TCIF_TEXT;
    tcItem.pszText = (LPWSTR)L" File Explorer ";
    TabCtrl_InsertItem(m_hTabControl, 0, &tcItem);
    tcItem.pszText = (LPWSTR)L" Multi-Boot Configuration ";
    TabCtrl_InsertItem(m_hTabControl, 1, &tcItem);
    tcItem.pszText = (LPWSTR)L" Disk Diagnostics ";
    TabCtrl_InsertItem(m_hTabControl, 2, &tcItem);

    // Path address bar and Up button for File Explorer
    m_hBtnUp = CreateWindowExW(0, L"BUTTON", L"⬆ Up", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        250, 36, 65, 26, m_hWnd, (HMENU)IDC_BTN_UP, m_hInstance, nullptr);
    m_hStaticPath = CreateWindowExW(0, L"STATIC", L"Path: :", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        322, 36, 600, 26, m_hWnd, (HMENU)IDC_STATIC_PATH, m_hInstance, nullptr);

    // Tab 0: File Explorer List View
    m_hListViewFiles = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        250, 68, 730, 330, m_hWnd, (HMENU)IDC_LISTVIEW_FILES, m_hInstance, nullptr);
    ListView_SetExtendedListViewStyle(m_hListViewFiles, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = (LPWSTR)L"Name"; col.cx = 240; ListView_InsertColumn(m_hListViewFiles, 0, &col);
    col.pszText = (LPWSTR)L"Type"; col.cx = 75; ListView_InsertColumn(m_hListViewFiles, 1, &col);
    col.pszText = (LPWSTR)L"Creator"; col.cx = 75; ListView_InsertColumn(m_hListViewFiles, 2, &col);
    col.pszText = (LPWSTR)L"Data Fork"; col.cx = 95; ListView_InsertColumn(m_hListViewFiles, 3, &col);
    col.pszText = (LPWSTR)L"Resource Fork"; col.cx = 100; ListView_InsertColumn(m_hListViewFiles, 4, &col);
    col.pszText = (LPWSTR)L"Details / Date"; col.cx = 140; ListView_InsertColumn(m_hListViewFiles, 5, &col);

    // Tab 1: Multi-Boot Config Controls
    m_hListViewBoot = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        250, 40, 540, 330, m_hWnd, (HMENU)IDC_LISTVIEW_BOOT, m_hInstance, nullptr);
    ListView_SetExtendedListViewStyle(m_hListViewBoot, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    col.pszText = (LPWSTR)L"#"; col.cx = 40; ListView_InsertColumn(m_hListViewBoot, 0, &col);
    col.pszText = (LPWSTR)L"Operating System Title"; col.cx = 250; ListView_InsertColumn(m_hListViewBoot, 1, &col);
    col.pszText = (LPWSTR)L"Target / SCSI Boot ID"; col.cx = 230; ListView_InsertColumn(m_hListViewBoot, 2, &col);

    m_hBtnBootAdd = CreateWindowExW(0, L"BUTTON", L"Add OS Entry...", WS_CHILD | WS_TABSTOP,
        810, 50, 160, 32, m_hWnd, (HMENU)IDC_BTN_BOOT_ADD, m_hInstance, nullptr);
    m_hBtnBootEdit = CreateWindowExW(0, L"BUTTON", L"Edit Title/ID...", WS_CHILD | WS_TABSTOP,
        810, 92, 160, 32, m_hWnd, (HMENU)IDC_BTN_BOOT_EDIT, m_hInstance, nullptr);
    m_hBtnBootRemove = CreateWindowExW(0, L"BUTTON", L"Remove Entry", WS_CHILD | WS_TABSTOP,
        810, 134, 160, 32, m_hWnd, (HMENU)IDC_BTN_BOOT_REMOVE, m_hInstance, nullptr);
    m_hBtnBootUp = CreateWindowExW(0, L"BUTTON", L"Move Up", WS_CHILD | WS_TABSTOP,
        810, 185, 160, 32, m_hWnd, (HMENU)IDC_BTN_BOOT_UP, m_hInstance, nullptr);
    m_hBtnBootDown = CreateWindowExW(0, L"BUTTON", L"Move Down", WS_CHILD | WS_TABSTOP,
        810, 227, 160, 32, m_hWnd, (HMENU)IDC_BTN_BOOT_DOWN, m_hInstance, nullptr);

    m_hComboBootPref = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST, 810, 280, 160, 120, m_hWnd, (HMENU)IDC_COMBO_BOOT_PREF, m_hInstance, nullptr);
    SendMessageW(m_hComboBootPref, CB_ADDSTRING, 0, (LPARAM)L"Boot: PlusOS Shell");
    SendMessageW(m_hComboBootPref, CB_ADDSTRING, 0, (LPARAM)L"Boot: PlusOS Desktop");
    SendMessageW(m_hComboBootPref, CB_ADDSTRING, 0, (LPARAM)L"Boot: Multi-Boot Menu");
    SendMessageW(m_hComboBootPref, CB_SETCURSEL, 2, 0);

    m_hBtnBootSave = CreateWindowExW(0, L"BUTTON", L"Save to Disk Image", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
        810, 330, 160, 40, m_hWnd, (HMENU)IDC_BTN_BOOT_SAVE, m_hInstance, nullptr);

    // Tab 2: Diagnostics Edit Box
    m_hEditDiagnostics = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
        WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_HSCROLL,
        250, 40, 720, 350, m_hWnd, (HMENU)IDC_EDIT_DIAGNOSTICS, m_hInstance, nullptr);
    SendMessageW(m_hEditDiagnostics, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), TRUE);

    // Drag and Drop
    DragAcceptFiles(m_hWnd, TRUE);
    DragAcceptFiles(m_hListViewFiles, TRUE);

    update_status_bar();
}

void MainWindow::layout_controls(int width, int height)
{
    if (m_hToolbar) SendMessageW(m_hToolbar, TB_AUTOSIZE, 0, 0);
    RECT rcTb = {};
    if (m_hToolbar) GetWindowRect(m_hToolbar, &rcTb);
    int tbHeight = rcTb.bottom - rcTb.top;

    RECT rcSb = {};
    if (m_hStatusBar) GetWindowRect(m_hStatusBar, &rcSb);
    int sbHeight = rcSb.bottom - rcSb.top;
    if (m_hStatusBar) SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

    int clientTop = tbHeight + 4;
    int clientHeight = height - clientTop - sbHeight - 4;
    if (clientHeight < 100) clientHeight = 100;

    int treeWidth = 240;
    int rightLeft = treeWidth + 8;
    int rightWidth = width - rightLeft - 8;
    if (rightWidth < 200) rightWidth = 200;

    if (m_hTreeView) MoveWindow(m_hTreeView, 8, clientTop, treeWidth, clientHeight, TRUE);
    if (m_hTabControl) MoveWindow(m_hTabControl, rightLeft, clientTop, rightWidth, clientHeight, TRUE);

    RECT rcTab;
    GetClientRect(m_hTabControl, &rcTab);
    TabCtrl_AdjustRect(m_hTabControl, FALSE, &rcTab);

    int tabContentX = rightLeft + rcTab.left;
    int tabContentY = clientTop + rcTab.top;
    int tabContentW = rcTab.right - rcTab.left;
    int tabContentH = rcTab.bottom - rcTab.top;

    // Path Bar
    int pathBarH = 28;
    if (m_hBtnUp) MoveWindow(m_hBtnUp, tabContentX, tabContentY + 2, 60, pathBarH - 2, TRUE);
    if (m_hStaticPath) MoveWindow(m_hStaticPath, tabContentX + 66, tabContentY + 2, tabContentW - 70, pathBarH - 2, TRUE);

    // File list under path bar
    int listY = tabContentY + pathBarH + 2;
    int listH = tabContentH - pathBarH - 4;
    if (listH < 50) listH = 50;

    if (m_hListViewFiles) MoveWindow(m_hListViewFiles, tabContentX, listY, tabContentW, listH, TRUE);

    // Multi-Boot Tab layout
    int bootBtnW = 160;
    int bootListW = tabContentW - bootBtnW - 16;
    if (bootListW < 100) bootListW = 100;
    int bootBtnX = tabContentX + bootListW + 12;

    if (m_hListViewBoot) MoveWindow(m_hListViewBoot, tabContentX, tabContentY, bootListW, tabContentH, TRUE);
    if (m_hBtnBootAdd) MoveWindow(m_hBtnBootAdd, bootBtnX, tabContentY + 8, bootBtnW, 32, TRUE);
    if (m_hBtnBootEdit) MoveWindow(m_hBtnBootEdit, bootBtnX, tabContentY + 48, bootBtnW, 32, TRUE);
    if (m_hBtnBootRemove) MoveWindow(m_hBtnBootRemove, bootBtnX, tabContentY + 88, bootBtnW, 32, TRUE);
    if (m_hBtnBootUp) MoveWindow(m_hBtnBootUp, bootBtnX, tabContentY + 138, bootBtnW, 32, TRUE);
    if (m_hBtnBootDown) MoveWindow(m_hBtnBootDown, bootBtnX, tabContentY + 178, bootBtnW, 32, TRUE);
    if (m_hComboBootPref) MoveWindow(m_hComboBootPref, bootBtnX, tabContentY + 230, bootBtnW, 100, TRUE);
    if (m_hBtnBootSave) MoveWindow(m_hBtnBootSave, bootBtnX, tabContentY + 280, bootBtnW, 38, TRUE);

    if (m_hEditDiagnostics) MoveWindow(m_hEditDiagnostics, tabContentX, tabContentY, tabContentW, tabContentH, TRUE);
}

void MainWindow::update_status_bar()
{
    if (!m_hStatusBar) return;

    if (!m_disk.is_open()) {
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"No disk image opened");
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 1, (LPARAM)L"");
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 2, (LPARAM)L"");
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 3, (LPARAM)L"Click 'Build Image' or 'Open Image'");
        return;
    }

    std::wstring imgName = m_disk.get_path();
    size_t lastSlash = imgName.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) imgName = imgName.substr(lastSlash + 1);

    std::wstring p1 = imgName + (m_disk.is_read_only() ? L" [Read-Only]" : L" [Mounted]");
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)p1.c_str());

    std::wstring p2 = L"Volume: " + to_wide(m_fs.get_volume_name());
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 1, (LPARAM)p2.c_str());

    std::wstring p3 = L"Free: " + to_wide(format_size((uint32_t)m_fs.get_free_bytes())) +
                      L" of " + to_wide(format_size((uint32_t)m_fs.get_total_bytes()));
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 2, (LPARAM)p3.c_str());

    std::wstring p4 = L"Items: " + std::to_wstring(m_fs.get_total_files()) + L" files, " +
                      std::to_wstring(m_fs.get_total_folders()) + L" folders";
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 3, (LPARAM)p4.c_str());
}

std::wstring MainWindow::get_folder_path(uint32_t cnid) const
{
    if (cnid == 2) return L":";
    const HFSItem* cur = m_fs.find_by_cnid(cnid);
    if (!cur) return L":";

    std::wstring path = L":" + to_wide(cur->name);
    uint32_t pid = cur->parent_cnid;
    while (pid != 2 && pid != 1 && pid != 0) {
        const HFSItem* parent = m_fs.find_by_cnid(pid);
        if (!parent) break;
        path = L":" + to_wide(parent->name) + path;
        pid = parent->parent_cnid;
    }
    return path;
}

static void insert_tree_node(HWND hTree, HTREEITEM hParent, const HFSItem& item, const HFSFileSystem& fs) {
    TVINSERTSTRUCTW tv = {};
    tv.hParent = hParent;
    tv.hInsertAfter = TVI_LAST;
    tv.item.mask = TVIF_TEXT | TVIF_PARAM;

    std::wstring wname = to_wide(item.name);
    tv.item.pszText = (LPWSTR)wname.c_str();
    tv.item.lParam = (LPARAM)item.cnid;

    HTREEITEM hItem = TreeView_InsertItem(hTree, &tv);

    std::vector<HFSItem> children = fs.get_children(item.cnid);
    for (const auto& child : children) {
        if (child.is_directory) {
            insert_tree_node(hTree, hItem, child, fs);
        }
    }

    if (hParent == TVI_ROOT || item.cnid == 2) {
        TreeView_Expand(hTree, hItem, TVE_EXPAND);
    }
}

void MainWindow::populate_tree()
{
    TreeView_DeleteAllItems(m_hTreeView);
    if (!m_fs.is_mounted()) return;

    const HFSItem* root = m_fs.find_by_cnid(2);
    if (root) {
        insert_tree_node(m_hTreeView, TVI_ROOT, *root, m_fs);
    }
}

void MainWindow::navigate_to_folder(uint32_t folder_cnid)
{
    m_current_folder_cnid = folder_cnid;
    populate_files_list(folder_cnid);

    // Update Path label
    std::wstring pathStr = L"Path: " + get_folder_path(folder_cnid);
    SetWindowTextW(m_hStaticPath, pathStr.c_str());

    // Sync selection in tree view
    // (Search tree items)
    HTREEITEM hRoot = TreeView_GetRoot(m_hTreeView);
    auto find_tree_item = [&](auto& self, HTREEITEM hCur) -> HTREEITEM {
        if (!hCur) return nullptr;
        TVITEMW tvi = {};
        tvi.mask = TVIF_PARAM;
        tvi.hItem = hCur;
        TreeView_GetItem(m_hTreeView, &tvi);
        if (tvi.lParam == (LPARAM)folder_cnid) return hCur;

        HTREEITEM hChild = TreeView_GetChild(m_hTreeView, hCur);
        while (hChild) {
            HTREEITEM res = self(self, hChild);
            if (res) return res;
            hChild = TreeView_GetNextSibling(m_hTreeView, hChild);
        }
        return nullptr;
    };

    HTREEITEM target = find_tree_item(find_tree_item, hRoot);
    if (target) {
        TreeView_SelectItem(m_hTreeView, target);
    }
}

void MainWindow::populate_files_list(uint32_t folder_cnid)
{
    m_current_folder_cnid = folder_cnid;
    ListView_DeleteAllItems(m_hListViewFiles);
    if (!m_fs.is_mounted()) return;

    std::vector<HFSItem> children = m_fs.get_children(folder_cnid);

    int idx = 0;
    // If not root, add a ".." parent folder entry
    if (folder_cnid != 2) {
        const HFSItem* cur = m_fs.find_by_cnid(folder_cnid);
        uint32_t parent_id = cur ? cur->parent_cnid : 2;

        LVITEMW item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = idx;
        item.iSubItem = 0;
        item.pszText = (LPWSTR)L"📁 .. [Parent Folder]";
        item.lParam = (LPARAM)parent_id;
        ListView_InsertItem(m_hListViewFiles, &item);

        ListView_SetItemText(m_hListViewFiles, idx, 1, (LPWSTR)L"Folder");
        ListView_SetItemText(m_hListViewFiles, idx, 2, (LPWSTR)L"");
        ListView_SetItemText(m_hListViewFiles, idx, 3, (LPWSTR)L"");
        ListView_SetItemText(m_hListViewFiles, idx, 4, (LPWSTR)L"");
        ListView_SetItemText(m_hListViewFiles, idx, 5, (LPWSTR)L"Parent Directory");
        idx++;
    }

    for (const auto& child : children) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = idx;
        item.iSubItem = 0;

        std::wstring prefix = child.is_directory ? L"📁 " : L"📄 ";
        std::wstring wname = prefix + to_wide(child.name);
        item.pszText = (LPWSTR)wname.c_str();
        item.lParam = (LPARAM)child.cnid;
        ListView_InsertItem(m_hListViewFiles, &item);

        std::wstring type_str = child.is_directory ? L"Folder" : to_wide(fourcc_to_str(child.type));
        ListView_SetItemText(m_hListViewFiles, idx, 1, (LPWSTR)type_str.c_str());

        std::wstring creator_str = child.is_directory ? L"" : to_wide(fourcc_to_str(child.creator));
        ListView_SetItemText(m_hListViewFiles, idx, 2, (LPWSTR)creator_str.c_str());

        std::wstring df_str = child.is_directory ? L"" : to_wide(format_size(child.data_size));
        ListView_SetItemText(m_hListViewFiles, idx, 3, (LPWSTR)df_str.c_str());

        std::wstring rf_str = child.is_directory ? L"" : to_wide(format_size(child.rsrc_size));
        ListView_SetItemText(m_hListViewFiles, idx, 4, (LPWSTR)rf_str.c_str());

        std::wstring mod_str = child.is_directory ? L"Directory" : L"Mac Volume File";
        ListView_SetItemText(m_hListViewFiles, idx, 5, (LPWSTR)mod_str.c_str());

        idx++;
    }

    std::wstring pathStr = L"Path: " + get_folder_path(folder_cnid);
    SetWindowTextW(m_hStaticPath, pathStr.c_str());
}

void MainWindow::populate_boot_tab()
{
    ListView_DeleteAllItems(m_hListViewBoot);
    if (!m_fs.is_mounted()) return;

    m_fs.read_multiboot_config(m_boot_entries, m_boot_pref);
    SendMessageW(m_hComboBootPref, CB_SETCURSEL, m_boot_pref >= 0 && m_boot_pref <= 2 ? m_boot_pref : 2, 0);

    for (size_t i = 0; i < m_boot_entries.size(); ++i) {
        const auto& e = m_boot_entries[i];
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = (int)i;
        item.iSubItem = 0;

        std::wstring num_str = std::to_wstring(i + 1);
        item.pszText = (LPWSTR)num_str.c_str();
        ListView_InsertItem(m_hListViewBoot, &item);

        std::wstring name_str = to_wide(e.name);
        ListView_SetItemText(m_hListViewBoot, (int)i, 1, (LPWSTR)name_str.c_str());

        std::wstring target_str;
        if (e.scsi_id >= 0 && e.scsi_id <= 7) target_str = L"SCSI ID " + std::to_wstring(e.scsi_id);
        else if (e.scsi_id == -1) target_str = L"PlusOS Shell";
        else if (e.scsi_id == -2) target_str = L"PlusOS Desktop";
        else target_str = L"Unknown (" + std::to_wstring(e.scsi_id) + L")";

        ListView_SetItemText(m_hListViewBoot, (int)i, 2, (LPWSTR)target_str.c_str());
    }
}

void MainWindow::populate_diagnostics_tab()
{
    if (!m_disk.is_open()) {
        SetWindowTextW(m_hEditDiagnostics, L"No disk image opened.");
        return;
    }

    std::wstringstream ss;
    ss << L"========================================================================\r\n";
    ss << L" PlusOS Disk Studio - Technical Image Diagnostics\r\n";
    ss << L"========================================================================\r\n\r\n";

    ss << L"[Disk Image Properties]\r\n";
    ss << L"Path: " << m_disk.get_path() << L"\r\n";
    ss << L"File Size: " << m_disk.get_file_size() << L" bytes ("
       << (m_disk.get_file_size() / (1024 * 1024)) << L" MB)\r\n";
    ss << L"Total Blocks (512B): " << m_disk.get_total_blocks() << L"\r\n";
    ss << L"Partition Scheme: " << (m_disk.has_partition_map() ? L"Apple Partition Map (APM)" : L"Raw / Floppy") << L"\r\n\r\n";

    ss << L"[Partitions]\r\n";
    for (const auto& p : m_disk.get_partitions()) {
        ss << L"  #" << p.index << L": " << to_wide(p.name) << L" (" << to_wide(p.type) << L")\r\n"
           << L"      Start Block: " << p.start_block << L"  Count: " << p.block_count
           << L" (" << (p.block_count * 512 / 1024) << L" KB)\r\n";
    }
    ss << L"\r\n";

    if (m_fs.is_mounted()) {
        ss << L"[HFS Volume Attributes]\r\n";
        ss << L"Volume Name: " << to_wide(m_fs.get_volume_name()) << L"\r\n";
        ss << L"Allocation Block Size: " << m_fs.get_alloc_block_size() << L" bytes\r\n";
        ss << L"Total Capacity: " << to_wide(format_size((uint32_t)m_fs.get_total_bytes())) << L"\r\n";
        ss << L"Free Space: " << to_wide(format_size((uint32_t)m_fs.get_free_bytes())) << L"\r\n";
        ss << L"Total Files: " << m_fs.get_total_files() << L"   Folders: " << m_fs.get_total_folders() << L"\r\n\r\n";

        ss << L"[PlusOS Operating System Installation Check]\r\n";
        // Check boot block
        uint8_t boot_sec[512];
        if (m_disk.read_hfs_blocks(0, 1, boot_sec)) {
            bool has_loader = (boot_sec[0] == 0x48 || boot_sec[0] == 0x60 || boot_sec[0] == 0x70 || boot_sec[0] == 0x4E);
            ss << L"Boot Blocks (0-1): " << (has_loader ? L"Installed (PlusOS Boot Loader present)" : L"Empty / Unknown") << L"\r\n";
        }

        // Check kernel block 60000
        uint8_t kern_sec[512];
        if (m_disk.read_hfs_blocks(60000, 1, kern_sec)) {
            bool has_kern = (kern_sec[4] == 'P' && kern_sec[5] == 'L' && kern_sec[6] == 'O' && kern_sec[7] == 'S');
            ss << L"Raw Kernel Area (block 60000): " << (has_kern ? L"Installed (PLOS Kernel Image valid)" : L"Empty / Not installed") << L"\r\n";
        }

        // Check raw app store block 61000
        uint8_t app_sec[512];
        if (m_disk.read_hfs_blocks(61000, 1, app_sec)) {
            bool has_apps = (app_sec[0] == 'P' && app_sec[1] == 'L' && app_sec[2] == 'D' && app_sec[3] == 'R');
            uint32_t napps = be32(*(uint32_t*)(app_sec + 4));
            ss << L"Raw Fallback App Store (block 61000): " << (has_apps ? (L"Present (" + std::to_wstring(napps) + L" apps)") : L"Empty") << L"\r\n";
        }
    }

    SetWindowTextW(m_hEditDiagnostics, ss.str().c_str());
}

void MainWindow::on_tree_selection_changed()
{
    HTREEITEM hSel = TreeView_GetSelection(m_hTreeView);
    if (!hSel) return;

    TVITEMW tvi = {};
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hSel;
    TreeView_GetItem(m_hTreeView, &tvi);

    m_current_folder_cnid = (uint32_t)tvi.lParam;
    populate_files_list(m_current_folder_cnid);
}

void MainWindow::on_tab_changed()
{
    m_current_tab = TabCtrl_GetCurSel(m_hTabControl);

    bool is_files = (m_current_tab == 0);
    bool is_boot = (m_current_tab == 1);
    bool is_diag = (m_current_tab == 2);

    ShowWindow(m_hStaticPath, is_files ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnUp, is_files ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hListViewFiles, is_files ? SW_SHOW : SW_HIDE);

    ShowWindow(m_hListViewBoot, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnBootAdd, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnBootEdit, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnBootRemove, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnBootUp, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnBootDown, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hComboBootPref, is_boot ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hBtnBootSave, is_boot ? SW_SHOW : SW_HIDE);

    ShowWindow(m_hEditDiagnostics, is_diag ? SW_SHOW : SW_HIDE);

    if (is_boot) populate_boot_tab();
    if (is_diag) populate_diagnostics_tab();
}

void MainWindow::open_image(const std::wstring& path)
{
    std::string err;
    if (!m_disk.open(path, false, &err)) {
        MessageBoxW(m_hWnd, to_wide(err).c_str(), L"Error Opening Disk Image", MB_ICONERROR | MB_OK);
        return;
    }

    if (!m_fs.mount(&m_disk, &err)) {
        MessageBoxW(m_hWnd, to_wide(err).c_str(), L"HFS Mount Error", MB_ICONERROR | MB_OK);
        m_disk.close();
        return;
    }

    populate_tree();
    navigate_to_folder(2);
    populate_boot_tab();
    populate_diagnostics_tab();
    update_status_bar();
}

void MainWindow::action_open_image()
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Macintosh Disk Images (*.hda;*.img;*.dsk;*.raw)\0*.hda;*.img;*.dsk;*.raw\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        open_image(szFile);
    }
}

void MainWindow::action_build_image()
{
    // Close current image first so the build tool has exclusive write access
    m_disk.close();
    m_fs.unmount();
    populate_tree();
    populate_files_list(2);
    update_status_bar();

    int res = MessageBoxW(m_hWnd,
        L"Build and compile the complete PlusOS hard drive image?\r\n\r\n"
        L"This will compile all PlusOS SDKs, kernel, bootloader, 48 applications,\r\n"
        L"and write them to images\\PlusOS.hda, then immediately open the image in Disk Studio.",
        L"Compile & Build Image", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);

    if (res != IDYES) return;

    // Show temporary status
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"Compiling and building image with the P68 toolchain... please wait");

    // Locate the repository root from the running executable: it sits either at <repo>\ or
    // at <repo>\tools\disk_studio\. We walk up until we find make.cmd next to Makefile.p68.
    std::wstring rootDir;
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring dir = exePath;
        size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos) dir = dir.substr(0, slash);
        for (int up = 0; up < 6 && !dir.empty(); ++up) {
            if (GetFileAttributesW((dir + L"\\make.cmd").c_str()) != INVALID_FILE_ATTRIBUTES &&
                GetFileAttributesW((dir + L"\\Makefile.p68").c_str()) != INVALID_FILE_ATTRIBUTES) {
                rootDir = dir;
                break;
            }
            size_t s2 = dir.find_last_of(L"\\/");
            if (s2 == std::wstring::npos) break;
            dir = dir.substr(0, s2);
        }
    }
    if (rootDir.empty()) {
        MessageBoxW(m_hWnd,
            L"Could not locate the PlusOS repository (make.cmd / Makefile.p68).\r\n\r\n"
            L"Run Disk Studio from inside the PlusOS folder.",
            L"Repository Not Found", MB_ICONERROR | MB_OK);
        return;
    }
    if (GetFileAttributesW((rootDir + L"\\toolchain\\bin\\p68-cc.exe").c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(m_hWnd,
            L"The PlusOS native compilers are not built yet.\r\n\r\n"
            L"Open the OS Builder and click 'Setup Build Environment', or run toolchain\\build.cmd.",
            L"P68 Toolchain Required", MB_ICONWARNING | MB_OK);
        return;
    }

    // Build natively with our own tools: make.cmd compiles the OS (p68-cc/as/ld/ar) and
    // produces the hard-drive image through tools\hdbuild.
    std::wstring cmd = L"cmd.exe /c \"cd /d \"" + rootDir + L"\" && call make.cmd image\"";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, rootDir.c_str(), &si, &pi);
    if (ok) {
        WaitForSingleObject(pi.hProcess, 600000); // up to 10 minutes
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        std::wstring img = rootDir + L"\\images\\PlusOS_p68.hda";
        if (exitCode == 0) {
            open_image(img);
            MessageBoxW(m_hWnd, L"Build succeeded! The hard drive image is now open in Disk Studio.",
                        L"Build Successful", MB_ICONINFORMATION | MB_OK);
        } else {
            MessageBoxW(m_hWnd, L"Build completed with errors. Check the build log.",
                        L"Build Notice", MB_ICONWARNING | MB_OK);
            if (GetFileAttributesW(img.c_str()) != INVALID_FILE_ATTRIBUTES) open_image(img);
        }
    } else {
        MessageBoxW(m_hWnd, L"Failed to start the build process.", L"Execution Error", MB_ICONERROR | MB_OK);
    }
}

void MainWindow::show_start_options_dialog()
{
    // Clean Welcome / Quick Start options box
    int choice = MessageBoxW(m_hWnd,
        L"Welcome to PlusOS Disk Studio!\r\n\r\n"
        L"Choose an option:\r\n"
        L"  [YES]    🔨 Compile & Build fresh PlusOS hard drive image\r\n"
        L"  [NO]     ⚡ Open default PlusOS hard drive image (images\\PlusOS.hda)\r\n"
        L"  [CANCEL] 📂 Browse for an existing disk image (.hda / .dsk)\r\n",
        L"PlusOS Disk Studio - Start Options", MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1);

    if (choice == IDYES) {
        action_build_image();
    } else if (choice == IDNO) {
        std::wstring defaultImg = L"C:\\Users\\Evil\\PlusOS\\images\\PlusOS.hda";
        if (GetFileAttributesW(defaultImg.c_str()) != INVALID_FILE_ATTRIBUTES) {
            open_image(defaultImg);
        } else {
            action_open_image();
        }
    } else if (choice == IDCANCEL) {
        action_open_image();
    }
}

void MainWindow::action_inject_current()
{
    if (!m_fs.is_mounted()) {
        MessageBoxW(m_hWnd, L"Please open a hard drive image first.", L"Inject Return to PlusOS", MB_ICONWARNING | MB_OK);
        return;
    }

    std::string msg;
    if (m_fs.inject_return_to_plusos(6, &msg)) {
        MessageBoxW(m_hWnd, to_wide(msg).c_str(), L"Injection Succeeded", MB_ICONINFORMATION | MB_OK);
        action_refresh();
    } else {
        MessageBoxW(m_hWnd, to_wide(msg).c_str(), L"Injection Failed", MB_ICONERROR | MB_OK);
    }
}

void MainWindow::action_inject_external()
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Hard Drive & Floppy Images (*.hda;*.img;*.dsk;*.image)\0*.hda;*.img;*.dsk;*.image\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Select Target Hard Drive Image or Floppy Drive Image";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::string msg;
        if (HFSFileSystem::inject_into_external_image(szFile, 6, &msg)) {
            MessageBoxW(m_hWnd, to_wide(msg).c_str(), L"Injection Succeeded", MB_ICONINFORMATION | MB_OK);
            if (m_disk.get_path() == szFile) action_refresh();
        } else {
            MessageBoxW(m_hWnd, to_wide(msg).c_str(), L"Injection Failed", MB_ICONERROR | MB_OK);
        }
    }
}

void MainWindow::on_drop_files(HDROP hDrop)
{
    if (!m_fs.is_mounted()) {
        DragFinish(hDrop);
        MessageBoxW(m_hWnd, L"Open a hard drive image before dropping files.", L"PlusOS Disk Studio", MB_ICONWARNING | MB_OK);
        return;
    }

    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    int imported = 0;

    for (UINT i = 0; i < count; ++i) {
        WCHAR szPath[MAX_PATH];
        DragQueryFileW(hDrop, i, szPath, MAX_PATH);

        std::ifstream f(szPath, std::ios::binary);
        if (!f.is_open()) continue;

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        // Extract filename
        std::wstring wpath(szPath);
        size_t last_slash = wpath.find_last_of(L"\\/");
        std::wstring wname = (last_slash != std::wstring::npos) ? wpath.substr(last_slash + 1) : wpath;
        std::string name = to_narrow(wname);

        uint32_t type = str_to_fourcc("????");
        uint32_t creator = str_to_fourcc("????");
        std::vector<uint8_t> df = data;
        std::vector<uint8_t> rf;
        uint16_t flags = 0;

        // Auto-detect format by extension
        size_t dot = name.find_last_of('.');
        std::string ext = (dot != std::string::npos) ? name.substr(dot) : "";
        for (char& c : ext) c = (char)tolower(c);

        if (ext == ".pex") {
            type = str_to_fourcc("PLOS");
            creator = str_to_fourcc("PLOS");
        } else if (ext == ".txt" || ext == ".cfg" || ext == ".md" || ext == ".c" || ext == ".h") {
            type = str_to_fourcc("TEXT");
            creator = str_to_fourcc("ttxt");
        } else if (ext == ".bin" && data.size() >= 128 && data[0] == 0) {
            // Unpack MacBinary II
            const MacBinaryHeader* mb = (const MacBinaryHeader*)data.data();
            int nlen = mb->filenameLength;
            if (nlen > 0 && nlen <= 63) {
                name = std::string(mb->filename, nlen);
                type = be32(mb->fileType);
                creator = be32(mb->fileCreator);
                flags = (uint16_t)((mb->finderFlagsHigh << 8) | mb->finderFlagsLow);
                uint32_t dlen = be32(mb->dataForkLength);
                uint32_t rlen = be32(mb->resourceForkLength);

                uint32_t doff = 128;
                uint32_t roff = doff + ((dlen + 127) / 128 * 128);

                df.clear();
                if (doff + dlen <= data.size()) {
                    df.assign(data.begin() + doff, data.begin() + doff + dlen);
                }

                rf.clear();
                if (roff + rlen <= data.size()) {
                    rf.assign(data.begin() + roff, data.begin() + roff + rlen);
                }
            }
        }

        if (m_fs.import_file(m_current_folder_cnid, name, type, creator, df, rf, flags)) {
            imported++;
        }
    }

    DragFinish(hDrop);

    if (imported > 0) {
        action_refresh();
        std::wstring s = L"Successfully imported " + std::to_wstring(imported) + L" file(s) into current directory.";
        MessageBoxW(m_hWnd, s.c_str(), L"Import Complete", MB_ICONINFORMATION | MB_OK);
    }
}

void MainWindow::action_import_files()
{
    if (!m_fs.is_mounted()) {
        MessageBoxW(m_hWnd, L"Please open a hard drive image first.", L"Import Files", MB_ICONWARNING | MB_OK);
        return;
    }

    WCHAR szFiles[4096] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFiles;
    ofn.nMaxFile = sizeof(szFiles) / sizeof(szFiles[0]);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0MacBinary II (*.bin)\0*.bin\0PlusOS Executables (*.pex)\0*.pex\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameW(&ofn)) {
        // Parse multi-select files
        std::wstring dir = szFiles;
        WCHAR* p = szFiles + dir.length() + 1;
        int count = 0;

        if (*p == 0) { // Single file selected
            std::ifstream f(dir, std::ios::binary);
            if (f.is_open()) {
                std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                f.close();
                size_t slash = dir.find_last_of(L"\\/");
                std::string fname = to_narrow((slash != std::wstring::npos) ? dir.substr(slash + 1) : dir);
                if (m_fs.import_file(m_current_folder_cnid, fname, str_to_fourcc("????"), str_to_fourcc("????"), data)) count++;
            }
        } else { // Multiple files
            while (*p) {
                std::wstring fname = p;
                std::wstring fullPath = dir + L"\\" + fname;
                std::ifstream f(fullPath, std::ios::binary);
                if (f.is_open()) {
                    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    f.close();
                    if (m_fs.import_file(m_current_folder_cnid, to_narrow(fname), str_to_fourcc("????"), str_to_fourcc("????"), data)) count++;
                }
                p += fname.length() + 1;
            }
        }

        if (count > 0) {
            action_refresh();
            MessageBoxW(m_hWnd, (L"Imported " + std::to_wstring(count) + L" files.").c_str(), L"Import Finished", MB_ICONINFORMATION | MB_OK);
        }
    }
}

void MainWindow::action_export_selected_file()
{
    int sel = ListView_GetNextItem(m_hListViewFiles, -1, LVNI_SELECTED);
    if (sel < 0) {
        MessageBoxW(m_hWnd, L"Please select a file to export.", L"Export File", MB_ICONWARNING | MB_OK);
        return;
    }

    LVITEMW item = {};
    item.mask = LVIF_PARAM;
    item.iItem = sel;
    ListView_GetItem(m_hListViewFiles, &item);

    const HFSItem* fitem = m_fs.find_by_cnid((uint32_t)item.lParam);
    if (!fitem || fitem->is_directory) {
        MessageBoxW(m_hWnd, L"Folders cannot be directly exported as a single file.", L"Export File", MB_ICONWARNING | MB_OK);
        return;
    }

    WCHAR szFile[MAX_PATH];
    wcscpy_s(szFile, to_wide(fitem->name).c_str());

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        std::vector<uint8_t> data;
        if (m_fs.read_file_fork(*fitem, false, data)) {
            std::ofstream out(szFile, std::ios::binary);
            if (out.is_open()) {
                out.write((const char*)data.data(), data.size());
                out.close();
                MessageBoxW(m_hWnd, L"File successfully exported to PC!", L"Export Complete", MB_ICONINFORMATION | MB_OK);
            }
        }
    }
}

void MainWindow::action_delete_selected()
{
    int sel = ListView_GetNextItem(m_hListViewFiles, -1, LVNI_SELECTED);
    if (sel < 0) return;

    LVITEMW item = {};
    item.mask = LVIF_PARAM;
    item.iItem = sel;
    ListView_GetItem(m_hListViewFiles, &item);

    if (MessageBoxW(m_hWnd, L"Are you sure you want to delete the selected item from the disk image?",
                    L"Confirm Delete", MB_ICONQUESTION | MB_YESNO) == IDYES) {
        m_fs.delete_item((uint32_t)item.lParam);
        action_refresh();
    }
}

void MainWindow::action_refresh()
{
    populate_tree();
    navigate_to_folder(m_current_folder_cnid);
    populate_boot_tab();
    populate_diagnostics_tab();
    update_status_bar();
}

void MainWindow::boot_add_entry()
{
    MultiBootEntry e;
    e.name = "Custom Operating System";
    e.scsi_id = 0;
    m_boot_entries.push_back(e);

    populate_boot_tab();
    ListView_SetItemState(m_hListViewBoot, (int)m_boot_entries.size() - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    boot_edit_entry();
}

void MainWindow::boot_edit_entry()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_boot_entries.size()) return;

    MultiBootEntry& e = m_boot_entries[sel];

    // Cycle target ID: 0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> Shell (-1) -> Desktop (-2) -> 0
    if (e.scsi_id >= 0 && e.scsi_id < 7) e.scsi_id++;
    else if (e.scsi_id == 7) e.scsi_id = -1;
    else if (e.scsi_id == -1) e.scsi_id = -2;
    else e.scsi_id = 0;

    populate_boot_tab();
    ListView_SetItemState(m_hListViewBoot, sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void MainWindow::boot_remove_entry()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_boot_entries.size()) return;

    m_boot_entries.erase(m_boot_entries.begin() + sel);
    populate_boot_tab();
}

void MainWindow::boot_move_up()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel <= 0 || sel >= (int)m_boot_entries.size()) return;

    std::swap(m_boot_entries[sel], m_boot_entries[sel - 1]);
    populate_boot_tab();
    ListView_SetItemState(m_hListViewBoot, sel - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void MainWindow::boot_move_down()
{
    int sel = ListView_GetNextItem(m_hListViewBoot, -1, LVNI_SELECTED);
    if (sel < 0 || sel + 1 >= (int)m_boot_entries.size()) return;

    std::swap(m_boot_entries[sel], m_boot_entries[sel + 1]);
    populate_boot_tab();
    ListView_SetItemState(m_hListViewBoot, sel + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void MainWindow::boot_save_config()
{
    if (!m_fs.is_mounted()) {
        MessageBoxW(m_hWnd, L"Please open a hard drive image first.", L"Save Multi-Boot", MB_ICONWARNING | MB_OK);
        return;
    }

    int pref = (int)SendMessageW(m_hComboBootPref, CB_GETCURSEL, 0, 0);
    if (pref < 0 || pref > 2) pref = 2;

    if (m_fs.save_multiboot_config(m_boot_entries, pref)) {
        MessageBoxW(m_hWnd, L"Multi-Boot configuration saved to disk image successfully (multiboot.cfg)!",
                    L"Configuration Saved", MB_ICONINFORMATION | MB_OK);
        action_refresh();
    } else {
        MessageBoxW(m_hWnd, L"Failed to save multiboot.cfg to disk image.", L"Save Error", MB_ICONERROR | MB_OK);
    }
}

LRESULT MainWindow::handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_SIZE:
        layout_controls(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_DROPFILES:
        on_drop_files((HDROP)wParam);
        return 0;

    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lParam;
        if (nm->idFrom == IDC_TREEVIEW && nm->code == TVN_SELCHANGEDW) {
            on_tree_selection_changed();
            return 0;
        }
        if (nm->idFrom == IDC_TABCONTROL && nm->code == TCN_SELCHANGE) {
            on_tab_changed();
            return 0;
        }
        if (nm->idFrom == IDC_LISTVIEW_FILES && nm->code == NM_DBLCLK) {
            int sel = ListView_GetNextItem(m_hListViewFiles, -1, LVNI_SELECTED);
            if (sel >= 0) {
                LVITEMW itm = {};
                itm.mask = LVIF_PARAM;
                itm.iItem = sel;
                ListView_GetItem(m_hListViewFiles, &itm);
                uint32_t target_cnid = (uint32_t)itm.lParam;

                const HFSItem* f = m_fs.find_by_cnid(target_cnid);
                if (f && f->is_directory) {
                    navigate_to_folder(target_cnid);
                } else if (f) {
                    // Show file preview
                    std::vector<uint8_t> data;
                    if (m_fs.read_file_fork(*f, false, data)) {
                        std::wstring info = L"File: " + to_wide(f->name) + L"\r\n" +
                                            L"Type: " + to_wide(fourcc_to_str(f->type)) +
                                            L"  Creator: " + to_wide(fourcc_to_str(f->creator)) + L"\r\n" +
                                            L"Size: " + std::to_wstring(data.size()) + L" bytes\r\n\r\n";
                        std::string preview_text((char*)data.data(), (std::min)((size_t)data.size(), (size_t)1024));
                        info += to_wide(preview_text);
                        MessageBoxW(hWnd, info.c_str(), L"File Preview", MB_OK);
                    }
                }
            }
            return 0;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case ID_ACTION_BUILD_IMAGE: action_build_image(); break;
        case ID_FILE_OPEN_IMAGE: action_open_image(); break;
        case ID_ACTION_START_OPTIONS: show_start_options_dialog(); break;
        case IDC_BTN_UP: {
            if (m_current_folder_cnid != 2) {
                const HFSItem* cur = m_fs.find_by_cnid(m_current_folder_cnid);
                uint32_t parent_id = cur ? cur->parent_cnid : 2;
                navigate_to_folder(parent_id);
            }
            break;
        }
        case ID_ACTION_INJECT_CURRENT: action_inject_current(); break;
        case ID_ACTION_INJECT_EXTERNAL: action_inject_external(); break;
        case ID_ACTION_IMPORT_FILES: action_import_files(); break;
        case ID_ACTION_EXPORT_FILE: action_export_selected_file(); break;
        case ID_ACTION_DELETE: action_delete_selected(); break;
        case ID_ACTION_REFRESH: action_refresh(); break;

        case IDC_BTN_BOOT_ADD: boot_add_entry(); break;
        case IDC_BTN_BOOT_EDIT: boot_edit_entry(); break;
        case IDC_BTN_BOOT_REMOVE: boot_remove_entry(); break;
        case IDC_BTN_BOOT_UP: boot_move_up(); break;
        case IDC_BTN_BOOT_DOWN: boot_move_down(); break;
        case IDC_BTN_BOOT_SAVE: boot_save_config(); break;

        case ID_FILE_EXIT: DestroyWindow(hWnd); break;
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_F5) {
            action_refresh();
            return 0;
        }
        if (wParam == VK_DELETE) {
            action_delete_selected();
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
