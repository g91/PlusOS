#include "explorer_window.h"
#include "builder_window.h"
#include <commdlg.h>
#include <shellapi.h>
#include <sstream>
#include <fstream>
#include <iomanip>
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

ExplorerWindow::ExplorerWindow()
    : m_hInstance(nullptr), m_hWnd(nullptr), m_builderWin(nullptr),
      m_hToolbar(nullptr), m_hStatusBar(nullptr), m_hStaticPath(nullptr),
      m_hBtnUp(nullptr), m_hTreeView(nullptr), m_hListViewFiles(nullptr),
      m_current_folder_cnid(2)
{
}

ExplorerWindow::~ExplorerWindow()
{
}

bool ExplorerWindow::create(HINSTANCE hInstance, BuilderWindow* builderWin)
{
    m_hInstance = hInstance;
    m_builderWin = builderWin;

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PlusOSExplorerClass";
    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        wc.lpszClassName,
        L"PlusOS Hard Drive Explorer & Image Manager",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1020, 700,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hWnd) return false;

    create_controls();
    return true;
}

void ExplorerWindow::show(const std::wstring& imagePath)
{
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_SHOW);
        SetForegroundWindow(m_hWnd);
        if (!imagePath.empty() && m_disk.get_path() != imagePath) {
            open_image(imagePath);
        } else if (imagePath.empty() && !m_fs.is_mounted()) {
            open_image(L"C:\\Users\\Evil\\PlusOS\\images\\PlusOS.hda");
        }
    }
}

void ExplorerWindow::hide()
{
    if (m_hWnd) ShowWindow(m_hWnd, SW_HIDE);
}

void ExplorerWindow::create_controls()
{
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // Toolbar
    m_hToolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP | TBSTYLE_WRAPABLE,
        0, 0, 0, 0, m_hWnd, (HMENU)IDE_TOOLBAR, m_hInstance, nullptr);
    SendMessageW(m_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON buttons[] = {
        { I_IMAGENONE, IDE_SWITCH_BUILDER, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Switch to OS Builder " },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, IDE_OPEN_IMAGE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Open Image " },
        { I_IMAGENONE, IDE_INJECT_RETURN_APP, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Inject Return to PlusOS " },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, IDE_IMPORT_FILES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Import Files... " },
        { I_IMAGENONE, IDE_EXPORT_FILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Export Selected " },
        { I_IMAGENONE, IDE_DELETE_ITEM, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Delete " },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, IDE_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L" Refresh (F5) " },
    };
    SendMessageW(m_hToolbar, TB_ADDBUTTONSW, sizeof(buttons) / sizeof(buttons[0]), (LPARAM)buttons);
    SendMessageW(m_hToolbar, TB_AUTOSIZE, 0, 0);

    // Path address bar & Up button
    m_hBtnUp = CreateWindowExW(0, L"BUTTON", L"Up",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 42, 60, 24, m_hWnd, (HMENU)IDE_BTN_UP, m_hInstance, nullptr);
    SendMessageW(m_hBtnUp, WM_SETFONT, (WPARAM)hFont, TRUE);

    m_hStaticPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L" Path: : (Volume Root)",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        75, 42, 900, 24, m_hWnd, (HMENU)IDE_STATIC_PATH, m_hInstance, nullptr);
    SendMessageW(m_hStaticPath, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Tree View
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        10, 72, 240, 500, m_hWnd, (HMENU)IDE_TREEVIEW, m_hInstance, nullptr);

    // List View (Files)
    m_hListViewFiles = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        255, 72, 740, 500, m_hWnd, (HMENU)IDE_LISTVIEW_FILES, m_hInstance, nullptr);
    SendMessageW(m_hListViewFiles, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 240; col.pszText = (LPWSTR)L"Name"; ListView_InsertColumn(m_hListViewFiles, 0, &col);
    col.cx = 70;  col.pszText = (LPWSTR)L"Type"; ListView_InsertColumn(m_hListViewFiles, 1, &col);
    col.cx = 70;  col.pszText = (LPWSTR)L"Creator"; ListView_InsertColumn(m_hListViewFiles, 2, &col);
    col.cx = 100; col.pszText = (LPWSTR)L"Data Size"; ListView_InsertColumn(m_hListViewFiles, 3, &col);
    col.cx = 100; col.pszText = (LPWSTR)L"Rsrc Size"; ListView_InsertColumn(m_hListViewFiles, 4, &col);
    col.cx = 70;  col.pszText = (LPWSTR)L"CNID"; ListView_InsertColumn(m_hListViewFiles, 5, &col);

    // Status Bar
    m_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hWnd, (HMENU)IDE_STATUSBAR, m_hInstance, nullptr);
    int statwidths[] = { 400, 650, -1 };
    SendMessageW(m_hStatusBar, SB_SETPARTS, sizeof(statwidths) / sizeof(statwidths[0]), (LPARAM)statwidths);

    update_status_bar();
}

void ExplorerWindow::layout_controls(int width, int height)
{
    SendMessageW(m_hToolbar, TB_AUTOSIZE, 0, 0);
    SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

    RECT rcTool, rcStat;
    GetWindowRect(m_hToolbar, &rcTool);
    GetWindowRect(m_hStatusBar, &rcStat);
    int toolH = rcTool.bottom - rcTool.top;
    int statH = rcStat.bottom - rcStat.top;

    int topY = toolH + 5;
    int pathH = 26;
    SetWindowPos(m_hBtnUp, nullptr, 10, topY, 60, pathH, SWP_NOZORDER);
    SetWindowPos(m_hStaticPath, nullptr, 75, topY, width - 85, pathH, SWP_NOZORDER);

    int contentY = topY + pathH + 6;
    int contentH = height - contentY - statH - 5;
    if (contentH < 50) contentH = 50;

    int treeW = 240;
    SetWindowPos(m_hTreeView, nullptr, 10, contentY, treeW, contentH, SWP_NOZORDER);
    SetWindowPos(m_hListViewFiles, nullptr, treeW + 15, contentY, width - treeW - 25, contentH, SWP_NOZORDER);
}

void ExplorerWindow::update_status_bar()
{
    if (!m_fs.is_mounted()) {
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"No hard drive image open.");
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 1, (LPARAM)L"");
        SendMessageW(m_hStatusBar, SB_SETTEXTW, 2, (LPARAM)L"Drag & drop files or click Open Image");
        return;
    }

    std::wstring s0 = L"Volume: " + to_wide(m_fs.get_volume_name());
    std::wstring s1 = L"Free: " + to_wide(format_size((uint32_t)m_fs.get_free_bytes())) + L" / " +
                      to_wide(format_size((uint32_t)m_fs.get_total_bytes()));
    std::wstring s2 = L"Files: " + std::to_wstring(m_fs.get_total_files()) + L"  Folders: " +
                      std::to_wstring(m_fs.get_total_folders());

    SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)s0.c_str());
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 1, (LPARAM)s1.c_str());
    SendMessageW(m_hStatusBar, SB_SETTEXTW, 2, (LPARAM)s2.c_str());
}

void ExplorerWindow::open_image(const std::wstring& path)
{
    m_disk.close();
    m_fs.unmount();

    if (!m_disk.open(path)) {
        MessageBoxW(m_hWnd, (L"Failed to open image:\n" + path).c_str(), L"Open Error", MB_ICONERROR | MB_OK);
        update_status_bar();
        return;
    }

    std::string err;
    if (!m_fs.mount(&m_disk, &err)) {
        MessageBoxW(m_hWnd, (L"Failed to mount HFS filesystem:\n" + to_wide(err)).c_str(), L"HFS Mount Error", MB_ICONERROR | MB_OK);
        m_disk.close();
        update_status_bar();
        return;
    }

    m_current_folder_cnid = 2; // Root
    populate_tree();
    navigate_to_folder(2);
    update_status_bar();
}

static void add_tree_nodes(HWND hTree, HTREEITEM hParent, const HFSItem& dir, const HFSFileSystem& fs)
{
    auto children = fs.get_children(dir.cnid);
    for (const auto& item : children) {
        if (item.is_directory) {
            TVINSERTSTRUCTW tvis = {};
            tvis.hParent = hParent;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
            std::wstring nameW = to_wide(item.name);
            tvis.item.pszText = (LPWSTR)nameW.c_str();
            tvis.item.lParam = (LPARAM)item.cnid;
            HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
            add_tree_nodes(hTree, hItem, item, fs);
        }
    }
}

void ExplorerWindow::populate_tree()
{
    TreeView_DeleteAllItems(m_hTreeView);
    if (!m_fs.is_mounted()) return;

    const HFSItem* root = m_fs.get_root();
    if (!root) return;

    TVINSERTSTRUCTW tvis = {};
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    tvis.item.state = TVIS_EXPANDED;
    tvis.item.stateMask = TVIS_EXPANDED;
    std::wstring rootName = to_wide(m_fs.get_volume_name()) + L" (Root)";
    tvis.item.pszText = (LPWSTR)rootName.c_str();
    tvis.item.lParam = (LPARAM)2;

    HTREEITEM hRoot = TreeView_InsertItem(m_hTreeView, &tvis);
    add_tree_nodes(m_hTreeView, hRoot, *root, m_fs);
    TreeView_Expand(m_hTreeView, hRoot, TVE_EXPAND);
}

void ExplorerWindow::populate_files_list(uint32_t folder_cnid)
{
    ListView_DeleteAllItems(m_hListViewFiles);
    if (!m_fs.is_mounted()) return;

    auto children = m_fs.get_children(folder_cnid);
    for (size_t i = 0; i < children.size(); i++) {
        const auto& item = children[i];
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)i;
        lvi.lParam = (LPARAM)item.cnid;

        std::wstring nameW = to_wide(item.name);
        lvi.pszText = (LPWSTR)nameW.c_str();
        ListView_InsertItem(m_hListViewFiles, &lvi);

        std::wstring typeStr = item.is_directory ? L"Folder" : to_wide(fourcc_to_str(item.type));
        std::wstring creatorStr = item.is_directory ? L"" : to_wide(fourcc_to_str(item.creator));
        std::wstring dataSz = item.is_directory ? L"" : to_wide(format_size(item.data_size));
        std::wstring rsrcSz = (item.is_directory || item.rsrc_size == 0) ? L"" : to_wide(format_size(item.rsrc_size));
        std::wstring cnidStr = std::to_wstring(item.cnid);

        ListView_SetItemText(m_hListViewFiles, (int)i, 1, (LPWSTR)typeStr.c_str());
        ListView_SetItemText(m_hListViewFiles, (int)i, 2, (LPWSTR)creatorStr.c_str());
        ListView_SetItemText(m_hListViewFiles, (int)i, 3, (LPWSTR)dataSz.c_str());
        ListView_SetItemText(m_hListViewFiles, (int)i, 4, (LPWSTR)rsrcSz.c_str());
        ListView_SetItemText(m_hListViewFiles, (int)i, 5, (LPWSTR)cnidStr.c_str());
    }
}

std::wstring ExplorerWindow::get_folder_path(uint32_t cnid) const
{
    if (cnid <= 2) return L": (Root)";
    std::wstring path = L"";
    uint32_t cur = cnid;
    while (cur > 2) {
        const HFSItem* itm = m_fs.find_by_cnid(cur);
        if (!itm) break;
        path = L":" + to_wide(itm->name) + path;
        cur = itm->parent_cnid;
    }
    return path.empty() ? L":" : path;
}

void ExplorerWindow::navigate_to_folder(uint32_t folder_cnid)
{
    m_current_folder_cnid = folder_cnid;
    std::wstring pathText = L" Path: " + get_folder_path(folder_cnid);
    SetWindowTextW(m_hStaticPath, pathText.c_str());
    populate_files_list(folder_cnid);
}

void ExplorerWindow::on_tree_selection_changed()
{
    HTREEITEM hSel = TreeView_GetSelection(m_hTreeView);
    if (!hSel) return;

    TVITEMW tvi = {};
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hSel;
    TreeView_GetItem(m_hTreeView, &tvi);

    uint32_t cnid = (uint32_t)tvi.lParam;
    navigate_to_folder(cnid);
}

void ExplorerWindow::on_drop_files(HDROP hDrop)
{
    if (!m_fs.is_mounted()) {
        DragFinish(hDrop);
        MessageBoxW(m_hWnd, L"Please open a hard drive image before dropping files.", L"PlusOS Explorer", MB_ICONWARNING | MB_OK);
        return;
    }

    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    int imported = 0;

    for (UINT i = 0; i < count; i++) {
        WCHAR szPath[MAX_PATH];
        DragQueryFileW(hDrop, i, szPath, MAX_PATH);

        std::ifstream in(szPath, std::ios::binary);
        if (!in.is_open()) continue;
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        std::wstring filename = szPath;
        size_t last_slash = filename.find_last_of(L"\\/");
        if (last_slash != std::wstring::npos) filename = filename.substr(last_slash + 1);

        uint32_t type = str_to_fourcc("TEXT");
        uint32_t creator = str_to_fourcc("ttxt");

        // Inspect header or extension for type
        if (data.size() >= 8 && data[4] == 'P' && data[5] == 'L' && data[6] == 'O' && data[7] == 'S') {
            type = str_to_fourcc("PLOS");
            creator = str_to_fourcc("PLOS");
        } else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == L".pex") {
            type = str_to_fourcc("PLOS");
            creator = str_to_fourcc("PLOS");
            filename = filename.substr(0, filename.size() - 4);
        }

        std::string mac_name = to_narrow(filename);
        if (mac_name.size() > 31) mac_name = mac_name.substr(0, 31);

        if (m_fs.import_file(m_current_folder_cnid, mac_name, type, creator, data)) {
            imported++;
        }
    }

    DragFinish(hDrop);
    action_refresh();

    if (imported > 0) {
        MessageBoxW(m_hWnd, (std::to_wstring(imported) + L" file(s) imported into disk image successfully!").c_str(),
                    L"Import Complete", MB_ICONINFORMATION | MB_OK);
    }
}

void ExplorerWindow::action_open_image()
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Hard Drive & Floppy Images (*.hda;*.img;*.dsk)\0*.hda;*.img;*.dsk\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Open Macintosh / PlusOS Disk Image";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        open_image(szFile);
    }
}

void ExplorerWindow::action_switch_to_builder()
{
    if (m_builderWin) {
        m_builderWin->show();
    }
}

void ExplorerWindow::action_inject_current()
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

void ExplorerWindow::action_import_files()
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
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0PlusOS Executables (*.pex)\0*.pex\0Text Files (*.txt;*.cfg)\0*.txt;*.cfg\0";
    ofn.lpstrTitle = L"Select Files to Import into Disk Image";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameW(&ofn)) {
        WCHAR* p = szFiles;
        std::wstring dir = p;
        p += dir.length() + 1;

        int imported = 0;
        if (*p == 0) {
            // Single file
            std::wstring filename = dir;
            size_t ls = filename.find_last_of(L"\\/");
            if (ls != std::wstring::npos) filename = filename.substr(ls + 1);

            std::ifstream in(dir, std::ios::binary);
            if (in.is_open()) {
                std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                uint32_t type = str_to_fourcc("TEXT");
                uint32_t creator = str_to_fourcc("ttxt");
                if (data.size() >= 8 && data[4] == 'P' && data[5] == 'L' && data[6] == 'O' && data[7] == 'S') {
                    type = str_to_fourcc("PLOS"); creator = str_to_fourcc("PLOS");
                }
                std::string mac_name = to_narrow(filename);
                if (mac_name.size() > 31) mac_name = mac_name.substr(0, 31);
                if (m_fs.import_file(m_current_folder_cnid, mac_name, type, creator, data)) imported++;
            }
        } else {
            // Multiple files
            while (*p) {
                std::wstring fname = p;
                std::wstring fullPath = dir + L"\\" + fname;
                p += fname.length() + 1;

                std::ifstream in(fullPath, std::ios::binary);
                if (in.is_open()) {
                    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                    in.close();
                    uint32_t type = str_to_fourcc("TEXT");
                    uint32_t creator = str_to_fourcc("ttxt");
                    if (data.size() >= 8 && data[4] == 'P' && data[5] == 'L' && data[6] == 'O' && data[7] == 'S') {
                        type = str_to_fourcc("PLOS"); creator = str_to_fourcc("PLOS");
                    }
                    std::string mac_name = to_narrow(fname);
                    if (mac_name.size() > 31) mac_name = mac_name.substr(0, 31);
                    if (m_fs.import_file(m_current_folder_cnid, mac_name, type, creator, data)) imported++;
                }
            }
        }
        action_refresh();
        if (imported > 0) {
            MessageBoxW(m_hWnd, (std::to_wstring(imported) + L" file(s) imported successfully!").c_str(),
                        L"Import Complete", MB_ICONINFORMATION | MB_OK);
        }
    }
}

void ExplorerWindow::action_export_selected_file()
{
    int sel = ListView_GetNextItem(m_hListViewFiles, -1, LVNI_SELECTED);
    if (sel < 0) return;

    LVITEMW item = {};
    item.mask = LVIF_PARAM;
    item.iItem = sel;
    ListView_GetItem(m_hListViewFiles, &item);
    uint32_t cnid = (uint32_t)item.lParam;

    const HFSItem* fitem = m_fs.find_by_cnid(cnid);
    if (!fitem || fitem->is_directory) return;

    WCHAR szFile[MAX_PATH];
    std::wstring defName = to_wide(fitem->name);
    wcsncpy_s(szFile, defName.c_str(), MAX_PATH);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Export File to PC";
    ofn.Flags = OFN_OVERWRITEPROMPT;

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

void ExplorerWindow::action_delete_selected()
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

void ExplorerWindow::action_refresh()
{
    populate_tree();
    navigate_to_folder(m_current_folder_cnid);
    update_status_bar();
}

void ExplorerWindow::action_preview_file(uint32_t cnid)
{
    const HFSItem* f = m_fs.find_by_cnid(cnid);
    if (!f) return;

    if (f->is_directory) {
        navigate_to_folder(cnid);
        return;
    }

    std::vector<uint8_t> data;
    if (m_fs.read_file_fork(*f, false, data)) {
        std::wstring info = L"File: " + to_wide(f->name) + L"\r\n" +
                            L"Type: " + to_wide(fourcc_to_str(f->type)) +
                            L"  Creator: " + to_wide(fourcc_to_str(f->creator)) + L"\r\n" +
                            L"Size: " + std::to_wstring(data.size()) + L" bytes\r\n\r\n";
        std::string preview_text((char*)data.data(), (std::min)((size_t)data.size(), (size_t)1024));
        info += to_wide(preview_text);
        MessageBoxW(m_hWnd, info.c_str(), L"File Preview", MB_OK);
    }
}

LRESULT CALLBACK ExplorerWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ExplorerWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        pThis = (ExplorerWindow*)cs->lpCreateParams;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;
    } else {
        pThis = (ExplorerWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    }

    if (pThis) return pThis->handle_message(hWnd, msg, wParam, lParam);
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT ExplorerWindow::handle_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        if (nm->idFrom == IDE_TREEVIEW && nm->code == TVN_SELCHANGEDW) {
            on_tree_selection_changed();
            return 0;
        }
        if (nm->idFrom == IDE_LISTVIEW_FILES && nm->code == NM_DBLCLK) {
            int sel = ListView_GetNextItem(m_hListViewFiles, -1, LVNI_SELECTED);
            if (sel >= 0) {
                LVITEMW itm = {};
                itm.mask = LVIF_PARAM;
                itm.iItem = sel;
                ListView_GetItem(m_hListViewFiles, &itm);
                action_preview_file((uint32_t)itm.lParam);
            }
            return 0;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case IDE_SWITCH_BUILDER: action_switch_to_builder(); break;
        case IDE_OPEN_IMAGE: action_open_image(); break;
        case IDE_BTN_UP: {
            if (m_current_folder_cnid != 2) {
                const HFSItem* cur = m_fs.find_by_cnid(m_current_folder_cnid);
                uint32_t parent_id = cur ? cur->parent_cnid : 2;
                navigate_to_folder(parent_id);
            }
            break;
        }
        case IDE_INJECT_RETURN_APP: action_inject_current(); break;
        case IDE_IMPORT_FILES: action_import_files(); break;
        case IDE_EXPORT_FILE: action_export_selected_file(); break;
        case IDE_DELETE_ITEM: action_delete_selected(); break;
        case IDE_REFRESH: action_refresh(); break;
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_F5) {
            action_refresh();
            return 0;
        }
        break;

    case WM_CLOSE:
        // Hiding rather than destroying so builder window remains alive
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
