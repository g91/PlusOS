#include "builder_window.h"
#include "explorer_window.h"
#include <commdlg.h>
#include <shellapi.h>
#include <wininet.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <thread>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "wininet.lib")

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

struct AppPackageMeta {
    const char* name;
    const char* category;
    const char* description;
    uint32_t fallback_size;
};

static const AppPackageMeta ALL_BUILTIN_APPS[] = {
    // System
    { "BOOT", "System", "Startup disk selector & SCSI chainbooter", 6668 },
    { "DESKTOP", "System", "Graphical window manager & theme compositor", 62208 },
    { "INSTALLER", "System", "PlusOS Hard Drive Installation Utility", 10164 },
    { "OFF", "System", "Safe power off & shutdown utility", 508 },
    { "PKGMGR", "System", "Graphical software center package manager", 30372 },
    { "REBOOT", "System", "Warm system reboot utility", 508 },
    { "RET", "System", "Return to Mac OS from PlusOS", 492 },
    { "SHELL", "System", "Interactive command line interpreter", 8044 },
    { "STARTUP", "System", "Configure default boot application", 6404 },

    // Tools
    { "CALC", "Tools", "Desktop floating-point calculator", 13908 },
    { "CAT", "Tools", "Concatenate and print text files", 6688 },
    { "CLEAR", "Tools", "Clear terminal console screen", 508 },
    { "CLIP", "Tools", "Clipboard scrap viewer and text buffer", 7268 },
    { "DATE", "Tools", "Display or set current date and time", 6448 },
    { "DMESG", "Tools", "Dump kernel boot and driver log messages", 6248 },
    { "ECHO", "Tools", "Print arguments to console", 6092 },
    { "FILES", "Tools", "Interactive dual-pane HFS file manager", 18648 },
    { "GC", "Tools", "Kernel garbage collector stats and cycle", 6848 },
    { "HELP", "Tools", "List all commands and user guides", 7044 },
    { "HEXEDIT", "Tools", "Hexadecimal binary file editor & viewer", 11440 },
    { "KILL", "Tools", "Terminate process by PID", 6528 },
    { "LS", "Tools", "List files and directories", 6940 },
    { "MEM", "Tools", "Display free and used RAM statistics", 6076 },
    { "NOTES", "Tools", "Notepad text editor with persistent storage", 14876 },
    { "PS", "Tools", "List running processes and thread states", 6388 },
    { "RAM", "Tools", "Display RAM memory layout and heap bars", 7284 },
    { "REG", "Tools", "System registry viewer and modifier", 6732 },
    { "SLEEP", "Tools", "Delay execution for given seconds", 928 },
    { "SOUND", "Tools", "4-voice PWM audio synthesizer test", 7912 },
    { "SYSINFO", "Tools", "CPU, ROM, screen, and hardware audit", 6424 },
    { "TASKMAN", "Tools", "Live CPU and memory usage task manager", 12440 },
    { "THEME", "Tools", "Multi-OS visual theme selector", 8816 },
    { "TOP", "Tools", "Live system monitor and process viewer", 7648 },
    { "UPTIME", "Tools", "Show system running time and ticks", 6732 },
    { "VER", "Tools", "Display PlusOS kernel and SDK version", 6076 },

    // Games
    { "ASTEROIDS", "Games", "Vector Asteroids arcade game with physics", 10036 },
    { "BREAKOUT", "Games", "Paddle and brick breakout arcade game", 11264 },
    { "DUNGEON", "Games", "Roguelike dungeon crawler adventure RPG", 11360 },
    { "FLAPPY", "Games", "Flappy Mac side-scrolling arcade challenge", 15280 },
    { "INVADERS", "Games", "Space Invaders arcade defense game", 6120 },
    { "LUNAR", "Games", "Lunar Lander physics simulation", 8512 },
    { "MINES", "Games", "Minesweeper puzzle game", 6600 },
    { "PACMAN", "Games", "Classic maze ghost-chase arcade game", 5228 },
    { "PUZZLE", "Games", "15-tile sliding number puzzle DA", 12680 },
    { "RACER", "Games", "Top-down retro road racing game", 13636 },
    { "SNAKE", "Games", "Classic nibbles snake arcade game", 5112 },
    { "STARFLIGHT", "Games", "Deep space exploration and galaxy combat RPG", 17112 },
    { "TETRIS", "Games", "Classic falling block puzzle game", 4640 },

    // Development & Emulation
    { "APPLE2E", "Dev", "Apple //e 65C02 128K microcomputer emulator", 23360 },
    { "BASIC", "Dev", "Retro BASIC interpreter with graphics & math", 21968 },
    { "C128", "Dev", "Commodore 128 / 64 dual-mode microcomputer", 15008 },
    { "C64", "Dev", "Commodore 64 MOS 6510 & SID microcomputer emulator", 15320 },
    { "CHIP8", "Dev", "CHIP-8 virtual machine & retro arcade emulator", 14488 },
    { "CUBE3D", "Dev", "Real-time 3D wireframe & solid spinning cube", 44240 },
    { "EMU6502", "Dev", "MOS 6502 CPU and Apple I Woz Monitor", 13748 },
    { "EMU8080", "Dev", "Intel 8080 CPU and CP/M 2.2 microcomputer emulator", 15704 },
    { "HELLO", "Dev", "Minimal SDK hello world demo", 6652 },
    { "MACRUN", "Dev", "Classic Mac OS APPL executable loader", 10380 },
    { "QDEMO", "Dev", "QuickDraw shapes and pattern showcase", 7468 },
    { "SOLARSYS", "Dev", "Solar system planetary orbit simulation", 9992 },
    { "SPIN", "Dev", "Real-time 3D solid mesh rotating model", 6444 },

    // Network
    { "IRC", "Network", "Internet Relay Chat client over BlueSCSI", 22788 },
    { "NET", "Network", "TCP/IP configuration and DaynaPORT stats", 18316 },
    { "PING", "Network", "ICMP network ping utility", 18300 },
    { "PKG", "Network", "APT/YUM style package manager CLI", 25020 },
    { "TELNET", "Network", "Telnet network terminal client", 19680 }
};
static const int NUM_BUILTIN_APPS = sizeof(ALL_BUILTIN_APPS) / sizeof(ALL_BUILTIN_APPS[0]);

static uint64_t get_file_size_on_disk(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) {
        return ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
    }
    return 0;
}

static std::wstring format_size_human(uint64_t bytes) {
    if (bytes == 0) return L"0 B";
    if (bytes < 1024) {
        WCHAR b[32]; wsprintfW(b, L"%u B", (unsigned int)bytes);
        return b;
    }
    double kb = (double)bytes / 1024.0;
    WCHAR b[32];
    if (kb < 100.0) {
        swprintf_s(b, L"%.1f KB", kb);
    } else if (kb < 1024.0) {
        swprintf_s(b, L"%.0f KB", kb);
    } else {
        swprintf_s(b, L"%.2f MB", kb / 1024.0);
    }
    return b;
}

// Probe for the native P68 toolchain (toolchain\bin\*.exe). Populates m_backend, m_p68Dir.
// PlusOS builds only with its own compilers — there is no WSL, MSYS2 or Retro68 path any more.
void BuilderWindow::detect_build_backend() {
    m_backend = BB_NONE;
    m_p68Dir  = L"";

    // The P68 tools live in <repo>\toolchain\bin\. We need at least the compiler and make.
    std::wstring binDir = m_repoRoot + L"\\toolchain\\bin";
    std::wstring cc     = binDir + L"\\p68-cc.exe";
    std::wstring mk     = binDir + L"\\p68-make.exe";
    if (GetFileAttributesW(cc.c_str()) != INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(mk.c_str()) != INVALID_FILE_ATTRIBUTES) {
        m_p68Dir  = binDir;
        m_backend = BB_P68_NATIVE;
    }
}

// Build the command that runs a native build target through make.cmd (the one-click front
// end that drives p68-make + Makefile.p68). All compilation uses the P68 C++ compilers and
// the hard-drive image is produced by tools\hdbuild (the BlueSCSI image creator).
std::wstring BuilderWindow::build_make_command(const std::wstring& target,
                                               const std::wstring& extraFlags) const {
    if (m_backend != BB_P68_NATIVE) return L"";
    // make.cmd [machine keyword] <target> [VAR=value...]   — run inside the repo root.
    return L"cmd.exe /c \"cd /d \"" + m_repoRoot + L"\" && call make.cmd " + target + extraFlags + L"\"";
}

static std::wstring find_bluescsi_drive() {
    for (WCHAR letter = L'D'; letter <= L'Z'; letter++) {
        std::wstring root = std::wstring(1, letter) + L":\\";
        UINT dt = GetDriveTypeW(root.c_str());
        if (dt == DRIVE_REMOVABLE || dt == DRIVE_FIXED) {
            if (GetFileAttributesW((root + L"BOOTTEST.hda").c_str()) != INVALID_FILE_ATTRIBUTES ||
                GetFileAttributesW((root + L"HD60_512 PlusOS.hda").c_str()) != INVALID_FILE_ATTRIBUTES ||
                GetFileAttributesW((root + L"bluescsi.ini").c_str()) != INVALID_FILE_ATTRIBUTES ||
                GetFileAttributesW((root + L"NE4.hda").c_str()) != INVALID_FILE_ATTRIBUTES) {
                return root;
            }
        }
    }
    return L"";
}

static bool dir_has_repo(const std::wstring& path) {
    if (path.empty()) return false;
    std::wstring mk = path + L"\\Makefile";
    std::wstring kr = path + L"\\kernel";
    return (GetFileAttributesW(mk.c_str()) != INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesW(kr.c_str()) != INVALID_FILE_ATTRIBUTES);
}

static std::wstring detect_repository_root() {
    WCHAR szExe[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, szExe, MAX_PATH);
    std::wstring exeDir = szExe;
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) exeDir = exeDir.substr(0, lastSlash);

    // 1. Directory where exe is located
    if (dir_has_repo(exeDir)) return exeDir;

    // 2. Parent directory (e.g. tools/disk_studio/ -> repo root)
    size_t pSlash = exeDir.find_last_of(L"\\/");
    if (pSlash != std::wstring::npos) {
        std::wstring parent = exeDir.substr(0, pSlash);
        if (dir_has_repo(parent)) return parent;

        // 3. Grandparent
        size_t gpSlash = parent.find_last_of(L"\\/");
        if (gpSlash != std::wstring::npos) {
            std::wstring gp = parent.substr(0, gpSlash);
            if (dir_has_repo(gp)) return gp;
        }
    }

    // 4. Current working directory
    WCHAR szCwd[MAX_PATH] = { 0 };
    GetCurrentDirectoryW(MAX_PATH, szCwd);
    if (dir_has_repo(szCwd)) return szCwd;

    // 5. Check subfolder 'PlusOS'
    if (dir_has_repo(exeDir + L"\\PlusOS")) return exeDir + L"\\PlusOS";
    if (dir_has_repo(std::wstring(szCwd) + L"\\PlusOS")) return std::wstring(szCwd) + L"\\PlusOS";

    // 6. Fallback
    if (dir_has_repo(L"C:\\Users\\Evil\\PlusOS")) return L"C:\\Users\\Evil\\PlusOS";

    return L"";
}

void BuilderWindow::detect_and_init_environment()
{
    m_repoRoot = detect_repository_root();
    if (m_repoRoot.empty()) {
        WCHAR szExe[MAX_PATH] = { 0 };
        GetModuleFileNameW(nullptr, szExe, MAX_PATH);
        std::wstring targetDir = szExe;
        size_t lastSlash = targetDir.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) targetDir = targetDir.substr(0, lastSlash);

        std::wstring msg = L"PlusOS source repository was not found in this folder:\r\n" + targetDir +
            L"\r\n\r\nWould you like Disk Studio to automatically download and configure the PlusOS repository here?\r\n\r\n"
            L"Source: https://github.com/g91/PlusOS.git";
        int choice = MessageBoxW(nullptr, msg.c_str(), L"Download PlusOS Repository", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
        if (choice == IDYES) {
            std::wstring psCmd = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \""
                L"$ProgressPreference = 'SilentlyContinue'; "
                L"Set-Location -LiteralPath '" + targetDir + L"'; "
                L"if (Get-Command git -ErrorAction SilentlyContinue) { "
                L"    git clone https://github.com/g91/PlusOS.git . "
                L"} else { "
                L"    Invoke-WebRequest -Uri 'https://github.com/g91/PlusOS/archive/refs/heads/main.zip' -OutFile 'plusos.zip'; "
                L"    Expand-Archive -Path 'plusos.zip' -DestinationPath 'temp_plusos' -Force; "
                L"    Copy-Item -Path 'temp_plusos\\PlusOS-main\\*' -Destination '.' -Recurse -Force; "
                L"    Remove-Item -Path 'temp_plusos', 'plusos.zip' -Recurse -Force; "
                L"}\"";
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi = {};
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            std::vector<WCHAR> cmdVec(psCmd.begin(), psCmd.end());
            cmdVec.push_back(0);
            if (CreateProcessW(nullptr, cmdVec.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, targetDir.c_str(), &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 90000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            if (dir_has_repo(targetDir)) {
                m_repoRoot = targetDir;
            } else {
                m_repoRoot = targetDir;
            }
        } else {
            m_repoRoot = targetDir;
        }
    }

    m_blueScsiDrive = find_bluescsi_drive();
    m_cfg.image_path = m_repoRoot + L"\\images\\PlusOS.hda";
    detect_build_backend();
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
      m_hListViewApps(nullptr), m_hBtnAppsSelectAll(nullptr), m_hBtnAppsSelectNone(nullptr),
      m_hBtnAppsSysOnly(nullptr), m_hBtnAppsGamesOnly(nullptr), m_hBtnAppsAdd(nullptr),
      m_hBtnAppsRemove(nullptr), m_hStaticBaseSystem(nullptr), m_hStaticAppCount(nullptr),
      m_hStaticTotalDisk(nullptr), m_hStaticFloppyStatus(nullptr), m_hStaticFloppyTip(nullptr),
      m_hBtnBuildImage(nullptr), m_hBtnPushUpdate(nullptr), m_hBtnBuildFloppy(nullptr), m_hBtnOpenExplorer(nullptr),
      m_hBtnSaveConfig(nullptr), m_hBtnInjectApp(nullptr), m_hBtnSetupEnv(nullptr),
      m_hBtnBootVM(nullptr),
      m_hGroupLog(nullptr), m_hEditBuildLog(nullptr),
      m_hConsoleFont(nullptr), m_hBoldFont(nullptr), m_hHeaderFont(nullptr)
{
    detect_and_init_environment();
    m_cfg.greeting = L"PlusOS 2.0 (Dual-Boot Mac Edition)";

    // Default boot entries
    MultiBootEntry e1; e1.name = "Mac OS System 7.5.5"; e1.scsi_id = 3; m_cfg.boot_entries.push_back(e1);
    MultiBootEntry e2; e2.name = "Mac OS System 7.1"; e2.scsi_id = 2; m_cfg.boot_entries.push_back(e2);
    MultiBootEntry e3; e3.name = "Mac OS System 6.0.8"; e3.scsi_id = 1; m_cfg.boot_entries.push_back(e3);
    MultiBootEntry e4; e4.name = "PlusOS Desktop"; e4.scsi_id = -2; m_cfg.boot_entries.push_back(e4);
    MultiBootEntry e5; e5.name = "PlusOS Shell"; e5.scsi_id = -1; m_cfg.boot_entries.push_back(e5);

    // Default included applications: every app folder under apps/<cat>/.
    // scan_source_directory needs m_repoRoot, which detect_and_init_environment
    // has already set above.
    scan_source_directory();
    for (const auto& sa : m_sourceApps) {
        m_cfg.included_apps.insert(sa.name);
    }
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
        CW_USEDEFAULT, CW_USEDEFAULT, 1320, 780,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hWnd) return false;

    create_controls();
    load_config_from_file();
    update_controls_from_cfg();

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    append_log(L"=== PlusOS OS Studio & Builder Ready ===");
    append_log(L"Repository Root: " + m_repoRoot);
    if (!m_blueScsiDrive.empty()) {
        append_log(L"BlueSCSI Storage: " + m_blueScsiDrive + L" (Removable drive detected)");
    } else {
        append_log(L"BlueSCSI Storage: Auto-scan active (Insert SD card anytime)");
    }
    // Show the native P68 toolchain status (no WSL / MSYS2 / Linux)
    switch (m_backend) {
    case BB_P68_NATIVE:
        append_log(L"Compiler: P68 native toolchain (p68-cc / p68-as / p68-ld / p68-ar / p68-make)");
        append_log(L"Tools: " + m_p68Dir);
        append_log(L"Image creation: tools\\hdbuild (from-scratch BlueSCSI .hda / HFS builder)");
        break;
    default:
        append_log(L"Compiler: P68 toolchain NOT BUILT — click 'Setup Build Environment' to build it (MSVC).");
        break;
    }
    {
        WCHAR line[96];
        swprintf_s(line, L"Source scan: %u app folder%s found under apps/.",
                   (unsigned)m_sourceApps.size(), m_sourceApps.size() == 1 ? L"" : L"s");
        append_log(line);
    }
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

// ---- Kernel build settings -------------------------------------------------
// Optional kernel modules, in the order of KMODS_ALL in the Makefile.
static const char* const kKmodName[KMOD_COUNT] = {
    "sound", "profiler", "entropy", "power", "pram", "shm",
    "ramdisk", "scrap", "macloader", "stackguard", "watchdog"
};
static const wchar_t* const kKmodLabel[KMOD_COUNT] = {
    L"Sound", L"Profiler", L"Random (entropy)", L"Screen saver", L"PRAM / clock", L"Shared memory",
    L"RAM disk", L"Clipboard", L"Mac app loader", L"Stack guard", L"Watchdog"
};
// Modules a preset leaves OUT (bit i = module i). Must match KMODS_OFF_* in the Makefile.
static const unsigned kProfileOff[4] = {
    0,                                                                      // full
    (1u << 1) | (1u << 2) | (1u << 3),                                      // lean: profiler entropy power
    (1u << 1) | (1u << 2) | (1u << 3) | (1u << 10),                         // fast: lean + watchdog
    0x7FFu & ~(1u << 9),                                                    // tiny: all but stackguard
};
static const char* const kProfileName[4] = { "full", "lean", "fast", "tiny" };

void BuilderWindow::create_controls()
{
    HFONT hNormalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // Group 1: Hardware & Image Target
    HWND grpHardware = CreateWindowExW(0, L"BUTTON", L" 1. Hardware Architecture & Hard Drive Image ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 10, 450, 115, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpHardware, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblHw = CreateWindowExW(0, L"STATIC", L"Target Machine:", WS_CHILD | WS_VISIBLE, 25, 33, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblHw, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboHardware = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 125, 30, 325, 150, m_hWnd, (HMENU)IDB_HARDWARE_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboHardware, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh Plus (8 MHz 68000, 1MB-4MB RAM, SCSI)");
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh SE (8 MHz 68000, ADB, XPRAM Dual-Boot)");
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh Classic (8 MHz 68000, ADB, 512x342)");
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Macintosh LC III / Performa 450 (25 MHz 68030, 32-bit Bus, 640x480 RGB)");
    SendMessageW(m_hComboHardware, CB_ADDSTRING, 0, (LPARAM)L"Universal - ONE image, boots any Mac (auto-detects 68000 / 68030 at startup)");
    SendMessageW(m_hComboHardware, CB_SETCURSEL, 1, 0);

    HWND lblImg = CreateWindowExW(0, L"STATIC", L"Target Image:", WS_CHILD | WS_VISIBLE, 25, 60, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblImg, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hEditImagePath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_cfg.image_path.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 125, 58, 245, 22, m_hWnd, (HMENU)IDB_IMAGE_PATH_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditImagePath, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnBrowseImage = CreateWindowExW(0, L"BUTTON", L"Browse...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 375, 57, 75, 24, m_hWnd, (HMENU)IDB_IMAGE_BROWSE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBrowseImage, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    std::wstring bscsi;
    if (!m_blueScsiDrive.empty()) {
        bscsi = L"BlueSCSI (" + m_blueScsiDrive.substr(0, 2) + L"): Detected! (" + m_blueScsiDrive + L"BOOTTEST.hda updated on install)";
    } else {
        bscsi = L"BlueSCSI: Auto-scan (No BlueSCSI SD card detected; using local PlusOS.hda)";
    }
    m_hStaticBlueSCSI = CreateWindowExW(0, L"STATIC", bscsi.c_str(), WS_CHILD | WS_VISIBLE, 25, 90, 425, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(m_hStaticBlueSCSI, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 2: Boot & Dual-Boot System Configuration
    HWND grpBoot = CreateWindowExW(0, L"BUTTON", L" 2. Dual-Boot System & Boot Preferences ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 130, 450, 245, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpBoot, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblMode = CreateWindowExW(0, L"STATIC", L"Boot Mode:", WS_CHILD | WS_VISIBLE, 25, 155, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblMode, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboBootMode = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 125, 152, 325, 150, m_hWnd, (HMENU)IDB_BOOT_MODE_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboBootMode, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Direct Boot: PlusOS Command Shell");
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Direct Boot: PlusOS Desktop (Finder UI)");
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Interactive Multi-Boot Menu (with Countdown)");
    SendMessageW(m_hComboBootMode, CB_ADDSTRING, 0, (LPARAM)L"Direct Boot: Mac OS (via SCSI Target)");
    SendMessageW(m_hComboBootMode, CB_SETCURSEL, 2, 0);

    HWND lblMacID = CreateWindowExW(0, L"STATIC", L"Mac Target:", WS_CHILD | WS_VISIBLE, 25, 185, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblMacID, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboDirectMacID = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 125, 182, 175, 180, m_hWnd, (HMENU)IDB_DIRECT_MAC_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboDirectMacID, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    for (int i = 0; i < 8; i++) {
        WCHAR buf[64];
        wsprintfW(buf, L"SCSI ID %d%s", i, (i == 1 ? L" (System 6.0.8)" : (i == 2 ? L" (System 7.1)" : (i == 3 ? L" (System 7.5.5)" : L""))));
        SendMessageW(m_hComboDirectMacID, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessageW(m_hComboDirectMacID, CB_SETCURSEL, 3, 0);

    HWND lblTime = CreateWindowExW(0, L"STATIC", L"Timeout:", WS_CHILD | WS_VISIBLE, 310, 185, 60, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblTime, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hEditTimeout = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"5",
        WS_CHILD | WS_VISIBLE | ES_NUMBER, 375, 182, 75, 22, m_hWnd, (HMENU)IDB_TIMEOUT_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditTimeout, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Multi-Boot Entries ListView
    m_hListViewBoot = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        25, 212, 425, 120, m_hWnd, (HMENU)IDB_BOOT_LISTVIEW, m_hInstance, nullptr);
    SendMessageW(m_hListViewBoot, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SendMessageW(m_hListViewBoot, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 35; col.pszText = (LPWSTR)L"#"; ListView_InsertColumn(m_hListViewBoot, 0, &col);
    col.cx = 220; col.pszText = (LPWSTR)L"Operating System Title"; ListView_InsertColumn(m_hListViewBoot, 1, &col);
    col.cx = 150; col.pszText = (LPWSTR)L"Boot Target"; ListView_InsertColumn(m_hListViewBoot, 2, &col);

    // Boot entry buttons
    m_hBtnAddOS = CreateWindowExW(0, L"BUTTON", L"Add OS", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        25, 340, 80, 26, m_hWnd, (HMENU)IDB_BOOT_ADD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAddOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnEditOS = CreateWindowExW(0, L"BUTTON", L"Cycle Target", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        110, 340, 85, 26, m_hWnd, (HMENU)IDB_BOOT_EDIT_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnEditOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnRemoveOS = CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        200, 340, 80, 26, m_hWnd, (HMENU)IDB_BOOT_REMOVE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnRemoveOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnUpOS = CreateWindowExW(0, L"BUTTON", L"Move Up", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        285, 340, 75, 26, m_hWnd, (HMENU)IDB_BOOT_UP_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnUpOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnDownOS = CreateWindowExW(0, L"BUTTON", L"Move Down", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        365, 340, 85, 26, m_hWnd, (HMENU)IDB_BOOT_DOWN_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnDownOS, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 3: Pre-Build OS Customizations & Theming
    HWND grpTheme = CreateWindowExW(0, L"BUTTON", L" 3. Pre-Build Theming & Wallpaper ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 380, 450, 135, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpTheme, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblTheme = CreateWindowExW(0, L"STATIC", L"Theme:", WS_CHILD | WS_VISIBLE, 25, 405, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblTheme, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboTheme = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 125, 402, 325, 180, m_hWnd, (HMENU)IDB_THEME_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboTheme, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Mac OS 7.1 (Classic Platinum Pinstripe)");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Commodore Amiga Workbench 1.3/2.0");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Windows 95 (3D Bevels & Start Bar)");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Windows 98 (Gradient Dither Titlebars)");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Windows Vista/7 Aero Glass");
    SendMessageW(m_hComboTheme, CB_ADDSTRING, 0, (LPARAM)L"Hacker Mode (CRT Phosphor & Matrix Digital Rain)");
    SendMessageW(m_hComboTheme, CB_SETCURSEL, 2, 0);

    HWND lblWall = CreateWindowExW(0, L"STATIC", L"Wallpaper:", WS_CHILD | WS_VISIBLE, 25, 435, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblWall, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboWallpaper = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 125, 432, 160, 120, m_hWnd, (HMENU)IDB_WALLPAPER_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboWallpaper, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboWallpaper, CB_ADDSTRING, 0, (LPARAM)L"Classic Mac 50% Gray");
    SendMessageW(m_hComboWallpaper, CB_ADDSTRING, 0, (LPARAM)L"Matrix Digital Rain");
    SendMessageW(m_hComboWallpaper, CB_SETCURSEL, 0, 0);

    HWND lblRet = CreateWindowExW(0, L"STATIC", L"Return SCSI:", WS_CHILD | WS_VISIBLE, 295, 435, 80, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblRet, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboReturnSCSI = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 375, 432, 75, 150, m_hWnd, (HMENU)IDB_RETURN_SCSI_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboReturnSCSI, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    for (int i = 0; i < 8; i++) {
        WCHAR b[16]; wsprintfW(b, L"SCSI %d", i);
        SendMessageW(m_hComboReturnSCSI, CB_ADDSTRING, 0, (LPARAM)b);
    }
    SendMessageW(m_hComboReturnSCSI, CB_SETCURSEL, 6, 0);

    HWND lblGreet = CreateWindowExW(0, L"STATIC", L"Greeting / Title:", WS_CHILD | WS_VISIBLE, 25, 465, 95, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblGreet, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hEditGreeting = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_cfg.greeting.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 125, 462, 325, 22, m_hWnd, (HMENU)IDB_GREETING_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditGreeting, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 4: Build Execution & Disk Manager
    HWND grpActions = CreateWindowExW(0, L"BUTTON", L" 4. Build Execution & Disk Manager ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 510, 450, 228, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpActions, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnBuildImage = CreateWindowExW(0, L"BUTTON", L"Compile && Build Hard Drive Image (fresh)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON, 25, 532, 430, 32, m_hWnd, (HMENU)IDB_BUILD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBuildImage, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnUpdate = CreateWindowExW(0, L"BUTTON", L"Update (recompile && refresh, keep my files)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 568, 430, 32, m_hWnd, (HMENU)IDB_UPDATE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnUpdate, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    // Admin: compile + publish an OTA release to the update server. Gated by the
    // server's admin password (entered once per session) and, server-side, by the
    // signing key, so a stranger running Disk Studio still cannot publish.
    m_hBtnPushUpdate = CreateWindowExW(0, L"BUTTON", L"Push Update (admin)...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 604, 135, 30, m_hWnd, (HMENU)IDB_PUSH_UPDATE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnPushUpdate, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnBuildFloppy = CreateWindowExW(0, L"BUTTON", L"Floppy (.dsk)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 168, 604, 135, 30, m_hWnd, (HMENU)IDB_BUILD_FLOPPY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBuildFloppy, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnOpenExplorer = CreateWindowExW(0, L"BUTTON", L"Disk Explorer",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 311, 604, 144, 30, m_hWnd, (HMENU)IDB_OPEN_EXPLORER_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnOpenExplorer, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnSaveConfig = CreateWindowExW(0, L"BUTTON", L"Save Configuration",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 638, 210, 28, m_hWnd, (HMENU)IDB_SAVE_CFG_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnSaveConfig, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnInjectApp = CreateWindowExW(0, L"BUTTON", L"Inject 'Return to PlusOS'...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 245, 638, 210, 28, m_hWnd, (HMENU)IDB_INJECT_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnInjectApp, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnSetupEnv = CreateWindowExW(0, L"BUTTON", L"Setup Build Environment (build P68 compilers with MSVC)...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 670, 430, 28, m_hWnd, (HMENU)IDB_SETUP_ENV_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnSetupEnv, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnBootVM = CreateWindowExW(0, L"BUTTON", L"Boot Hard Drive Image in Mini vMac (BlueSCSI)...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 700, 430, 28, m_hWnd, (HMENU)IDB_BOOT_VM_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBootVM, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 5: Software Applications & Floppy Capacity (Column 2)
    HWND grpApps = CreateWindowExW(0, L"BUTTON", L" 5. Software Packages & Floppy Capacity ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 470, 10, 395, 525, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpApps, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hListViewApps = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        480, 33, 375, 300, m_hWnd, (HMENU)IDB_APPS_LISTVIEW, m_hInstance, nullptr);
    SendMessageW(m_hListViewApps, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SendMessageW(m_hListViewApps, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    LVCOLUMNW appCol = {};
    appCol.mask = LVCF_TEXT | LVCF_WIDTH;
    appCol.cx = 145; appCol.pszText = (LPWSTR)L"Application"; ListView_InsertColumn(m_hListViewApps, 0, &appCol);
    appCol.cx = 75;  appCol.pszText = (LPWSTR)L"Category";    ListView_InsertColumn(m_hListViewApps, 1, &appCol);
    appCol.cx = 80;  appCol.pszText = (LPWSTR)L"Size";        ListView_InsertColumn(m_hListViewApps, 2, &appCol);
    appCol.cx = 65;  appCol.pszText = (LPWSTR)L"Origin";      ListView_InsertColumn(m_hListViewApps, 3, &appCol);

    m_hBtnAppsSelectAll = CreateWindowExW(0, L"BUTTON", L"All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        480, 340, 88, 26, m_hWnd, (HMENU)IDB_APPS_SELECT_ALL_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsSelectAll, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsSelectNone = CreateWindowExW(0, L"BUTTON", L"None", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        575, 340, 88, 26, m_hWnd, (HMENU)IDB_APPS_SELECT_NONE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsSelectNone, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsSysOnly = CreateWindowExW(0, L"BUTTON", L"System", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        670, 340, 90, 26, m_hWnd, (HMENU)IDB_APPS_SYS_ONLY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsSysOnly, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsGamesOnly = CreateWindowExW(0, L"BUTTON", L"Games", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        765, 340, 90, 26, m_hWnd, (HMENU)IDB_APPS_GAMES_ONLY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsGamesOnly, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsAdd = CreateWindowExW(0, L"BUTTON", L"Add Custom App...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        480, 372, 183, 28, m_hWnd, (HMENU)IDB_APPS_ADD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsAdd, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsRemove = CreateWindowExW(0, L"BUTTON", L"Remove Selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        672, 372, 183, 28, m_hWnd, (HMENU)IDB_APPS_REMOVE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsRemove, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hStaticBaseSystem = CreateWindowExW(0, L"STATIC", L"Base OS Core: Calculating size...",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 404, 375, 18, m_hWnd, (HMENU)IDB_BASE_SYSTEM_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticBaseSystem, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hStaticAppCount = CreateWindowExW(0, L"STATIC", L"Selected Software: 66 of 66 packages (0 KB)",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 423, 375, 18, m_hWnd, (HMENU)IDB_APPS_COUNT_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticAppCount, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hStaticTotalDisk = CreateWindowExW(0, L"STATIC", L"Total Bootable Footprint: 0 KB (Base OS + Software)",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 442, 375, 19, m_hWnd, (HMENU)IDB_TOTAL_DISK_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticTotalDisk, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hStaticFloppyStatus = CreateWindowExW(0, L"STATIC", L"Floppy: Calculating size...",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 463, 375, 20, m_hWnd, (HMENU)IDB_APPS_FLOPPY_STATUS_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticFloppyStatus, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hStaticFloppyTip = CreateWindowExW(0, L"STATIC",
        L"Floppy Guide: Base OS (~191 KB) is essential and included on every disk. On 800K floppies, software must stay under ~609 KB (Total < 800 KB). 1.44M HD disks support up to 1.44 MB.",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 480, 485, 375, 46, m_hWnd, (HMENU)IDB_APPS_FLOPPY_TIP_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticFloppyTip, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 5b: Kernel build settings (column 2, below the software list)
    HWND grpKernel = CreateWindowExW(0, L"BUTTON", L" Kernel Build Settings (modules) ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 470, 540, 395, 175, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpKernel, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    HWND lblKProf = CreateWindowExW(0, L"STATIC", L"Profile:", WS_CHILD | WS_VISIBLE, 480, 565, 55, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblKProf, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hComboKProfile = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP, 540, 562, 315, 150, m_hWnd, (HMENU)IDB_KPROFILE_COMBO, m_hInstance, nullptr);
    SendMessageW(m_hComboKProfile, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
    SendMessageW(m_hComboKProfile, CB_ADDSTRING, 0, (LPARAM)L"Full - every module (default)");
    SendMessageW(m_hComboKProfile, CB_ADDSTRING, 0, (LPARAM)L"Lean - no profiler, entropy, screen saver");
    SendMessageW(m_hComboKProfile, CB_ADDSTRING, 0, (LPARAM)L"Fast - lean + no watchdog (least overhead)");
    SendMessageW(m_hComboKProfile, CB_ADDSTRING, 0, (LPARAM)L"Tiny - smallest kernel, quickest boot");
    SendMessageW(m_hComboKProfile, CB_ADDSTRING, 0, (LPARAM)L"Custom - pick modules below");
    SendMessageW(m_hComboKProfile, CB_SETCURSEL, 0, 0);

    for (int i = 0; i < KMOD_COUNT; i++) {
        int col = i / 4, row = i % 4;
        m_hCheckKMod[i] = CreateWindowExW(0, L"BUTTON", kKmodLabel[i],
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            480 + col * 127, 594 + row * 22, 125, 20, m_hWnd, (HMENU)(INT_PTR)(IDB_KMOD_FIRST + i), m_hInstance, nullptr);
        SendMessageW(m_hCheckKMod[i], WM_SETFONT, (WPARAM)hNormalFont, TRUE);
        SendMessageW(m_hCheckKMod[i], BM_SETCHECK, BST_CHECKED, 0);
    }

    HWND lblKNote = CreateWindowExW(0, L"STATIC",
        L"Checked = built in. Unchecked modules are left out of the kernel (smaller image, less work per tick/syscall; their syscalls return -1). Saved to kernel.cfg.",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 480, 684, 375, 30, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(lblKNote, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 6: Live Build Log & System Diagnostics (Column 3)
    m_hGroupLog = CreateWindowExW(0, L"BUTTON", L" Build Output & Compilation Log ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 875, 10, 430, 705, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(m_hGroupLog, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hEditBuildLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        885, 35, 410, 665, m_hWnd, (HMENU)IDB_BUILD_LOG_EDIT, m_hInstance, nullptr);
    SendMessageW(m_hEditBuildLog, WM_SETFONT, (WPARAM)m_hConsoleFont, TRUE);
}

void BuilderWindow::layout_controls(int width, int height)
{
    if (width < 900) width = 900;
    int logLeft = 875;
    int logWidth = width - logLeft - 15;
    if (logWidth < 200) logWidth = 200;
    int logHeight = height - 40;
    if (logHeight < 300) logHeight = 705;

    if (m_hGroupLog) SetWindowPos(m_hGroupLog, nullptr, logLeft, 10, logWidth, logHeight, SWP_NOZORDER);
    if (m_hEditBuildLog) SetWindowPos(m_hEditBuildLog, nullptr, logLeft + 10, 35, logWidth - 20, logHeight - 35, SWP_NOZORDER);
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

    // Kernel build settings
    int prof = m_cfg.kernel_profile;
    if (prof < 0 || prof > 4) prof = 0;
    SendMessageW(m_hComboKProfile, CB_SETCURSEL, prof, 0);
    for (int i = 0; i < KMOD_COUNT; i++) {
        SendMessageW(m_hCheckKMod[i], BM_SETCHECK,
            m_cfg.kmod_on[i] ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    populate_boot_list();
    populate_apps_list();
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

    sync_included_apps_from_list();
}

void BuilderWindow::load_config_from_file()
{
    std::string cfgPath = to_narrow(m_repoRoot) + "\\multiboot.cfg";
    std::ifstream in(cfgPath);
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
            else if (k == "include_apps") {
                m_cfg.included_apps.clear();
                std::stringstream ss(v);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    while (!item.empty() && (item.front() == ' ' || item.front() == '\t')) item.erase(item.begin());
                    while (!item.empty() && (item.back() == ' ' || item.back() == '\t' || item.back() == '\r')) item.pop_back();
                    if (!item.empty()) {
                        for (auto& ch : item) ch = (char)toupper(ch);
                        m_cfg.included_apps.insert(item);
                    }
                }
                m_cfg.included_apps.insert("SHELL"); // SHELL is always mandatory
            }
            else if (k == "custom_apps") {
                m_cfg.custom_apps.clear();
                std::stringstream ss(v);
                std::string item;
                while (std::getline(ss, item, ';')) {
                    while (!item.empty() && (item.front() == ' ' || item.front() == '\t')) item.erase(item.begin());
                    while (!item.empty() && (item.back() == ' ' || item.back() == '\t' || item.back() == '\r')) item.pop_back();
                    size_t cpos = item.find(':');
                    if (cpos != std::string::npos) {
                        std::string cname = item.substr(0, cpos);
                        std::string cpath = item.substr(cpos + 1);
                        for (auto& ch : cname) ch = (char)toupper(ch);
                        m_cfg.custom_apps[cname] = to_wide(cpath);
                    }
                }
            }
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

    // Kernel settings live in a separate file written in make syntax
    load_kernel_cfg();
}

bool BuilderWindow::save_config_to_file()
{
    update_cfg_from_controls();

    std::string cfgPath = to_narrow(m_repoRoot) + "\\multiboot.cfg";
    std::ofstream out(cfgPath);
    if (!out.is_open()) return false;

    out << "# PlusOS Multi-Boot & System Configuration\r\n";
    out << "boot_pref=" << m_cfg.boot_pref << "\r\n";
    out << "direct_mac=" << m_cfg.direct_mac_id << "\r\n";
    out << "timeout=" << m_cfg.timeout << "\r\n";
    out << "theme=" << m_cfg.theme << "\r\n";
    out << "wallpaper=" << m_cfg.wallpaper << "\r\n";
    out << "return_scsi_id=" << m_cfg.return_scsi_id << "\r\n";
    out << "greeting=" << to_narrow(m_cfg.greeting) << "\r\n";

    if (!m_cfg.included_apps.empty()) {
        out << "include_apps=";
        bool first = true;
        for (const auto& a : m_cfg.included_apps) {
            if (!first) out << ",";
            out << a;
            first = false;
        }
        out << "\r\n";
    }

    if (!m_cfg.custom_apps.empty()) {
        out << "custom_apps=";
        bool first = true;
        for (const auto& kv : m_cfg.custom_apps) {
            if (!first) out << ";";
            out << kv.first << ":" << to_narrow(kv.second);
            first = false;
        }
        out << "\r\n";
    }
    out << "\r\n";

    for (const auto& e : m_cfg.boot_entries) {
        out << "[entry]\r\n";
        out << "name=" << e.name << "\r\n";
        if (e.scsi_id >= 0) out << "id=" << e.scsi_id << "\r\n\r\n";
        else if (e.scsi_id == -1) out << "id=shell\r\n\r\n";
        else if (e.scsi_id == -2) out << "id=desktop\r\n\r\n";
    }

    out.close();
    append_log(L"Configuration saved to multiboot.cfg successfully.");
    save_kernel_cfg();
    return true;
}

// ---- Kernel build settings -------------------------------------------------
// Map a preset index (0..3) onto the m_cfg.kmod_on[] array; index 4 ("Custom")
// leaves the checkboxes alone.
void BuilderWindow::on_kprofile_changed()
{
    int sel = (int)SendMessageW(m_hComboKProfile, CB_GETCURSEL, 0, 0);
    if (sel < 0 || sel > 3) return;                 // "Custom" or nothing selected
    unsigned off = kProfileOff[sel];
    for (int i = 0; i < KMOD_COUNT; i++) {
        bool on = ((off >> i) & 1u) == 0;
        m_cfg.kmod_on[i] = on;
        SendMessageW(m_hCheckKMod[i], BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    m_cfg.kernel_profile = sel;
    append_log(L"[kernel] profile: " + kernel_summary());
}

// A checkbox was clicked. Read every box back, decide whether the result still
// matches one of the four presets; if it does keep its name, otherwise flip to
// "Custom".
void BuilderWindow::on_kmod_toggled()
{
    unsigned mask = 0;
    for (int i = 0; i < KMOD_COUNT; i++) {
        bool on = SendMessageW(m_hCheckKMod[i], BM_GETCHECK, 0, 0) == BST_CHECKED;
        m_cfg.kmod_on[i] = on;
        if (!on) mask |= (1u << i);
    }
    int match = 4;                                   // default: Custom
    for (int p = 0; p < 4; p++) {
        if (kProfileOff[p] == mask) { match = p; break; }
    }
    m_cfg.kernel_profile = match;
    SendMessageW(m_hComboKProfile, CB_SETCURSEL, match, 0);
}

// Writes kernel.cfg in make syntax: a PROFILE= line plus DISABLE= / ENABLE=
// lists that round-trip exactly what the user picked. The Makefile reads this
// via -include kernel.cfg.
bool BuilderWindow::save_kernel_cfg()
{
    for (int i = 0; i < KMOD_COUNT; i++) {
        m_cfg.kmod_on[i] = SendMessageW(m_hCheckKMod[i], BM_GETCHECK, 0, 0) == BST_CHECKED;
    }
    int profile = (int)SendMessageW(m_hComboKProfile, CB_GETCURSEL, 0, 0);
    if (profile < 0) profile = 0;
    m_cfg.kernel_profile = profile;

    unsigned preset_off = (profile >= 0 && profile <= 3) ? kProfileOff[profile] : 0;
    unsigned want_off   = 0;
    for (int i = 0; i < KMOD_COUNT; i++) if (!m_cfg.kmod_on[i]) want_off |= (1u << i);

    // For Custom, express the whole thing as DISABLE= on top of PROFILE=full.
    // For a preset, record which modules diverge from it as DISABLE= / ENABLE=.
    int base_profile = (profile >= 0 && profile <= 3) ? profile : 0;
    unsigned base_off = (profile >= 0 && profile <= 3) ? preset_off : 0u;
    std::string disable_list, enable_list;
    for (int i = 0; i < KMOD_COUNT; i++) {
        bool base_on = ((base_off >> i) & 1u) == 0;
        bool want_on = m_cfg.kmod_on[i];
        if (base_on && !want_on) {
            if (!disable_list.empty()) disable_list += " ";
            disable_list += kKmodName[i];
        } else if (!base_on && want_on) {
            if (!enable_list.empty()) enable_list += " ";
            enable_list += kKmodName[i];
        }
    }

    std::string path = to_narrow(m_repoRoot) + "\\kernel.cfg";
    std::ofstream out(path);
    if (!out.is_open()) {
        append_log(L"[WARN] Could not write kernel.cfg (check permissions).");
        return false;
    }
    out << "# PlusOS kernel build settings (written by Disk Studio).\n";
    out << "# Read by the top-level Makefile; command-line PROFILE=/DISABLE=/ENABLE= override.\n";
    out << "PROFILE := " << kProfileName[base_profile] << "\n";
    if (!disable_list.empty()) out << "DISABLE := " << disable_list << "\n";
    if (!enable_list.empty())  out << "ENABLE  := " << enable_list  << "\n";
    (void)want_off;
    append_log(L"[kernel] kernel.cfg saved: " + kernel_summary());
    return true;
}

// Load kernel.cfg back into m_cfg. Tolerant of either ':=' or '=' and of
// comma / plus-separated module lists.
void BuilderWindow::load_kernel_cfg()
{
    for (bool& b : m_cfg.kmod_on) b = true;
    m_cfg.kernel_profile = 0;

    std::string path = to_narrow(m_repoRoot) + "\\kernel.cfg";
    std::ifstream in(path);
    if (!in.is_open()) return;

    auto split_names = [](std::string v) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : v) {
            if (c == ' ' || c == '\t' || c == ',' || c == '+') {
                if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            } else cur += c;
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    };
    auto find_mod = [](const std::string& name) -> int {
        for (int i = 0; i < KMOD_COUNT; i++) if (name == kKmodName[i]) return i;
        return -1;
    };

    std::string profile = "full";
    std::vector<std::string> disable_list, enable_list;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        line = line.substr(s);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        while (!k.empty() && (k.back() == ' ' || k.back() == ':')) k.pop_back();
        while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(v.begin());
        if (k == "PROFILE") profile = v;
        else if (k == "DISABLE") disable_list = split_names(v);
        else if (k == "ENABLE")  enable_list  = split_names(v);
    }

    int pidx = 0;
    for (int i = 0; i < 4; i++) if (profile == kProfileName[i]) { pidx = i; break; }
    unsigned off = kProfileOff[pidx];
    for (int i = 0; i < KMOD_COUNT; i++) m_cfg.kmod_on[i] = ((off >> i) & 1u) == 0;
    for (const auto& n : disable_list) { int i = find_mod(n); if (i >= 0) m_cfg.kmod_on[i] = false; }
    for (const auto& n : enable_list)  { int i = find_mod(n); if (i >= 0) m_cfg.kmod_on[i] = true;  }

    // Collapse back to a preset if the resulting mask matches one exactly.
    unsigned mask = 0;
    for (int i = 0; i < KMOD_COUNT; i++) if (!m_cfg.kmod_on[i]) mask |= (1u << i);
    int match = 4;
    for (int p = 0; p < 4; p++) if (kProfileOff[p] == mask) { match = p; break; }
    m_cfg.kernel_profile = match;
}

// One line for the build log: "profile (built-in: a b c; left out: x y)".
std::wstring BuilderWindow::kernel_summary() const
{
    int pidx = m_cfg.kernel_profile;
    if (pidx < 0 || pidx > 4) pidx = 0;
    std::wstring on, off;
    for (int i = 0; i < KMOD_COUNT; i++) {
        std::wstring name = to_wide(kKmodName[i]);
        if (m_cfg.kmod_on[i]) { if (!on.empty())  on  += L" "; on  += name; }
        else                  { if (!off.empty()) off += L" "; off += name; }
    }
    std::wstring profName = to_wide(pidx == 4 ? "custom" : kProfileName[pidx]);
    std::wstring s = profName;
    if (off.empty()) s += L" (all modules built in)";
    else             s += L" (left out: " + off + L")";
    return s;
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

uint64_t BuilderWindow::get_app_size_bytes(const std::string& name)
{
    // Check custom apps first
    auto it = m_cfg.custom_apps.find(name);
    if (it != m_cfg.custom_apps.end()) {
        uint64_t sz = get_file_size_on_disk(it->second);
        if (sz > 0) return sz;
    }

    // Check build/apps/<lower>.pex
    std::string lower = name;
    for (auto& c : lower) c = (char)tolower(c);
    std::wstring p = m_repoRoot + L"\\build\\apps\\" + to_wide(lower) + L".pex";
    uint64_t sz = get_file_size_on_disk(p);
    if (sz > 0) return sz;

    // Check discovered compiled applications
    for (const auto& da : m_discoveredApps) {
        if (_stricmp(da.name.c_str(), name.c_str()) == 0) {
            if (da.size > 0) return da.size;
        }
    }

    // Check builtin fallback metadata table
    for (int i = 0; i < NUM_BUILTIN_APPS; i++) {
        if (_stricmp(ALL_BUILTIN_APPS[i].name, name.c_str()) == 0) {
            return ALL_BUILTIN_APPS[i].fallback_size;
        }
    }
    return 10240; // 10 KB default fallback
}

uint64_t BuilderWindow::get_total_selected_size_bytes()
{
    uint64_t total = 0;
    if (!m_hListViewApps) return 0;
    int count = ListView_GetItemCount(m_hListViewApps);
    for (int i = 0; i < count; i++) {
        if (ListView_GetCheckState(m_hListViewApps, i)) {
            WCHAR buf[128] = L"";
            ListView_GetItemText(m_hListViewApps, i, 0, buf, 128);
            total += get_app_size_bytes(to_narrow(buf));
        }
    }
    return total;
}

uint64_t BuilderWindow::get_base_system_bytes() const
{
    // 1. Boot blocks: Sectors 0 & 1 (loader.bin)
    std::wstring bootPath = m_repoRoot + L"\\build\\boot\\loader.bin";
    uint64_t bootSz = get_file_size_on_disk(bootPath);
    if (bootSz == 0) bootSz = 1024;
    else bootSz = (bootSz + 511) / 512 * 512;

    // 2. PlusOS Kernel binary (kernel.pex)
    std::wstring kernPath = m_repoRoot + L"\\build\\kernel\\kernel.pex";
    uint64_t kernSz = get_file_size_on_disk(kernPath);
    if (kernSz == 0) kernSz = 165 * 1024;
    else kernSz = (kernSz + 511) / 512 * 512;

    // 3. Macintosh HFS volume structures & partition headers (MDB, VBM, B-trees, Desktop)
    uint64_t hfsOverhead = 24 * 1024;

    // 4. System startup configuration (multiboot.cfg)
    uint64_t cfgSz = 1024;

    return bootSz + kernSz + hfsOverhead + cfgSz;
}

uint64_t BuilderWindow::get_total_bootable_size_bytes()
{
    return get_base_system_bytes() + get_total_selected_size_bytes();
}

/* Walk the repo's apps/<category>/<name>/ tree and build the canonical list of
 * what COULD ship on an image. This is the one place that decides "what apps
 * exist" - adding a new folder under apps/<cat>/ is all it takes for the app
 * to show up in Disk Studio's software list. The hardcoded ALL_BUILTIN_APPS
 * table is now only a METADATA source (descriptions, categories for apps that
 * want to be grouped differently); it is not a gate. */
void BuilderWindow::scan_source_directory()
{
    m_sourceApps.clear();
    static const struct { const wchar_t *dir; const char *label; } cats[] = {
        { L"system",      "System"  },
        { L"tools",       "Tools"   },
        { L"games",       "Games"   },
        { L"development", "Dev"     },
        { L"network",     "Network" },
    };
    for (size_t c = 0; c < sizeof cats / sizeof cats[0]; c++) {
        std::wstring pattern = m_repoRoot + L"\\apps\\" + cats[c].dir + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (fd.cFileName[0] == L'.') continue;              // skip . and ..
            SourceApp sa;
            std::wstring nameW = fd.cFileName;
            sa.name = to_narrow(nameW);
            for (auto& ch : sa.name) ch = (char)toupper(ch);
            sa.category   = cats[c].label;
            sa.source_dir = m_repoRoot + L"\\apps\\" + cats[c].dir + L"\\" + nameW + L"\\";
            m_sourceApps.push_back(sa);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    /* Metadata overlay: if ALL_BUILTIN_APPS has a richer category for an app
     * (e.g. apps/system/off is a "System" app that we want listed under
     * "System" rather than its source folder), honour the hardcoded value. */
    for (auto& sa : m_sourceApps) {
        for (int i = 0; i < NUM_BUILTIN_APPS; i++) {
            if (_stricmp(ALL_BUILTIN_APPS[i].name, sa.name.c_str()) == 0) {
                sa.category = ALL_BUILTIN_APPS[i].category;
                break;
            }
        }
    }
}

void BuilderWindow::scan_compile_directory()
{
    std::wstring searchPattern = m_repoRoot + L"\\build\\apps\\*.pex";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring fname = fd.cFileName;
            size_t dotPos = fname.find_last_of(L'.');
            if (dotPos != std::wstring::npos) fname = fname.substr(0, dotPos);
            std::string appName = to_narrow(fname);
            for (auto& c : appName) c = (char)toupper(c);

            // Already covered by the source-directory scan? Then it's a
            // first-class app, not a stray "Discovered" extra.
            bool isSource = false;
            for (const auto& sa : m_sourceApps) {
                if (_stricmp(sa.name.c_str(), appName.c_str()) == 0) { isSource = true; break; }
            }
            if (isSource) continue;

            // Check if in custom apps
            if (m_cfg.custom_apps.find(appName) != m_cfg.custom_apps.end()) continue;

            // Check if already in m_discoveredApps
            auto it = std::find_if(m_discoveredApps.begin(), m_discoveredApps.end(),
                [&](const DiscoveredApp& da) { return da.name == appName; });

            std::wstring fullPath = m_repoRoot + L"\\build\\apps\\" + std::wstring(fd.cFileName);
            uint64_t sz = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;

            if (it == m_discoveredApps.end()) {
                DiscoveredApp da;
                da.name = appName;
                da.path = fullPath;
                da.size = sz;
                m_discoveredApps.push_back(da);
                m_cfg.included_apps.insert(appName);
                append_log(L"[SCAN] Discovered new compiled application: " + to_wide(appName) + L" (" + format_size_human(sz) + L")");
            } else {
                it->size = sz;
                it->path = fullPath;
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

void BuilderWindow::populate_apps_list()
{
    if (!m_hListViewApps) return;
    scan_source_directory();                 // authoritative: what apps exist in the repo
    scan_compile_directory();                // overlay: freshly compiled artefacts
    m_populatingApps = true;
    ListView_DeleteAllItems(m_hListViewApps);

    int rowIndex = 0;
    // Every app folder under apps/<category>/. New apps appear here automatically.
    for (const auto& sa : m_sourceApps) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = rowIndex;
        std::wstring nameW = to_wide(sa.name);
        item.pszText = (LPWSTR)nameW.c_str();
        ListView_InsertItem(m_hListViewApps, &item);

        std::wstring catW = to_wide(sa.category);
        ListView_SetItemText(m_hListViewApps, rowIndex, 1, (LPWSTR)catW.c_str());

        uint64_t sz = get_app_size_bytes(sa.name);
        std::wstring szStr = format_size_human(sz);
        ListView_SetItemText(m_hListViewApps, rowIndex, 2, (LPWSTR)szStr.c_str());

        bool isShell = (_stricmp(sa.name.c_str(), "SHELL") == 0);
        ListView_SetItemText(m_hListViewApps, rowIndex, 3,
            (LPWSTR)(isShell ? L"Built-in (Required)" : L"Built-in"));

        bool checked = isShell || (m_cfg.included_apps.find(sa.name) != m_cfg.included_apps.end());
        ListView_SetCheckState(m_hListViewApps, rowIndex, checked ? TRUE : FALSE);
        rowIndex++;
    }

    // Custom external applications
    for (const auto& kv : m_cfg.custom_apps) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = rowIndex;
        std::wstring nameW = to_wide(kv.first);
        item.pszText = (LPWSTR)nameW.c_str();
        ListView_InsertItem(m_hListViewApps, &item);

        ListView_SetItemText(m_hListViewApps, rowIndex, 1, (LPWSTR)L"Custom");

        uint64_t sz = get_app_size_bytes(kv.first);
        std::wstring szStr = format_size_human(sz);
        ListView_SetItemText(m_hListViewApps, rowIndex, 2, (LPWSTR)szStr.c_str());

        ListView_SetItemText(m_hListViewApps, rowIndex, 3, (LPWSTR)L"External");

        bool checked = (m_cfg.included_apps.find(kv.first) != m_cfg.included_apps.end());
        ListView_SetCheckState(m_hListViewApps, rowIndex, checked ? TRUE : FALSE);
        rowIndex++;
    }

    // Discovered compiled applications from build directory
    for (const auto& da : m_discoveredApps) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = rowIndex;
        std::wstring nameW = to_wide(da.name);
        item.pszText = (LPWSTR)nameW.c_str();
        ListView_InsertItem(m_hListViewApps, &item);

        ListView_SetItemText(m_hListViewApps, rowIndex, 1, (LPWSTR)L"Compiled");

        std::wstring szStr = format_size_human(da.size > 0 ? da.size : get_app_size_bytes(da.name));
        ListView_SetItemText(m_hListViewApps, rowIndex, 2, (LPWSTR)szStr.c_str());

        ListView_SetItemText(m_hListViewApps, rowIndex, 3, (LPWSTR)L"Discovered");

        bool checked = (m_cfg.included_apps.find(da.name) != m_cfg.included_apps.end());
        ListView_SetCheckState(m_hListViewApps, rowIndex, checked ? TRUE : FALSE);
        rowIndex++;
    }

    m_populatingApps = false;
    update_apps_count_label();
}

void BuilderWindow::update_apps_count_label()
{
    if (!m_hListViewApps) return;
    int total = ListView_GetItemCount(m_hListViewApps);
    int selected = 0;
    uint64_t appBytes = 0;
    for (int i = 0; i < total; i++) {
        if (ListView_GetCheckState(m_hListViewApps, i)) {
            selected++;
            WCHAR nameBuf[128] = L"";
            ListView_GetItemText(m_hListViewApps, i, 0, nameBuf, 128);
            appBytes += get_app_size_bytes(to_narrow(nameBuf));
        }
    }

    uint64_t baseBytes = get_base_system_bytes();
    uint64_t totalBootable = baseBytes + appBytes;

    std::wstring appStr = format_size_human(appBytes);
    std::wstring baseStr = format_size_human(baseBytes);
    std::wstring totalStr = format_size_human(totalBootable);

    // 1. Base OS Boot Core (Kernel + Bootloader + Filesystem Overhead)
    if (m_hStaticBaseSystem) {
        WCHAR baseBuf[192];
        wsprintfW(baseBuf, L"Base OS Core: %s (Kernel, Boot Blocks & HFS - Required)", baseStr.c_str());
        SetWindowTextW(m_hStaticBaseSystem, baseBuf);
    }

    // 2. Selected Software Packages
    if (m_hStaticAppCount) {
        WCHAR countBuf[192];
        wsprintfW(countBuf, L"Selected Software: %d of %d packages (%s)", selected, total, appStr.c_str());
        SetWindowTextW(m_hStaticAppCount, countBuf);
    }

    // 3. Total Bootable Footprint
    if (m_hStaticTotalDisk) {
        WCHAR totalBuf[220];
        wsprintfW(totalBuf, L"Total Bootable Footprint: %s (%s Base + %s Apps)", totalStr.c_str(), baseStr.c_str(), appStr.c_str());
        SetWindowTextW(m_hStaticTotalDisk, totalBuf);
    }

    // 4. Floppy Status with live capacity checks
    if (m_hStaticFloppyStatus) {
        WCHAR statusBuf[256];
        if (totalBootable <= 800 * 1024) {
            uint64_t free800 = (800 * 1024) - totalBootable;
            wsprintfW(statusBuf, L"Floppy: Fits 800K & 1.44M (%s / 800 KB | %s Free)", totalStr.c_str(), format_size_human(free800).c_str());
        } else if (totalBootable <= 1440 * 1024) {
            uint64_t free1440 = (1440 * 1024) - totalBootable;
            wsprintfW(statusBuf, L"Floppy: Fits 1.44M HD ONLY (%s / 1.44 MB | %s Free - EXCEEDS 800K!)", totalStr.c_str(), format_size_human(free1440).c_str());
        } else {
            wsprintfW(statusBuf, L"Floppy: EXCEEDS 1.44M! (%s - Hard Drive Only)", totalStr.c_str());
        }
        SetWindowTextW(m_hStaticFloppyStatus, statusBuf);
    }

    // 5. Floppy Guidance Tip
    if (m_hStaticFloppyTip) {
        uint64_t maxSoftware800 = (800 * 1024 > baseBytes) ? ((800 * 1024) - baseBytes) : 0;
        WCHAR tipBuf[320];
        wsprintfW(tipBuf,
            L"Floppy Guide: Base OS (%s) is essential to boot. On 800K floppies, software must stay under %s (Total < 800 KB). 1.44M HD disks support up to 1.44 MB.",
            baseStr.c_str(), format_size_human(maxSoftware800).c_str());
        SetWindowTextW(m_hStaticFloppyTip, tipBuf);
    }
}

void BuilderWindow::sync_included_apps_from_list()
{
    if (m_populatingApps || !m_hListViewApps) return;
    m_cfg.included_apps.clear();
    m_cfg.included_apps.insert("SHELL"); // SHELL is always mandatory

    int total = ListView_GetItemCount(m_hListViewApps);
    for (int i = 0; i < total; i++) {
        WCHAR buf[128] = L"";
        ListView_GetItemText(m_hListViewApps, i, 0, buf, 128);
        std::string name = to_narrow(buf);
        for (auto& ch : name) ch = (char)toupper(ch);

        if (name == "SHELL") {
            if (!ListView_GetCheckState(m_hListViewApps, i)) {
                m_populatingApps = true;
                ListView_SetCheckState(m_hListViewApps, i, TRUE);
                m_populatingApps = false;
            }
        } else if (ListView_GetCheckState(m_hListViewApps, i)) {
            if (!name.empty()) {
                m_cfg.included_apps.insert(name);
            }
        }
    }
    update_apps_count_label();
}

void BuilderWindow::on_apps_select_all()
{
    if (!m_hListViewApps) return;
    m_populatingApps = true;
    int total = ListView_GetItemCount(m_hListViewApps);
    for (int i = 0; i < total; i++) {
        ListView_SetCheckState(m_hListViewApps, i, TRUE);
    }
    m_populatingApps = false;
    sync_included_apps_from_list();
}

void BuilderWindow::on_apps_select_none()
{
    if (!m_hListViewApps) return;
    m_populatingApps = true;
    int total = ListView_GetItemCount(m_hListViewApps);
    for (int i = 0; i < total; i++) {
        WCHAR buf[128] = L"";
        ListView_GetItemText(m_hListViewApps, i, 0, buf, 128);
        if (_wcsicmp(buf, L"SHELL") == 0) {
            ListView_SetCheckState(m_hListViewApps, i, TRUE); // Keep SHELL always selected
        } else {
            ListView_SetCheckState(m_hListViewApps, i, FALSE);
        }
    }
    m_populatingApps = false;
    sync_included_apps_from_list();
}

void BuilderWindow::on_apps_sys_only()
{
    if (!m_hListViewApps) return;
    m_populatingApps = true;
    int total = ListView_GetItemCount(m_hListViewApps);
    for (int i = 0; i < total; i++) {
        WCHAR buf[128] = L"";
        ListView_GetItemText(m_hListViewApps, i, 0, buf, 128);
        WCHAR cat[64] = L"";
        ListView_GetItemText(m_hListViewApps, i, 1, cat, 64);
        bool isSys = (wcscmp(cat, L"System") == 0 || wcscmp(cat, L"Tools") == 0 || _wcsicmp(buf, L"SHELL") == 0);
        ListView_SetCheckState(m_hListViewApps, i, isSys ? TRUE : FALSE);
    }
    m_populatingApps = false;
    sync_included_apps_from_list();
}

void BuilderWindow::on_apps_games_only()
{
    if (!m_hListViewApps) return;
    m_populatingApps = true;
    int total = ListView_GetItemCount(m_hListViewApps);
    for (int i = 0; i < total; i++) {
        WCHAR buf[128] = L"";
        ListView_GetItemText(m_hListViewApps, i, 0, buf, 128);
        WCHAR cat[64] = L"";
        ListView_GetItemText(m_hListViewApps, i, 1, cat, 64);
        bool isGame = (wcscmp(cat, L"Games") == 0);
        bool isShell = (_wcsicmp(buf, L"SHELL") == 0);
        ListView_SetCheckState(m_hListViewApps, i, (isGame || isShell) ? TRUE : FALSE);
    }
    m_populatingApps = false;
    sync_included_apps_from_list();
}

void BuilderWindow::on_apps_add_custom()
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"PlusOS Executables (*.pex;*.bin)\0*.pex;*.bin\0All Files (*.*)\0*.*\0";
    ofn.lpstrTitle = L"Select Custom Application Binary to Include";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring pathStr = szFile;
        size_t lastSlash = pathStr.find_last_of(L"\\/");
        std::wstring fname = (lastSlash != std::wstring::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
        size_t dotPos = fname.find_last_of(L'.');
        if (dotPos != std::wstring::npos) fname = fname.substr(0, dotPos);

        std::string appName = to_narrow(fname);
        for (auto& ch : appName) ch = (char)toupper(ch);

        m_cfg.custom_apps[appName] = szFile;
        m_cfg.included_apps.insert(appName);

        populate_apps_list();
        append_log(L"Added custom application package: " + to_wide(appName) + L" [" + szFile + L"]");
    }
}

void BuilderWindow::on_apps_remove_selected()
{
    int sel = ListView_GetNextItem(m_hListViewApps, -1, LVNI_SELECTED);
    if (sel < 0) {
        MessageBoxW(m_hWnd, L"Please select an application from the list to remove or uncheck.", L"Selection Required", MB_ICONINFORMATION | MB_OK);
        return;
    }

    WCHAR nameBuf[128] = L"";
    ListView_GetItemText(m_hListViewApps, sel, 0, nameBuf, 128);
    std::string appName = to_narrow(nameBuf);
    for (auto& ch : appName) ch = (char)toupper(ch);

    if (appName == "SHELL") {
        MessageBoxW(m_hWnd,
            L"The PlusOS Command Shell ('SHELL') is the core operating system runtime environment.\r\n\r\n"
            L"It is required by the kernel and cannot be removed or unselected.",
            L"Required System Package", MB_ICONINFORMATION | MB_OK);
        return;
    }

    auto it = m_cfg.custom_apps.find(appName);
    if (it != m_cfg.custom_apps.end()) {
        m_cfg.custom_apps.erase(it);
        m_cfg.included_apps.erase(appName);
        populate_apps_list();
        append_log(L"Removed custom application package: " + to_wide(appName));
    } else {
        auto itDisc = std::find_if(m_discoveredApps.begin(), m_discoveredApps.end(),
            [&](const DiscoveredApp& da) { return da.name == appName; });
        if (itDisc != m_discoveredApps.end()) {
            ListView_SetCheckState(m_hListViewApps, sel, FALSE);
            sync_included_apps_from_list();
            append_log(L"Unchecked discovered compiled package: " + to_wide(appName));
        } else {
            ListView_SetCheckState(m_hListViewApps, sel, FALSE);
            sync_included_apps_from_list();
            append_log(L"Unchecked built-in package: " + to_wide(appName));
        }
    }
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

// ----------------------------------------------------------------------------
// Minimal modal text prompt (Win32 has no InputBox). Builds a dialog in memory
// with a label, an edit box (optionally a password box), and OK/Cancel. Returns
// true and fills `out` on OK; false on Cancel/close.
// ----------------------------------------------------------------------------
struct PromptCtx { const wchar_t* label; std::wstring* out; bool password; };

static INT_PTR CALLBACK PromptDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static PromptCtx* ctx = nullptr;
    switch (msg) {
    case WM_INITDIALOG:
        ctx = (PromptCtx*)lParam;
        SetWindowTextW(hDlg, L"PlusOS Disk Studio");
        SetDlgItemTextW(hDlg, 1001, ctx->label);
        if (ctx->password) SendDlgItemMessageW(hDlg, 1002, EM_SETPASSWORDCHAR, (WPARAM)L'*', 0);
        SetDlgItemTextW(hDlg, 1002, ctx->out->c_str());
        SetFocus(GetDlgItem(hDlg, 1002));
        return FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[512] = {};
            GetDlgItemTextW(hDlg, 1002, buf, 511);
            if (ctx && ctx->out) *ctx->out = buf;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

static bool prompt_text(HWND parent, HINSTANCE hInst, const wchar_t* label,
                        std::wstring& out, bool password)
{
    // In-memory dialog template: 220x80 DLU, with a static, an edit and 2 buttons.
    struct { DLGTEMPLATE dt; WORD menu, cls; WCHAR title[1]; } hdr = {};
    // Building a full DLGTEMPLATE by hand is error-prone across items, so use a
    // byte buffer and the documented layout.
    std::vector<BYTE> buf(1024, 0);
    BYTE* p = buf.data();
    auto align4 = [&](BYTE*& q){ while ((((UINT_PTR)q) & 3) != 0) *q++ = 0; };
    DLGTEMPLATE* dlg = (DLGTEMPLATE*)p;
    dlg->style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT;
    dlg->dwExtendedStyle = 0;
    dlg->cdit = 4;
    dlg->x = 0; dlg->y = 0; dlg->cx = 230; dlg->cy = 86;
    p += sizeof(DLGTEMPLATE);
    *(WORD*)p = 0; p += 2;                 // no menu
    *(WORD*)p = 0; p += 2;                 // default class
    *(WORD*)p = 0; p += 2;                 // empty title
    *(WORD*)p = 8; p += 2;                 // font size
    const wchar_t* fnt = L"Segoe UI";
    wmemcpy((wchar_t*)p, fnt, wcslen(fnt) + 1); p += (wcslen(fnt) + 1) * 2;

    auto add_item = [&](DWORD style, short x, short y, short cx, short cy, WORD id,
                        WORD cls, const wchar_t* text) {
        align4(p);
        DLGITEMTEMPLATE* it = (DLGITEMTEMPLATE*)p;
        it->style = style; it->dwExtendedStyle = 0;
        it->x = x; it->y = y; it->cx = cx; it->cy = cy; it->id = id;
        p += sizeof(DLGITEMTEMPLATE);
        *(WORD*)p = 0xFFFF; p += 2; *(WORD*)p = cls; p += 2;   // predefined class
        wmemcpy((wchar_t*)p, text, wcslen(text) + 1); p += (wcslen(text) + 1) * 2;
        *(WORD*)p = 0; p += 2;             // no creation data
    };
    add_item(WS_VISIBLE | WS_CHILD | SS_LEFT, 8, 7, 214, 18, 1001, 0x0082, L"");       // static
    add_item(WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
             8, 28, 214, 14, 1002, 0x0081, L"");                                        // edit
    add_item(WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 118, 60, 50, 16, IDOK, 0x0080, L"OK");
    add_item(WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 172, 60, 50, 16, IDCANCEL, 0x0080, L"Cancel");

    PromptCtx ctx{ label, &out, password };
    INT_PTR r = DialogBoxIndirectParamW(hInst, (LPCDLGTEMPLATEW)buf.data(), parent,
                                        PromptDlgProc, (LPARAM)&ctx);
    return r == IDOK;
}

static void RunProcessAsyncWithLogging(HWND hWnd, UINT completeMsg, const std::wstring& cmd, const std::wstring& fallbackCmd, const std::wstring& rootDir)
{
    std::thread([hWnd, completeMsg, cmd, fallbackCmd, rootDir]() {
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE hReadPipe = nullptr;
        HANDLE hWritePipe = nullptr;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            std::wstring* pErr = new std::wstring(L"[ERROR] Failed to create output pipe.");
            PostMessageW(hWnd, WM_BUILD_LOG_LINE, 0, (LPARAM)pErr);
            PostMessageW(hWnd, completeMsg, 1, 0);
            return;
        }
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        PROCESS_INFORMATION pi = {};
        std::wstring currentCmd = cmd;

        BOOL ok = CreateProcessW(
            nullptr,
            &currentCmd[0],
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            rootDir.c_str(),
            &si,
            &pi
        );

        if (!ok && !fallbackCmd.empty()) {
            std::wstring* pNotice = new std::wstring(L"Notice: Primary command failed to start, trying fallback: " + fallbackCmd);
            PostMessageW(hWnd, WM_BUILD_LOG_LINE, 0, (LPARAM)pNotice);
            currentCmd = fallbackCmd;
            ok = CreateProcessW(
                nullptr,
                &currentCmd[0],
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                rootDir.c_str(),
                &si,
                &pi
            );
        }

        CloseHandle(hWritePipe);

        if (!ok) {
            CloseHandle(hReadPipe);
            std::wstring* pErr = new std::wstring(L"[ERROR] Failed to start build process: " + currentCmd);
            PostMessageW(hWnd, WM_BUILD_LOG_LINE, 0, (LPARAM)pErr);
            PostMessageW(hWnd, completeMsg, 1, 0);
            return;
        }

        char buffer[2048];
        DWORD bytesRead = 0;
        std::string partialLine;

        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            partialLine.append(buffer, bytesRead);

            size_t newlinePos = 0;
            while ((newlinePos = partialLine.find('\n')) != std::string::npos) {
                std::string line = partialLine.substr(0, newlinePos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                partialLine.erase(0, newlinePos + 1);

                std::wstring* pLineW = new std::wstring(to_wide(line));
                PostMessageW(hWnd, WM_BUILD_LOG_LINE, 0, (LPARAM)pLineW);
            }
        }

        if (!partialLine.empty()) {
            if (partialLine.back() == '\r') partialLine.pop_back();
            std::wstring* pLineW = new std::wstring(to_wide(partialLine));
            PostMessageW(hWnd, WM_BUILD_LOG_LINE, 0, (LPARAM)pLineW);
        }

        CloseHandle(hReadPipe);
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        PostMessageW(hWnd, completeMsg, (WPARAM)exitCode, 0);
    }).detach();
}

// Map the Target Machine combo to the make.cmd machine keyword (first argument).
static std::wstring machine_keyword(int target_hardware) {
    switch (target_hardware) {
    case 0: return L"plus";
    case 2: return L"classic";
    case 3: return L"lc3";
    case 4: return L"universal";
    default: return L"se";
    }
}

void BuilderWindow::action_build_image()
{
    if (m_isBuilding) {
        MessageBoxW(m_hWnd, L"A build is already in progress. Please wait for it to complete.", L"Build Running", MB_ICONINFORMATION | MB_OK);
        return;
    }

    if (m_backend != BB_P68_NATIVE) {
        int r = MessageBoxW(m_hWnd,
            L"The PlusOS native compilers (toolchain\\bin\\p68-cc.exe ...) are not built yet.\r\n\r\n"
            L"Click 'Setup Build Environment' to build them once with Visual Studio's C++ tools (MSVC).\r\n\r\n"
            L"Build them now?",
            L"P68 Toolchain Required", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1);
        if (r == IDYES) action_setup_environment();
        return;
    }

    update_cfg_from_controls();
    save_config_to_file();

    m_isBuilding = true;
    EnableWindow(m_hBtnBuildImage, FALSE);
    EnableWindow(m_hBtnBuildFloppy, FALSE);

    uint64_t appBytes = get_total_selected_size_bytes();
    uint64_t baseBytes = get_base_system_bytes();
    uint64_t totalBootable = baseBytes + appBytes;

    // The Macintosh LC III's dedicated boot loader (QuickDraw diagnostic dialog) is unreliable
    // on real hardware, so build PlusOS with the UNIVERSAL target instead: a minimal, ROM-
    // independent boot block that runs on every 680x0 (the kernel detects the 68030 and flushes
    // its caches at run time). The universal 68000 image boots correctly on the LC III.
    std::wstring machine = machine_keyword(m_cfg.target_hardware);
    bool lc3 = (m_cfg.target_hardware == 3);
    if (lc3) machine = L"universal";

    // Build to a LOCAL image with no spaces in the path (so the path survives cmd -> make ->
    // hdbuild cleanly), then, if a BlueSCSI SD card is present, copy it onto the card ourselves
    // with CopyFileW (which handles the card's "HD60_512 PlusOS.hda" name with its space fine).
    bool toCard = !m_blueScsiDrive.empty();
    std::wstring outImg = m_repoRoot + L"\\build_p68\\PlusOS_disk.hda";
    m_cardTarget = toCard ? (m_blueScsiDrive + L"HD60_512 PlusOS.hda") : L"";

    // Source an Apple SCSI driver automatically: scan the SD card for a bootable Mac OS .hda and
    // reuse its driver (hdbuild --driver-from), so the Mac ROM will boot the disk on real hardware.
    std::wstring driverSrc = toCard ? m_blueScsiDrive : L"";

    append_log(L"\r\n==================================================");
    append_log(L"[BUILD] PlusOS Hard Drive Image Build Pipeline (native P68 toolchain)");
    append_log(L"Compiler: p68-cc / p68-as / p68-ld / p68-ar  (no WSL / Linux)");
    append_log(L"Image builder: tools\\hdbuild  (from-scratch APM + HFS + Apple SCSI driver)");
    if (lc3) append_log(L"Target Machine: Macintosh LC III -> building UNIVERSAL image (reliable boot loader)");
    else append_log(L"Target Machine: " + machine);
    if (toCard)
        append_log(L"Output: will be written to the BlueSCSI SD card at " + m_cardTarget);
    else
        append_log(L"Output: " + outImg);
    if (!driverSrc.empty())
        append_log(L"Apple SCSI driver: auto-sourced from a bootable Mac OS disk on " + driverSrc);
    else {
        append_log(L"[WARNING] No BlueSCSI SD card detected - the image will have NO Apple driver and");
        append_log(L"          will NOT boot on real hardware (insert the SD card and rebuild to fix).");
    }
    append_log(L"Base OS Core: " + format_size_human(baseBytes) + L" (Kernel, Boot Blocks & HFS Overhead - Required)");
    append_log(L"Selected Software: " + format_size_human(appBytes));
    append_log(L"Total Bootable Footprint: " + format_size_human(totalBootable));
    append_log(L"Kernel: " + kernel_summary());
    append_log(L"Building...");

    std::wstring rootDir = m_repoRoot;
    // make.cmd <machine> image OUTIMG=<local path> [DRIVER=<card>]
    //   OUTIMG = a local, space-free path (copied to the card afterwards by on_build_finished)
    //   DRIVER = where hdbuild auto-sources the Apple SCSI driver from (the SD card)
    std::wstring extra = L" OUTIMG=" + outImg;           // no spaces -> no quoting needed
    if (!driverSrc.empty()) {
        // pass the card as a bare drive ("F:") - a trailing backslash before a space in cmd
        // would escape badly; hdbuild expands "F:" to the drive root itself.
        std::wstring d = driverSrc;
        while (!d.empty() && (d.back() == L'\\' || d.back() == L'/')) d.pop_back();
        extra += L" DRIVER=" + d;
    }
    m_lastBuiltImage = outImg;
    std::wstring cmd = build_make_command(machine + L" image", extra);

    RunProcessAsyncWithLogging(m_hWnd, WM_BUILD_COMPLETED, cmd, L"", rootDir);
}

void BuilderWindow::on_build_finished(DWORD exitCode)
{
    bool wasUpdate = m_isUpdate;
    m_isBuilding = false;
    m_isUpdate = false;
    EnableWindow(m_hBtnBuildImage, TRUE);
    EnableWindow(m_hBtnUpdate, TRUE);
    EnableWindow(m_hBtnPushUpdate, TRUE);
    EnableWindow(m_hBtnBuildFloppy, TRUE);

    // An in-place UPDATE has its own flow: it already wrote the kernel/software
    // over the existing disk (user files kept, driver reused). No card-copy step.
    if (wasUpdate) {
        if (exitCode == 0) {
            append_log(L"--------------------------------------------------");
            append_log(L"[SUCCESS] PlusOS updated in place. Kernel + software refreshed.");
            append_log(L"User files were preserved and the Apple SCSI driver was reused.");
            if (!m_lastBuiltImage.empty()) append_log(L"Updated disk: " + m_lastBuiltImage);
            populate_apps_list();
            bool onCard = !m_blueScsiDrive.empty() &&
                          m_lastBuiltImage.rfind(m_blueScsiDrive, 0) == 0;
            MessageBoxW(m_hWnd,
                (L"PlusOS was updated on the disk, keeping all your files:\r\n\r\n    "
                 + m_lastBuiltImage + L"\r\n\r\n"
                 L"The kernel and all applications were recompiled and refreshed.\r\n\r\n"
                 + (onCard
                    ? std::wstring(L"Safely EJECT the SD card from Windows, put it back in the BlueSCSI, and restart the Mac.")
                    : std::wstring(L"Write this disk to your BlueSCSI SD card to boot the updated system."))).c_str(),
                L"Update Successful", MB_ICONINFORMATION | MB_OK);
        } else {
            append_log(L"--------------------------------------------------");
            append_log(L"[ERROR] Update finished with exit code: " + std::to_wstring(exitCode));
            MessageBoxW(m_hWnd,
                L"The update failed. Your existing disk was NOT modified (the new image is written to a "
                L"temporary file first and only moved into place on success). See the build log for details.",
                L"Update Failed", MB_ICONWARNING | MB_OK);
        }
        return;
    }

    if (exitCode == 0) {
        append_log(L"--------------------------------------------------");
        append_log(L"[SUCCESS] Hard drive image built with the P68 toolchain.");
        append_log(L"Kernel, boot blocks, raw store, and all applications written.");
        append_log(L"Apple SCSI driver partition embedded (boots on real BlueSCSI hardware).");
        if (!m_lastBuiltImage.empty())
            append_log(L"Image: " + m_lastBuiltImage);
        populate_apps_list();

        // If a BlueSCSI SD card is present, copy the finished image onto it now (CopyFileW
        // handles the card's spaced "HD60_512 PlusOS.hda" name without any shell quoting).
        bool copiedToCard = false;
        if (!m_cardTarget.empty() && !m_lastBuiltImage.empty()) {
            append_log(L"Writing image to the BlueSCSI SD card: " + m_cardTarget + L" ...");
            if (CopyFileW(m_lastBuiltImage.c_str(), m_cardTarget.c_str(), FALSE)) {
                copiedToCard = true;
                append_log(L"[OK] Image written to the SD card.");
            } else {
                DWORD err = GetLastError();
                append_log(L"[ERROR] Could not write to the SD card (Win32 error " + std::to_wstring(err) + L").");
                MessageBoxW(m_hWnd,
                    (L"The image was built but could not be copied to the SD card:\r\n\r\n    "
                     + m_cardTarget + L"\r\n\r\n"
                     L"Make sure the card is not write-protected or in use, then copy it manually:\r\n    "
                     + m_lastBuiltImage).c_str(),
                    L"SD Card Write Failed", MB_ICONWARNING | MB_OK);
            }
        }

        if (copiedToCard) {
            MessageBoxW(m_hWnd,
                (L"PlusOS was built and written to the BlueSCSI SD card:\r\n\r\n    "
                 + m_cardTarget + L"\r\n\r\n"
                 L"It includes an Apple SCSI driver, so the Macintosh ROM will boot it.\r\n\r\n"
                 L"Safely EJECT the SD card from Windows, put it back in the BlueSCSI, and restart the Mac.").c_str(),
                L"Build Successful - Written to SD Card", MB_ICONINFORMATION | MB_OK);
        } else if (m_cardTarget.empty()) {
            int c = MessageBoxW(m_hWnd,
                (L"Hard drive image built successfully:\r\n\r\n    " + m_lastBuiltImage + L"\r\n\r\n"
                 L"No BlueSCSI SD card was detected, so it was written locally. Insert the card and "
                 L"rebuild to write straight to it with a boot driver.\r\n\r\n"
                 L"Open the Disk Explorer to inspect the files now?").c_str(),
                L"Build Successful", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1);
            if (c == IDYES) action_open_explorer();
        }
    } else {
        append_log(L"--------------------------------------------------");
        append_log(L"[ERROR] Build finished with exit code: " + std::to_wstring(exitCode));
        MessageBoxW(m_hWnd, L"Build failed or completed with errors. See build log for details.", L"Build Warning", MB_ICONWARNING | MB_OK);
    }
}

// Recompile the kernel + software and write them into an EXISTING PlusOS disk
// (the one on the BlueSCSI SD card) WITHOUT reformatting it: the HFS volume's
// user data (home, etc, documents, Desktop items) is carried across, and the
// disk's own Apple SCSI driver is reused. This is the "update, don't reinstall"
// path - unlike "Build Hard Drive Image", which creates a fresh blank volume.
void BuilderWindow::action_update_image()
{
    if (m_isBuilding) {
        MessageBoxW(m_hWnd, L"A build is already in progress. Please wait for it to complete.", L"Build Running", MB_ICONINFORMATION | MB_OK);
        return;
    }
    if (m_backend != BB_P68_NATIVE) {
        int r = MessageBoxW(m_hWnd,
            L"The PlusOS native compilers (toolchain\\bin\\p68-cc.exe ...) are not built yet.\r\n\r\n"
            L"Click 'Setup Build Environment' to build them once with Visual Studio's C++ tools (MSVC).\r\n\r\n"
            L"Build them now?",
            L"P68 Toolchain Required", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1);
        if (r == IDYES) action_setup_environment();
        return;
    }

    // The update target is the existing PlusOS image to refresh. Prefer the one
    // on the BlueSCSI SD card; otherwise the last local image we built.
    std::wstring target;
    if (!m_blueScsiDrive.empty()) {
        std::wstring onCard = m_blueScsiDrive + L"HD60_512 PlusOS.hda";
        if (GetFileAttributesW(onCard.c_str()) != INVALID_FILE_ATTRIBUTES)
            target = onCard;
    }
    if (target.empty()) {
        std::wstring local = m_repoRoot + L"\\build_p68\\PlusOS_disk.hda";
        if (GetFileAttributesW(local.c_str()) != INVALID_FILE_ATTRIBUTES)
            target = local;
    }
    if (target.empty()) {
        MessageBoxW(m_hWnd,
            L"No existing PlusOS disk was found to update.\r\n\r\n"
            L"'Update' refreshes the kernel and software on a disk you already built, keeping your "
            L"files. There is nothing to update yet - use 'Compile & Build Hard Drive Image' first to "
            L"create the disk (insert the BlueSCSI SD card so it is written there).",
            L"Nothing to Update", MB_ICONINFORMATION | MB_OK);
        return;
    }

    std::wstring shown = target;
    int c = MessageBoxW(m_hWnd,
        (L"Update PlusOS on this disk, keeping all your files?\r\n\r\n    " + shown + L"\r\n\r\n"
         L"The kernel, boot blocks and every application will be recompiled and written over the disk. "
         L"Your documents and settings (home, etc, Desktop items) are preserved, and the disk's Apple "
         L"SCSI boot driver is reused.\r\n\r\nProceed?").c_str(),
        L"Update PlusOS (keep my files)", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
    if (c != IDYES) return;

    update_cfg_from_controls();
    save_config_to_file();

    m_isBuilding = true;
    m_isUpdate = true;
    EnableWindow(m_hBtnBuildImage, FALSE);
    EnableWindow(m_hBtnUpdate, FALSE);
    EnableWindow(m_hBtnBuildFloppy, FALSE);

    std::wstring machine = machine_keyword(m_cfg.target_hardware);
    if (m_cfg.target_hardware == 3) machine = L"universal";   // LC III -> reliable universal loader

    // The update writes in place, so there is no separate card-copy step.
    m_cardTarget.clear();
    m_lastBuiltImage = target;

    append_log(L"\r\n==================================================");
    append_log(L"[UPDATE] Refresh PlusOS kernel + software, keep user files");
    append_log(L"Target disk: " + target);
    append_log(L"Target Machine: " + machine);
    append_log(L"User data (home, etc, Desktop items) will be preserved; Apple SCSI driver reused.");
    append_log(L"Compiling & updating...");

    // make.cmd <machine> update UPDIMG=<target>
    // The target path can contain spaces ("HD60_512 PlusOS.hda"); make.cmd passes
    // %* through verbatim and the Makefile/hdbuild quote it, so wrap it in quotes.
    std::wstring extra = L" UPDIMG=\"" + target + L"\"";
    std::wstring cmd = build_make_command(machine + L" update", extra);
    RunProcessAsyncWithLogging(m_hWnd, WM_BUILD_COMPLETED, cmd, L"", m_repoRoot);
}

// Check an admin password against the OTA server's /admin/login. Returns true
// if the server accepts it. Uses WinINet so there is no extra dependency.
static bool ota_check_admin(const std::wstring& serverUrl, const std::wstring& password,
                            std::wstring& err)
{
    // Parse scheme://host[:port]
    URL_COMPONENTSW uc = {}; uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {}, path[1024] = {};
    uc.lpszHostName = host; uc.dwHostNameLength = 255;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 1023;
    if (!InternetCrackUrlW(serverUrl.c_str(), 0, 0, &uc)) { err = L"bad server URL"; return false; }
    bool https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    INTERNET_PORT port = uc.nPort ? uc.nPort : (https ? 443 : 80);

    HINTERNET hNet = InternetOpenW(L"PlusOSDiskStudio", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hNet) { err = L"InternetOpen failed"; return false; }
    bool ok = false;
    HINTERNET hConn = InternetConnectW(hNet, host, port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
    if (hConn) {
        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | (https ? INTERNET_FLAG_SECURE : 0);
        HINTERNET hReq = HttpOpenRequestW(hConn, L"POST", L"/admin/login", nullptr, nullptr, nullptr, flags, 0);
        if (hReq) {
            // JSON body {"password":"..."} — escape backslashes and quotes.
            std::string pw;
            for (wchar_t c : password) {
                if (c == L'"' || c == L'\\') pw += '\\';
                if (c < 128) pw += (char)c; else pw += '?';
            }
            std::string body = "{\"password\":\"" + pw + "\"}";
            const wchar_t* hdrs = L"Content-Type: application/json\r\n";
            if (HttpSendRequestW(hReq, hdrs, (DWORD)wcslen(hdrs), (LPVOID)body.data(), (DWORD)body.size())) {
                DWORD code = 0, len = sizeof(code);
                HttpQueryInfoW(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &code, &len, nullptr);
                ok = (code == 200);
                if (!ok) err = L"server rejected the password (HTTP " + std::to_wstring(code) + L")";
            } else {
                err = L"could not reach the server";
            }
            InternetCloseHandle(hReq);
        }
        InternetCloseHandle(hConn);
    } else {
        err = L"could not connect to the server";
    }
    InternetCloseHandle(hNet);
    return ok;
}

void BuilderWindow::action_push_update()
{
    if (m_isBuilding) {
        MessageBoxW(m_hWnd, L"A build is already in progress. Please wait.", L"Build Running", MB_ICONINFORMATION | MB_OK);
        return;
    }
    if (m_backend != BB_P68_NATIVE) {
        int r = MessageBoxW(m_hWnd,
            L"The PlusOS native compilers are not built yet.\r\n\r\nBuild them now with Visual Studio's C++ tools?",
            L"P68 Toolchain Required", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1);
        if (r == IDYES) action_setup_environment();
        return;
    }

    // 1. Server URL (remembered across prompts via a static).
    static std::wstring server = L"https://ota.d0a.net";
    if (!prompt_text(m_hWnd, m_hInstance, L"OTA update server URL:", server, false)) return;
    if (server.empty()) return;

    // 2. Admin login (once per session). The server verifies it; the actual
    //    publish is additionally gated by the signing key (otaserver\.publish_key),
    //    so this password alone cannot publish from a machine without the key.
    if (!m_otaAdmin) {
        std::wstring pw;
        if (!prompt_text(m_hWnd, m_hInstance, L"OTA admin password:", pw, true)) return;
        std::wstring err;
        if (!ota_check_admin(server, pw, err)) {
            MessageBoxW(m_hWnd, (L"Admin login failed: " + err).c_str(),
                        L"Push Update", MB_ICONERROR | MB_OK);
            return;
        }
        m_otaAdmin = true;
    }

    // 3. Channel + title for this release.
    static std::wstring channel = L"stable";
    if (!prompt_text(m_hWnd, m_hInstance, L"Release channel (stable / beta / nightly):", channel, false)) return;
    std::wstring title = L"";
    if (!prompt_text(m_hWnd, m_hInstance, L"Short title for this update:", title, false)) return;

    update_cfg_from_controls();
    save_config_to_file();

    std::wstring machine = machine_keyword(m_cfg.target_hardware);
    if (m_cfg.target_hardware == 3) machine = L"universal";

    m_isBuilding = true;
    EnableWindow(m_hBtnBuildImage, FALSE);
    EnableWindow(m_hBtnUpdate, FALSE);
    EnableWindow(m_hBtnPushUpdate, FALSE);
    EnableWindow(m_hBtnBuildFloppy, FALSE);

    append_log(L"\r\n==================================================");
    append_log(L"[PUSH UPDATE] Compile + publish a release to the OTA server");
    append_log(L"Server: " + server);
    append_log(L"Channel: " + channel + L"   Machine: " + machine);
    if (!title.empty()) append_log(L"Title: " + title);
    append_log(L"The AI assistant will generate release notes + per-app changelogs.");
    append_log(L"Compiling & publishing (signed with otaserver\\.publish_key)...");

    // make.cmd <machine> ota OTA_SERVER=.. OTA_CHANNEL=.. OTA_TITLE=".."
    std::wstring extra = L" OTA_SERVER=\"" + server + L"\" OTA_CHANNEL=" + channel;
    if (!title.empty()) extra += L" OTA_TITLE=\"" + title + L"\"";
    std::wstring cmd = build_make_command(machine + L" ota", extra);
    RunProcessAsyncWithLogging(m_hWnd, WM_BUILD_COMPLETED, cmd, L"", m_repoRoot);
}

void BuilderWindow::action_build_floppy()
{
    if (m_isBuilding) {
        MessageBoxW(m_hWnd, L"A build is already in progress. Please wait for it to complete.", L"Build Running", MB_ICONINFORMATION | MB_OK);
        return;
    }

    if (m_backend != BB_P68_NATIVE) {
        int r = MessageBoxW(m_hWnd,
            L"The PlusOS native compilers are not built yet.\r\n\r\nBuild them now with Visual Studio's C++ tools?",
            L"P68 Toolchain Required", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1);
        if (r == IDYES) action_setup_environment();
        return;
    }

    update_cfg_from_controls();     /* capture the current Target Machine selection */

    uint64_t appBytes = get_total_selected_size_bytes();
    uint64_t baseBytes = get_base_system_bytes();
    uint64_t totalBootable = baseBytes + appBytes;
    std::wstring appStr = format_size_human(appBytes);
    std::wstring baseStr = format_size_human(baseBytes);
    std::wstring totalStr = format_size_human(totalBootable);

    if (totalBootable > 1440 * 1024) {
        std::wstring msg = L"Floppy Size Warning:\r\n\r\n"
            L"Total bootable footprint: " + totalStr + L"\r\n"
            L" - Base OS Core (Kernel, Boot Blocks & HFS Overhead): " + baseStr + L" (Required to Boot)\r\n"
            L" - Selected Software: " + appStr + L"\r\n\r\n"
            L"This exceeds the 1.44 MB physical capacity limit of a Macintosh floppy disk!\r\n\r\n"
            L"When compiling for floppy, total bootable size must stay under 800 KB (for standard 800K disks) "
            L"or under 1.44 MB (for 1.4M High Density disks).\r\n\r\n"
            L"The bootable floppy will be built using the minimal essential system preset (~500 KB).\r\n\r\n"
            L"Do you want to proceed?";
        int res = MessageBoxW(m_hWnd, msg.c_str(), L"Floppy Capacity Exceeded", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
        if (res != IDYES) return;
    } else if (totalBootable > 800 * 1024) {
        uint64_t maxSoftware800 = (800 * 1024 > baseBytes) ? ((800 * 1024) - baseBytes) : 0;
        std::wstring msg = L"800K Floppy Capacity Notice:\r\n\r\n"
            L"Total bootable footprint: " + totalStr + L"\r\n"
            L" - Base OS Core (Kernel, Boot Blocks & HFS Overhead): " + baseStr + L" (Required to Boot)\r\n"
            L" - Selected Software: " + appStr + L"\r\n\r\n"
            L"This exceeds the 800 KB limit for standard 800K floppy disks (Mac Plus / original Mac SE), "
            L"requiring a 1.44 MB (1.4M) High Density floppy disk.\r\n\r\n"
            L"If targeting an 800K disk drive, please uncheck packages to keep software under " + format_size_human(maxSoftware800) + L".\r\n\r\n"
            L"Proceed with generating 1.44M High Density floppy image?";
        int res = MessageBoxW(m_hWnd, msg.c_str(), L"800K Floppy Notice", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1);
        if (res != IDYES) return;
    }

    save_config_to_file();

    m_isBuilding = true;
    EnableWindow(m_hBtnBuildImage, FALSE);
    EnableWindow(m_hBtnBuildFloppy, FALSE);

    append_log(L"\r\n==================================================");
    append_log(L"Starting Minimal Bootable Installer Floppy Generation...");
    append_log(L"Target: images\\PlusOS_Floppy.dsk (Macintosh HFS Floppy)");
    append_log(L"Base OS Core: " + baseStr + L" (Kernel, Boot Blocks & HFS Overhead - Required)");
    append_log(L"Selected Software: " + appStr);
    append_log(L"Total Bootable Footprint: " + totalStr);
    if (totalBootable <= 800 * 1024) {
        uint64_t free800 = (800 * 1024) - totalBootable;
        append_log(L"Capacity Check: OK for both 800K & 1.44M floppy disks (Free: " + format_size_human(free800) + L" on 800K).");
    } else if (totalBootable <= 1440 * 1024) {
        uint64_t free1440 = (1440 * 1024) - totalBootable;
        append_log(L"Capacity Check: Fits 1.44M HD Floppy ONLY (Free: " + format_size_human(free1440) + L"). Exceeds 800K.");
    } else {
        append_log(L"Capacity Check: Exceeds 1.44M floppy limit! (Hard Drive install recommended).");
    }
    append_log(L"Essential boot binaries & installer included (zero games).");

    std::wstring rootDir = m_repoRoot;
    /* The floppy's boot block stays the universal minimal loader, but the KERNEL
     * it bundles is machine-specific, so pass the Target Machine to make.cmd. */
    std::wstring machine = machine_keyword(m_cfg.target_hardware);
    append_log(L"Target Machine: " + machine);
    append_log(L"Compiler: P68 native toolchain  (image via tools\\hdbuild)");
    append_log(L"Running 'make.cmd " + machine + L" floppy' in a worker thread...");

    std::wstring cmd = build_make_command(machine + L" floppy", L"");
    RunProcessAsyncWithLogging(m_hWnd, WM_FLOPPY_COMPLETED, cmd, L"", rootDir);
}

void BuilderWindow::on_floppy_finished(DWORD exitCode)
{
    m_isBuilding = false;
    EnableWindow(m_hBtnBuildImage, TRUE);
    EnableWindow(m_hBtnBuildFloppy, TRUE);

    if (exitCode == 0) {
        append_log(L"--------------------------------------------------");
        append_log(L"[SUCCESS] Minimal bootable floppy successfully generated!");
        append_log(L"Image: images\\PlusOS_Floppy.dsk");
        append_log(L"Ready! You can now boot this floppy in Mini vMac or test on hardware.");

        int c = MessageBoxW(m_hWnd,
            L"Bootable minimal installer floppy generated successfully!\r\n(images\\PlusOS_Floppy.dsk)\r\n\r\nWould you like to open it in the Disk Explorer now?",
            L"Floppy Build Successful", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1);
        if (c == IDYES && m_explorerWin) {
            m_explorerWin->open_image(m_repoRoot + L"\\images\\PlusOS_Floppy.dsk");
            m_explorerWin->show();
        }
    } else {
        append_log(L"--------------------------------------------------");
        append_log(L"[ERROR] Floppy build finished with exit code: " + std::to_wstring(exitCode));
        MessageBoxW(m_hWnd, L"Floppy build failed or completed with errors.", L"Build Warning", MB_ICONWARNING | MB_OK);
    }
}

void BuilderWindow::action_setup_environment()
{
    append_log(L"\r\n==================================================");
    append_log(L"[SETUP] Building the PlusOS native P68 toolchain (no WSL / Linux)");
    append_log(L"Repository: " + m_repoRoot);

    // Step 1: Python companion packages for the disk builder (quick, synchronous).
    append_log(L"Installing Python companion packages (tools\\hdbuild dependencies)...");
    std::wstring pipCmd = L"cmd.exe /c \"cd /d \"" + m_repoRoot +
        L"\" && (py -m pip install -r requirements.txt 2>nul || python -m pip install -r requirements.txt 2>nul || pip install -r requirements.txt 2>nul)\"";
    {
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};
        std::vector<WCHAR> pipVec(pipCmd.begin(), pipCmd.end());
        pipVec.push_back(0);
        if (CreateProcessW(nullptr, pipVec.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                           nullptr, nullptr, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 15000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        append_log(L"[OK] Python packages installed (or already present).");
    }

    // Step 2: build the P68 tools with Visual Studio's C++ compiler (toolchain\build.cmd
    // finds vcvars itself). This produces p68-cc / p68-as / p68-ld / p68-ar / p68-make.
    std::wstring buildCmd = m_repoRoot + L"\\toolchain\\build.cmd";
    if (GetFileAttributesW(buildCmd.c_str()) == INVALID_FILE_ATTRIBUTES) {
        append_log(L"[ERROR] Cannot find toolchain\\build.cmd — the repository may be incomplete.");
        MessageBoxW(m_hWnd,
            L"toolchain\\build.cmd was not found.\r\n\r\nThe PlusOS repository appears incomplete.",
            L"Setup Error", MB_ICONERROR | MB_OK);
        return;
    }
    append_log(L"Compiling the P68 toolchain with MSVC (Visual Studio C++ Build Tools)...");
    append_log(L"This takes under a minute. If it fails, install \"Desktop development with C++\".");
    append_log(L"--------------------------------------------------");
    std::wstring cmd = L"cmd.exe /c \"cd /d \"" + m_repoRoot + L"\" && call toolchain\\build.cmd\"";
    RunProcessAsyncWithLogging(m_hWnd, WM_SETUP_COMPLETED, cmd, L"", m_repoRoot);
}

// ---------------------------------------------------------------------------
// Boot VM helpers
// ---------------------------------------------------------------------------

// Scan well-known locations for the Mini vMac (BlueSCSI patched) executable.
static std::wstring find_vmac_exe(const std::wstring& repoRoot) {
    static const wchar_t* names[] = {
        L"Mini vMac (BlueSCSI patched).exe",
        L"Mini vMac.exe",
        nullptr
    };
    std::wstring dirs[] = {
        repoRoot + L"\\tools\\emulator\\",
        repoRoot + L"\\",
        L"F:\\",
        L""
    };
    for (const auto& dir : dirs) {
        if (dir.empty()) break;
        for (int i = 0; names[i]; i++) {
            std::wstring p = dir + names[i];
            if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES)
                return p;
        }
    }
    return L"";
}

// Scan for a Mac ROM file that Mini vMac accepts.
// Mini vMac must have the ROM in its working directory, so we check there first,
// then check other common spots and offer to copy if found elsewhere.
static std::wstring find_rom_file(const std::wstring& exeDir,
                                  const std::wstring& repoRoot) {
    static const wchar_t* names[] = {
        L"vMac.ROM",
        L"Mac Plus.ROM",
        L"MacPlus.ROM",
        L"Mac SE.ROM",
        L"MacSE.ROM",
        L"vMacSE.ROM",
        L"ROM",        // some builds use just "ROM"
        nullptr
    };
    std::wstring searchDirs[] = {
        exeDir + L"\\",
        repoRoot + L"\\images\\",
        repoRoot + L"\\",
        L"F:\\",
        L""
    };
    for (const auto& dir : searchDirs) {
        if (dir.empty()) break;
        for (int i = 0; names[i]; i++) {
            std::wstring p = dir + names[i];
            if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES)
                return p;
        }
    }
    return L"";
}

// Return the best available disk image to boot.
std::wstring BuilderWindow::find_boot_image() const {
    std::vector<std::wstring> candidates = {
        m_cfg.image_path,
        m_repoRoot + L"\\images\\BOOTTEST.hda",
        m_repoRoot + L"\\images\\PlusOS.hda",
        m_repoRoot + L"\\images\\HD60_512 PlusOS.hda",
    };
    if (!m_blueScsiDrive.empty()) {
        std::wstring sd = m_blueScsiDrive;
        if (!sd.empty() && sd.back() != L'\\') sd += L'\\';
        candidates.push_back(sd + L"BOOTTEST.hda");
        candidates.push_back(sd + L"HD60_512 PlusOS.hda");
        candidates.push_back(sd + L"PlusOS.hda");
    }
    for (const auto& p : candidates) {
        if (!p.empty() && GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES)
            return p;
    }
    return L"";
}

void BuilderWindow::action_boot_vm()
{
    // ── 1. Find the emulator ──────────────────────────────────────────────
    std::wstring exePath = find_vmac_exe(m_repoRoot);
    if (exePath.empty()) {
        append_log(L"\r\n[BOOT VM] Emulator not found.");
        MessageBoxW(m_hWnd,
            L"Mini vMac (BlueSCSI patched) not found.\r\n\r\n"
            L"Place the emulator in one of these locations:\r\n"
            L"  tools\\emulator\\Mini vMac (BlueSCSI patched).exe\r\n"
            L"  tools\\emulator\\Mini vMac.exe\r\n\r\n"
            L"The emulator is not included in the PlusOS repository.\r\n"
            L"Download a BlueSCSI-patched Mini vMac build from the project wiki.",
            L"Emulator Not Found", MB_ICONWARNING | MB_OK);
        return;
    }

    // Derive exe directory (Mini vMac looks for the ROM in its working dir)
    std::wstring exeDir = exePath;
    size_t slash = exeDir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) exeDir = exeDir.substr(0, slash);

    // ── 2. Find the ROM ───────────────────────────────────────────────────
    std::wstring romPath = find_rom_file(exeDir, m_repoRoot);
    if (romPath.empty()) {
        append_log(L"[BOOT VM] ROM file not found.");
        std::wstring romMsg =
            L"Mac ROM file not found.\r\n\r\n"
            L"Mini vMac requires a Macintosh ROM dump (vMac.ROM) placed in the\r\n"
            L"same folder as the emulator:\r\n"
            L"  " + exeDir + L"\\vMac.ROM\r\n\r\n"
            L"This file is not included in the repository.\r\n"
            L"Dump the ROM from your own Macintosh or use a legal source.";
        MessageBoxW(m_hWnd, romMsg.c_str(), L"ROM Not Found", MB_ICONWARNING | MB_OK);
        return;
    }

    // If the ROM is not already in the exe dir, offer to copy it there
    std::wstring romDir = romPath;
    size_t rs = romDir.find_last_of(L"\\/");
    if (rs != std::wstring::npos) romDir = romDir.substr(0, rs);

    if (_wcsicmp(romDir.c_str(), exeDir.c_str()) != 0) {
        std::wstring romFile = romPath.substr(romPath.find_last_of(L"\\/") + 1);
        std::wstring destRom = exeDir + L"\\" + romFile;
        int r = MessageBoxW(m_hWnd,
            (L"ROM found outside the emulator folder:\r\n  " + romPath +
             L"\r\n\r\nMini vMac requires the ROM in the same folder as the .exe.\r\n\r\n"
             L"Copy it there now?\r\n  → " + destRom).c_str(),
            L"Copy ROM?", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
        if (r == IDYES) {
            if (!CopyFileW(romPath.c_str(), destRom.c_str(), FALSE)) {
                DWORD err = GetLastError();
                MessageBoxW(m_hWnd,
                    (L"Failed to copy ROM (error " + std::to_wstring(err) + L").\r\n"
                     L"Copy it manually to: " + destRom).c_str(),
                    L"Copy Failed", MB_ICONERROR | MB_OK);
                return;
            }
            romPath = destRom;
            append_log(L"[BOOT VM] ROM copied to emulator folder.");
        } else {
            return;
        }
    }

    // ── 3. Find the disk image ────────────────────────────────────────────
    std::wstring imagePath = find_boot_image();
    if (imagePath.empty()) {
        append_log(L"[BOOT VM] No disk image found.");
        MessageBoxW(m_hWnd,
            L"No hard drive image found.\r\n\r\n"
            L"Build the image first using 'Compile & Build Hard Drive Image',\r\n"
            L"or place a .hda file in:\r\n"
            L"  images\\BOOTTEST.hda\r\n"
            L"  images\\PlusOS.hda",
            L"No Disk Image", MB_ICONWARNING | MB_OK);
        return;
    }

    // ── 4. Launch ─────────────────────────────────────────────────────────
    append_log(L"\r\n==================================================");
    append_log(L"[BOOT VM] Launching Mini vMac (BlueSCSI patched)");
    append_log(L"Emulator : " + exePath);
    append_log(L"ROM      : " + romPath);
    append_log(L"Image    : " + imagePath);

    // Close any existing Mini vMac instance so it doesn't lock the image
    {
        HWND hVMac = FindWindowW(nullptr, L"Mini vMac");
        if (!hVMac) hVMac = FindWindowW(nullptr, L"Mini vMac (BlueSCSI patched)");
        if (hVMac) {
            append_log(L"Closing existing Mini vMac instance...");
            PostMessageW(hVMac, WM_CLOSE, 0, 0);
            Sleep(400);
        }
    }

    std::wstring cmdLine = L"\"" + exePath + L"\" \"" + imagePath + L"\"";
    std::vector<WCHAR> cmd(cmdLine.begin(), cmdLine.end());
    cmd.push_back(0);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                       0, nullptr, exeDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        append_log(L"[OK] Mini vMac launched. Enjoy PlusOS!");
    } else {
        DWORD err = GetLastError();
        append_log(L"[ERROR] Failed to launch emulator (Win32 error " + std::to_wstring(err) + L").");
        MessageBoxW(m_hWnd,
            (L"Failed to launch Mini vMac.\r\nWin32 error: " + std::to_wstring(err)).c_str(),
            L"Launch Failed", MB_ICONERROR | MB_OK);
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

    case WM_BUILD_LOG_LINE: {
        std::wstring* pLine = (std::wstring*)lParam;
        if (pLine) {
            append_log(*pLine);
            delete pLine;
        }
        return 0;
    }

    case WM_BUILD_COMPLETED: {
        DWORD exitCode = (DWORD)wParam;
        on_build_finished(exitCode);
        return 0;
    }

    case WM_FLOPPY_COMPLETED: {
        DWORD exitCode = (DWORD)wParam;
        on_floppy_finished(exitCode);
        return 0;
    }

    case WM_SETUP_COMPLETED: {
        DWORD exitCode = (DWORD)wParam;
        append_log(L"--------------------------------------------------");
        if (exitCode == 0) {
            // Re-probe for the P68 tools now that toolchain\build.cmd has finished
            detect_build_backend();
            if (m_backend == BB_P68_NATIVE) {
                append_log(L"[SUCCESS] P68 native toolchain built!");
                append_log(L"Tools: " + m_p68Dir);
                append_log(L"You can now build PlusOS with 'Compile & Build Hard Drive Image'.");
            } else {
                append_log(L"[WARNING] The P68 tools were not detected after setup — check the log above.");
            }
        } else {
            append_log(L"[ERROR] Toolchain build exited with code: " + std::to_wstring(exitCode));
            append_log(L"Install Visual Studio with the \"Desktop development with C++\" workload, then retry.");
            MessageBoxW(m_hWnd,
                L"Building the P68 toolchain failed.\r\n\r\n"
                L"The most common cause is missing Visual Studio C++ tools.\r\n"
                L"Install \"Desktop development with C++\" (or the Build Tools for Visual Studio),\r\n"
                L"then click Setup again.\r\n\r\n"
                L"You can also build manually:  toolchain\\build.cmd",
                L"Setup Warning", MB_ICONWARNING | MB_OK);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case IDB_IMAGE_BROWSE_BTN: action_browse_image(); break;
        case IDB_BUILD_BTN: action_build_image(); break;
        case IDB_UPDATE_BTN: action_update_image(); break;
        case IDB_PUSH_UPDATE_BTN: action_push_update(); break;
        case IDB_BUILD_FLOPPY_BTN: action_build_floppy(); break;
        case IDB_OPEN_EXPLORER_BTN: action_open_explorer(); break;
        case IDB_SAVE_CFG_BTN:
            if (save_config_to_file()) {
                MessageBoxW(m_hWnd, L"Configuration saved to multiboot.cfg successfully!", L"Saved", MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDB_INJECT_BTN: action_inject_external(); break;
        case IDB_SETUP_ENV_BTN: action_setup_environment(); break;
        case IDB_BOOT_VM_BTN: action_boot_vm(); break;
        case IDB_BOOT_ADD_BTN: on_boot_add(); break;
        case IDB_BOOT_EDIT_BTN: on_boot_edit(); break;
        case IDB_BOOT_REMOVE_BTN: on_boot_remove(); break;
        case IDB_BOOT_UP_BTN: on_boot_up(); break;
        case IDB_BOOT_DOWN_BTN: on_boot_down(); break;
        case IDB_APPS_SELECT_ALL_BTN: on_apps_select_all(); break;
        case IDB_APPS_SELECT_NONE_BTN: on_apps_select_none(); break;
        case IDB_APPS_SYS_ONLY_BTN: on_apps_sys_only(); break;
        case IDB_APPS_GAMES_ONLY_BTN: on_apps_games_only(); break;
        case IDB_APPS_ADD_BTN: on_apps_add_custom(); break;
        case IDB_APPS_REMOVE_BTN: on_apps_remove_selected(); break;
        case IDB_KPROFILE_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) on_kprofile_changed();
            break;
        default:
            if (id >= IDB_KMOD_FIRST && id <= IDB_KMOD_LAST &&
                HIWORD(wParam) == BN_CLICKED) {
                on_kmod_toggled();
            }
            break;
        }
        return 0;
    }

    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm && pnm->idFrom == IDB_APPS_LISTVIEW && pnm->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
            if ((pnmv->uChanged & LVIF_STATE) &&
                ((pnmv->uOldState & LVIS_STATEIMAGEMASK) != (pnmv->uNewState & LVIS_STATEIMAGEMASK))) {
                if (!m_populatingApps && pnmv->iItem >= 0) {
                    WCHAR itemBuf[128] = L"";
                    ListView_GetItemText(m_hListViewApps, pnmv->iItem, 0, itemBuf, 128);
                    if (_wcsicmp(itemBuf, L"SHELL") == 0) {
                        if (!ListView_GetCheckState(m_hListViewApps, pnmv->iItem)) {
                            // User clicked to uncheck SHELL: force it back immediately!
                            m_populatingApps = true;
                            ListView_SetCheckState(m_hListViewApps, pnmv->iItem, TRUE);
                            m_populatingApps = false;
                            append_log(L"[NOTICE] PlusOS Command Shell ('SHELL') is the core runtime environment and cannot be unselected.");
                        }
                    }
                }
                sync_included_apps_from_list();
            }
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
