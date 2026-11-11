#include "builder_window.h"
#include "explorer_window.h"
#include <commdlg.h>
#include <shellapi.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <thread>

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

// Convert "C:\foo\bar"  →  "/mnt/c/foo/bar"  (WSL path format)
static std::wstring win_to_wsl_path(const std::wstring& winPath) {
    if (winPath.length() >= 2 && winPath[1] == L':') {
        WCHAR drive = (WCHAR)towlower(winPath[0]);
        std::wstring rest = winPath.substr(2);
        for (auto& c : rest) {
            if (c == L'\\') c = L'/';
        }
        return L"/mnt/" + std::wstring(1, drive) + rest;
    }
    return winPath;
}

// Convert "C:\foo\bar"  →  "/c/foo/bar"  (MSYS2 mount format)
static std::wstring win_to_msys2_path(const std::wstring& winPath) {
    if (winPath.length() >= 2 && winPath[1] == L':') {
        WCHAR drive = (WCHAR)towlower(winPath[0]);
        std::wstring rest = winPath.substr(2);
        for (auto& c : rest) {
            if (c == L'\\') c = L'/';
        }
        return L"/" + std::wstring(1, drive) + rest;
    }
    return winPath;
}

// Read a key=value text file (env.txt written by setup.ps1)
static std::map<std::wstring, std::wstring> read_env_txt(const std::wstring& path) {
    std::map<std::wstring, std::wstring> kv;
    std::ifstream f(path);
    if (!f.is_open()) return kv;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::wstring key   = to_wide(line.substr(0, eq));
        std::wstring value = to_wide(line.substr(eq + 1));
        // Trim trailing whitespace / CR
        while (!value.empty() && (value.back() == L'\r' || value.back() == L' '))
            value.pop_back();
        kv[key] = value;
    }
    return kv;
}

// Persist / recall the user's backend preference in tools\winbuild\build_config.txt
static BuildBackend load_backend_pref(const std::wstring& repoRoot) {
    std::wstring path = repoRoot + L"\\tools\\winbuild\\build_config.txt";
    std::ifstream f(path);
    if (!f.is_open()) return BB_NONE;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("BUILD_BACKEND=", 0) == 0) {
            std::string val = line.substr(14);
            while (!val.empty() && (val.back() == '\r' || val.back() == ' ')) val.pop_back();
            if (val == "wsl")   return BB_WSL;
            if (val == "msys2") return BB_MSYS2_LOCAL;
        }
    }
    return BB_NONE;
}

static void save_backend_pref(const std::wstring& repoRoot, BuildBackend backend) {
    std::wstring dir  = repoRoot + L"\\tools\\winbuild";
    CreateDirectoryW(dir.c_str(), nullptr);  // no-op if exists
    std::wstring path = dir + L"\\build_config.txt";
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) return;
    f << "# PlusOS build backend preference (written by Disk Studio)\n";
    f << "BUILD_BACKEND=" << (backend == BB_WSL ? "wsl" : "msys2") << "\n";
}

// Probe the build backend: MSYS2 (local/system) → WSL → nothing.
// Populates m_backend, m_backendBash, m_localToolchain.
void BuilderWindow::detect_build_backend() {
    m_backend       = BB_NONE;
    m_backendBash   = L"";
    m_localToolchain = L"";

    // 1. Read env.txt written by setup.ps1 (highest priority)
    std::wstring envTxt = m_repoRoot + L"\\tools\\winbuild\\env.txt";
    if (GetFileAttributesW(envTxt.c_str()) != INVALID_FILE_ATTRIBUTES) {
        auto kv = read_env_txt(envTxt);
        auto itBash = kv.find(L"MSYS2_BASH");
        auto itTC   = kv.find(L"TOOLCHAIN_DIR");
        if (itBash != kv.end() && GetFileAttributesW(itBash->second.c_str()) != INVALID_FILE_ATTRIBUTES) {
            m_backendBash = itBash->second;
            m_backend     = BB_MSYS2_LOCAL;
        }
        if (itTC != kv.end() && GetFileAttributesW(itTC->second.c_str()) != INVALID_FILE_ATTRIBUTES) {
            m_localToolchain = itTC->second;
        }
    }

    // 2. Local MSYS2 inside the repo (tools\winbuild\msys2\)
    if (m_backend == BB_NONE) {
        std::wstring localBash = m_repoRoot + L"\\tools\\winbuild\\msys2\\usr\\bin\\bash.exe";
        if (GetFileAttributesW(localBash.c_str()) != INVALID_FILE_ATTRIBUTES) {
            m_backendBash = localBash;
            m_backend     = BB_MSYS2_LOCAL;
        }
    }

    // 3. System-wide MSYS2 at C:\msys64 or C:\msys2
    if (m_backend == BB_NONE) {
        const wchar_t* sysLocations[] = {
            L"C:\\msys64\\usr\\bin\\bash.exe",
            L"C:\\msys2\\usr\\bin\\bash.exe",
            nullptr
        };
        for (int i = 0; sysLocations[i]; i++) {
            if (GetFileAttributesW(sysLocations[i]) != INVALID_FILE_ATTRIBUTES) {
                m_backendBash = sysLocations[i];
                m_backend     = BB_MSYS2_SYSTEM;
                break;
            }
        }
    }

    // 4. WSL Ubuntu fallback
    if (m_backend == BB_NONE) {
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};
        WCHAR wslCheck[] = L"wsl.exe -d Ubuntu -- uname";
        if (CreateProcessW(nullptr, wslCheck, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                           nullptr, nullptr, &si, &pi)) {
            DWORD ec = 1;
            WaitForSingleObject(pi.hProcess, 5000);
            GetExitCodeProcess(pi.hProcess, &ec);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            if (ec == 0) m_backend = BB_WSL;
        }
    }

    // Also check for local toolchain even if MSYS2 came from env.txt
    if (m_localToolchain.empty()) {
        std::wstring localTC = m_repoRoot + L"\\tools\\winbuild\\toolchain";
        std::wstring tcGcc   = localTC + L"\\bin\\m68k-apple-macos-gcc.exe";
        if (GetFileAttributesW(tcGcc.c_str()) != INVALID_FILE_ATTRIBUTES) {
            m_localToolchain = localTC;
        }
    }
}

// Build the shell command to invoke 'make <target> [extras]' using the
// detected backend. Returns an empty string if no backend is available.
std::wstring BuilderWindow::build_make_command(const std::wstring& target,
                                               const std::wstring& extraFlags) const {
    if (m_backend == BB_MSYS2_LOCAL || m_backend == BB_MSYS2_SYSTEM) {
        std::wstring msys2Repo = win_to_msys2_path(m_repoRoot);
        std::wstring envSetup  = L"export MSYSTEM=UCRT64; export PATH='/ucrt64/bin:/usr/bin:$PATH'";
        std::wstring makeCmd   = L"make " + target + extraFlags;
        if (!m_localToolchain.empty()) {
            makeCmd += L" TOOLCHAIN='" + win_to_msys2_path(m_localToolchain) + L"'";
        }
        return L"\"" + m_backendBash + L"\" --login -c \"" +
               envSetup + L"; cd '" + msys2Repo + L"' && " + makeCmd + L"\"";
    }
    if (m_backend == BB_WSL) {
        std::wstring wslRepo = win_to_wsl_path(m_repoRoot);
        return L"wsl.exe -d Ubuntu --cd " + wslRepo + L" -- make " + target + extraFlags;
    }
    return L""; // BB_NONE
}

// Show a "choose build environment" dialog, save the preference, return the choice.
// Returns BB_NONE if the user cancelled.
// forSetup = true  → wording about setting up / installing
// forSetup = false → wording about building now
BuildBackend BuilderWindow::prompt_build_backend(bool forSetup) {
    // Decide the default button: saved pref > detected backend > MSYS2
    BuildBackend saved = load_backend_pref(m_repoRoot);
    if (saved == BB_NONE) saved = m_backend;
    if (saved == BB_NONE) saved = BB_MSYS2_LOCAL;  // default to native

    std::wstring currentName = (saved == BB_WSL) ? L"WSL Ubuntu" : L"MSYS2 (Windows-Native)";
    std::wstring verb = forSetup ? L"set up" : L"build with";

    std::wstring msg =
        L"Select Build Environment to " + verb + L":\r\n\r\n"
        L"  [Yes]    MSYS2  — Windows-native, no Linux required\r\n"
        L"  [No]     WSL Ubuntu  — Linux subsystem (requires WSL installed)\r\n"
        L"  [Cancel] Cancel\r\n\r\n"
        L"Last saved: " + currentName;

    UINT flags = MB_YESNOCANCEL | MB_ICONQUESTION;
    flags |= (saved == BB_WSL) ? MB_DEFBUTTON2 : MB_DEFBUTTON1;
    int r = MessageBoxW(m_hWnd, msg.c_str(), L"Build Environment", flags);

    if (r == IDYES) {
        save_backend_pref(m_repoRoot, BB_MSYS2_LOCAL);
        return BB_MSYS2_LOCAL;
    }
    if (r == IDNO) {
        save_backend_pref(m_repoRoot, BB_WSL);
        return BB_WSL;
    }
    return BB_NONE;  // cancelled
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
      m_hBtnBuildImage(nullptr), m_hBtnBuildFloppy(nullptr), m_hBtnOpenExplorer(nullptr),
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

    // Default included applications: all built-in apps
    for (int i = 0; i < NUM_BUILTIN_APPS; i++) {
        m_cfg.included_apps.insert(ALL_BUILTIN_APPS[i].name);
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
    // Show detected build backend
    switch (m_backend) {
    case BB_MSYS2_LOCAL:
        append_log(L"Build Backend: MSYS2 (portable, inside tools\\winbuild\\msys2\\)");
        if (!m_localToolchain.empty())
            append_log(L"Toolchain: Retro68 (local) " + m_localToolchain);
        else
            append_log(L"Toolchain: Retro68 — run Setup to build it.");
        break;
    case BB_MSYS2_SYSTEM:
        append_log(L"Build Backend: MSYS2 (system-wide)  " + m_backendBash);
        if (!m_localToolchain.empty())
            append_log(L"Toolchain: Retro68 (local) " + m_localToolchain);
        else
            append_log(L"Toolchain: Retro68 — run Setup to build it.");
        break;
    case BB_WSL:
        append_log(L"Build Backend: WSL Ubuntu (legacy)");
        append_log(L"Tip: Run 'Auto-Setup Build Environment' to migrate to native Windows build.");
        break;
    default:
        append_log(L"Build Backend: NOT FOUND — click 'Auto-Setup Build Environment' to install.");
        break;
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
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 510, 450, 205, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpActions, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnBuildImage = CreateWindowExW(0, L"BUTTON", L"Compile & Build Hard Drive Image",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON, 25, 532, 210, 32, m_hWnd, (HMENU)IDB_BUILD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBuildImage, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnBuildFloppy = CreateWindowExW(0, L"BUTTON", L"Build Bootable Floppy (.dsk)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 245, 532, 210, 32, m_hWnd, (HMENU)IDB_BUILD_FLOPPY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBuildFloppy, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hBtnOpenExplorer = CreateWindowExW(0, L"BUTTON", L"Open Disk Explorer / Manager",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 568, 210, 30, m_hWnd, (HMENU)IDB_OPEN_EXPLORER_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnOpenExplorer, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnSaveConfig = CreateWindowExW(0, L"BUTTON", L"Save Configuration",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 245, 568, 210, 30, m_hWnd, (HMENU)IDB_SAVE_CFG_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnSaveConfig, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnInjectApp = CreateWindowExW(0, L"BUTTON", L"Inject 'Return to PlusOS' to External Disk / Floppy...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 602, 430, 30, m_hWnd, (HMENU)IDB_INJECT_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnInjectApp, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnSetupEnv = CreateWindowExW(0, L"BUTTON", L"Auto-Setup Build Environment (Windows-Native)...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 636, 430, 30, m_hWnd, (HMENU)IDB_SETUP_ENV_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnSetupEnv, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnBootVM = CreateWindowExW(0, L"BUTTON", L"Boot Hard Drive Image in Mini vMac (BlueSCSI)...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 25, 670, 430, 30, m_hWnd, (HMENU)IDB_BOOT_VM_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnBootVM, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // Group 5: Software Applications & Floppy Capacity (Column 2)
    HWND grpApps = CreateWindowExW(0, L"BUTTON", L" 5. Software Packages & Floppy Capacity ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 470, 10, 395, 705, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessageW(grpApps, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hListViewApps = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        480, 33, 375, 480, m_hWnd, (HMENU)IDB_APPS_LISTVIEW, m_hInstance, nullptr);
    SendMessageW(m_hListViewApps, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SendMessageW(m_hListViewApps, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    LVCOLUMNW appCol = {};
    appCol.mask = LVCF_TEXT | LVCF_WIDTH;
    appCol.cx = 145; appCol.pszText = (LPWSTR)L"Application"; ListView_InsertColumn(m_hListViewApps, 0, &appCol);
    appCol.cx = 75;  appCol.pszText = (LPWSTR)L"Category";    ListView_InsertColumn(m_hListViewApps, 1, &appCol);
    appCol.cx = 80;  appCol.pszText = (LPWSTR)L"Size";        ListView_InsertColumn(m_hListViewApps, 2, &appCol);
    appCol.cx = 65;  appCol.pszText = (LPWSTR)L"Origin";      ListView_InsertColumn(m_hListViewApps, 3, &appCol);

    m_hBtnAppsSelectAll = CreateWindowExW(0, L"BUTTON", L"All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        480, 520, 88, 26, m_hWnd, (HMENU)IDB_APPS_SELECT_ALL_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsSelectAll, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsSelectNone = CreateWindowExW(0, L"BUTTON", L"None", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        575, 520, 88, 26, m_hWnd, (HMENU)IDB_APPS_SELECT_NONE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsSelectNone, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsSysOnly = CreateWindowExW(0, L"BUTTON", L"System", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        670, 520, 90, 26, m_hWnd, (HMENU)IDB_APPS_SYS_ONLY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsSysOnly, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsGamesOnly = CreateWindowExW(0, L"BUTTON", L"Games", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        765, 520, 90, 26, m_hWnd, (HMENU)IDB_APPS_GAMES_ONLY_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsGamesOnly, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsAdd = CreateWindowExW(0, L"BUTTON", L"Add Custom App...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        480, 552, 183, 28, m_hWnd, (HMENU)IDB_APPS_ADD_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsAdd, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hBtnAppsRemove = CreateWindowExW(0, L"BUTTON", L"Remove Selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        672, 552, 183, 28, m_hWnd, (HMENU)IDB_APPS_REMOVE_BTN, m_hInstance, nullptr);
    SendMessageW(m_hBtnAppsRemove, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hStaticBaseSystem = CreateWindowExW(0, L"STATIC", L"Base OS Core: Calculating size...",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 582, 375, 18, m_hWnd, (HMENU)IDB_BASE_SYSTEM_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticBaseSystem, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hStaticAppCount = CreateWindowExW(0, L"STATIC", L"Selected Software: 66 of 66 packages (0 KB)",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 601, 375, 18, m_hWnd, (HMENU)IDB_APPS_COUNT_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticAppCount, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hStaticTotalDisk = CreateWindowExW(0, L"STATIC", L"Total Bootable Footprint: 0 KB (Base OS + Software)",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 620, 375, 19, m_hWnd, (HMENU)IDB_TOTAL_DISK_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticTotalDisk, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hStaticFloppyStatus = CreateWindowExW(0, L"STATIC", L"Floppy: Calculating size...",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 480, 641, 375, 20, m_hWnd, (HMENU)IDB_APPS_FLOPPY_STATUS_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticFloppyStatus, WM_SETFONT, (WPARAM)m_hBoldFont, TRUE);

    m_hStaticFloppyTip = CreateWindowExW(0, L"STATIC",
        L"Floppy Guide: Base OS (~191 KB) is essential and included on every disk. On 800K floppies, software must stay under ~609 KB (Total < 800 KB). 1.44M HD disks support up to 1.44 MB.",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 480, 663, 375, 46, m_hWnd, (HMENU)IDB_APPS_FLOPPY_TIP_STATIC, m_hInstance, nullptr);
    SendMessageW(m_hStaticFloppyTip, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

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

            // Check if already in built-in list
            bool isBuiltin = false;
            for (int i = 0; i < NUM_BUILTIN_APPS; i++) {
                if (_stricmp(ALL_BUILTIN_APPS[i].name, appName.c_str()) == 0) {
                    isBuiltin = true;
                    break;
                }
            }
            if (isBuiltin) continue;

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
    scan_compile_directory();
    m_populatingApps = true;
    ListView_DeleteAllItems(m_hListViewApps);

    int rowIndex = 0;
    // Built-in applications
    for (int i = 0; i < NUM_BUILTIN_APPS; i++) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = rowIndex;
        std::wstring nameW = to_wide(ALL_BUILTIN_APPS[i].name);
        item.pszText = (LPWSTR)nameW.c_str();
        ListView_InsertItem(m_hListViewApps, &item);

        std::wstring catW = to_wide(ALL_BUILTIN_APPS[i].category);
        ListView_SetItemText(m_hListViewApps, rowIndex, 1, (LPWSTR)catW.c_str());

        uint64_t sz = get_app_size_bytes(ALL_BUILTIN_APPS[i].name);
        std::wstring szStr = format_size_human(sz);
        ListView_SetItemText(m_hListViewApps, rowIndex, 2, (LPWSTR)szStr.c_str());

        bool isShell = (_stricmp(ALL_BUILTIN_APPS[i].name, "SHELL") == 0);
        if (isShell) {
            ListView_SetItemText(m_hListViewApps, rowIndex, 3, (LPWSTR)L"Built-in (Required)");
        } else {
            ListView_SetItemText(m_hListViewApps, rowIndex, 3, (LPWSTR)L"Built-in");
        }

        bool checked = isShell || (m_cfg.included_apps.find(ALL_BUILTIN_APPS[i].name) != m_cfg.included_apps.end());
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

/* After a BlueSCSI SD card is reinserted, WSL's /mnt/<drive> drvfs mount goes
 * stale: writes appear to succeed but never reach the card, so `make install`
 * silently installs nothing and the Mac keeps booting the OLD image. Refresh
 * the mount (umount + remount) before every build so the install really lands
 * on the card. Runs synchronously so the build that follows sees a live mount. */
void BuilderWindow::remount_bluescsi()
{
    if (m_blueScsiDrive.empty()) return;
    wchar_t up = m_blueScsiDrive[0];
    if (up >= L'a' && up <= L'z') up = (wchar_t)(up - L'a' + L'A');
    wchar_t lo = (up >= L'A' && up <= L'Z') ? (wchar_t)(up - L'A' + L'a') : up;

    std::wstring mnt = std::wstring(L"/mnt/") + lo;
    std::wstring cmd = L"wsl.exe -d Ubuntu -u root -- bash -lc \"umount -l " + mnt +
                       L" 2>/dev/null; mount -t drvfs " + up + L": " + mnt +
                       L" 2>/dev/null; true\"";
    append_log(std::wstring(L"Refreshing SD card mount (") + up + L":) so the install reaches the card...");

    STARTUPINFOW si; ZeroMemory(&si, sizeof si); si.cb = sizeof si;
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof pi);
    std::vector<wchar_t> buf(cmd.begin(), cmd.end()); buf.push_back(0);
    if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 20000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void BuilderWindow::action_build_image()
{
    if (m_isBuilding) {
        MessageBoxW(m_hWnd, L"A build is already in progress. Please wait for it to complete.", L"Build Running", MB_ICONINFORMATION | MB_OK);
        return;
    }

    // Always ask which build environment to use
    BuildBackend chosen = prompt_build_backend(false);
    if (chosen == BB_NONE) return;  // user cancelled

    update_cfg_from_controls();
    save_config_to_file();
    if (chosen == BB_WSL) remount_bluescsi();  // only needed for WSL drvfs mounts

    m_isBuilding = true;
    EnableWindow(m_hBtnBuildImage, FALSE);
    EnableWindow(m_hBtnBuildFloppy, FALSE);

    uint64_t appBytes = get_total_selected_size_bytes();
    uint64_t baseBytes = get_base_system_bytes();
    uint64_t totalBootable = baseBytes + appBytes;

    append_log(L"\r\n==================================================");
    append_log(L"[BUILD] Starting PlusOS Hard Drive Image Build Pipeline");
    append_log(L"Backend: " + std::wstring(chosen == BB_WSL ? L"WSL Ubuntu" : L"MSYS2 (Windows-Native)"));
    append_log(L"Target: " + m_cfg.image_path);
    append_log(L"Base OS Core: " + format_size_human(baseBytes) + L" (Kernel, Boot Blocks & HFS Overhead - Required)");
    append_log(L"Selected Software: " + format_size_human(appBytes));
    append_log(L"Total Bootable Footprint: " + format_size_human(totalBootable));
    append_log(L"Boot Preference: " + std::to_wstring(m_cfg.boot_pref));
    append_log(L"Default Theme: " + std::to_wstring(m_cfg.theme));
    append_log(L"Running 'make install' asynchronously in worker thread...");

    std::wstring rootDir = m_repoRoot;
    std::wstring machineFlag = L" MACHINE=se";
    if (m_cfg.target_hardware == 0) machineFlag = L" MACHINE=plus";
    else if (m_cfg.target_hardware == 1) machineFlag = L" MACHINE=se";
    else if (m_cfg.target_hardware == 2) machineFlag = L" MACHINE=classic";
    else if (m_cfg.target_hardware == 3) machineFlag = L" MACHINE=lc3";
    else if (m_cfg.target_hardware == 4) machineFlag = L" MACHINE=universal";

    std::wstring extraMake = machineFlag;
    if (!m_blueScsiDrive.empty()) {
        std::wstring sdPath = (chosen == BB_WSL)
            ? win_to_wsl_path(m_blueScsiDrive)
            : win_to_msys2_path(m_blueScsiDrive);
        while (!sdPath.empty() && sdPath.back() == L'/') sdPath.pop_back();
        extraMake += L" SDCARD=" + sdPath;
    }

    // Temporarily switch backend for this call so build_make_command uses chosen
    BuildBackend saved = m_backend;
    m_backend = chosen;
    std::wstring cmd = build_make_command(L"install", extraMake);
    m_backend = saved;

    std::wstring fallbackCmd = L"cmd.exe /c \"cd /d \"" + m_repoRoot + L"\" && call build.bat install" + machineFlag + L"\"";
    if (cmd.empty()) {
        append_log(L"[ERROR] Build environment not found. Run 'Auto-Setup Build Environment' first.");
        m_isBuilding = false;
        EnableWindow(m_hBtnBuildImage, TRUE);
        EnableWindow(m_hBtnBuildFloppy, TRUE);
        return;
    }

    RunProcessAsyncWithLogging(m_hWnd, WM_BUILD_COMPLETED, cmd, fallbackCmd, rootDir);
}

void BuilderWindow::on_build_finished(DWORD exitCode)
{
    m_isBuilding = false;
    EnableWindow(m_hBtnBuildImage, TRUE);
    EnableWindow(m_hBtnBuildFloppy, TRUE);

    if (exitCode == 0) {
        append_log(L"--------------------------------------------------");
        append_log(L"[SUCCESS] Hard drive build & installation succeeded!");
        append_log(L"Kernel, boot blocks, raw store, and all applications written.");
        append_log(L"Custom multiboot.cfg and Desktop Return-to-PlusOS seeded.");
        populate_apps_list();
        append_log(L"Ready! Click 'Open Disk Explorer / Manager' to inspect.");

        int c = MessageBoxW(m_hWnd,
            L"Hard drive build completed successfully!\r\n\r\nWould you like to open the Disk Explorer now to inspect files?",
            L"Build Successful", MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1);
        if (c == IDYES) {
            action_open_explorer();
        }
    } else {
        append_log(L"--------------------------------------------------");
        append_log(L"[ERROR] Build finished with exit code: " + std::to_wstring(exitCode));
        MessageBoxW(m_hWnd, L"Build failed or completed with errors. See build log for details.", L"Build Warning", MB_ICONWARNING | MB_OK);
    }
}

void BuilderWindow::action_build_floppy()
{
    if (m_isBuilding) {
        MessageBoxW(m_hWnd, L"A build is already in progress. Please wait for it to complete.", L"Build Running", MB_ICONINFORMATION | MB_OK);
        return;
    }

    // Always ask which build environment to use
    BuildBackend chosen = prompt_build_backend(false);
    if (chosen == BB_NONE) return;  // user cancelled

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
    append_log(L"Running 'make floppy' asynchronously in worker thread...");
    append_log(L"Essential boot binaries & installer included (zero games).");
    append_log(L"Running 'make floppy' asynchronously in worker thread...");

    std::wstring rootDir = m_repoRoot;
    /* The floppy's boot block stays the universal minimal loader, but the KERNEL
     * it bundles is machine-specific, so pass the Target Machine to make. */
    std::wstring machineFlag = L" MACHINE=se";
    if (m_cfg.target_hardware == 0) machineFlag = L" MACHINE=plus";
    else if (m_cfg.target_hardware == 1) machineFlag = L" MACHINE=se";
    else if (m_cfg.target_hardware == 2) machineFlag = L" MACHINE=classic";
    else if (m_cfg.target_hardware == 3) machineFlag = L" MACHINE=lc3";
    else if (m_cfg.target_hardware == 4) machineFlag = L" MACHINE=universal";
    append_log(L"Target Machine:" + machineFlag);
    append_log(L"Backend: " + std::wstring(chosen == BB_WSL ? L"WSL Ubuntu" : L"MSYS2 (Windows-Native)"));

    // Temporarily switch backend for this call so build_make_command uses chosen
    BuildBackend saved = m_backend;
    m_backend = chosen;
    std::wstring cmd = build_make_command(L"floppy", machineFlag);
    m_backend = saved;

    std::wstring fallbackCmd = L"cmd.exe /c \"cd /d \"" + m_repoRoot + L"\" && call build.bat floppy" + machineFlag + L"\"";
    if (cmd.empty()) {
        append_log(L"[ERROR] Build environment not found. Run 'Auto-Setup Build Environment' first.");
        m_isBuilding = false;
        EnableWindow(m_hBtnBuildImage, TRUE);
        EnableWindow(m_hBtnBuildFloppy, TRUE);
        return;
    }

    RunProcessAsyncWithLogging(m_hWnd, WM_FLOPPY_COMPLETED, cmd, fallbackCmd, rootDir);
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
    // Let the user choose (and persist) which backend to set up
    BuildBackend chosen = prompt_build_backend(true);
    if (chosen == BB_NONE) return;  // user cancelled

    append_log(L"\r\n==================================================");
    if (chosen == BB_WSL) {
        append_log(L"[SETUP] Setting Up WSL Ubuntu Build Environment");
    } else {
        append_log(L"[SETUP] Setting Up MSYS2 (Windows-Native) Build Environment");
    }
    append_log(L"Repository: " + m_repoRoot);

    // Step 1: Python pip dependencies (quick, synchronous)
    append_log(L"Installing Python companion packages...");
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

    if (chosen == BB_WSL) {
        // ── WSL path ──────────────────────────────────────────────────────
        append_log(L"Verifying WSL Ubuntu...");
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};
        WCHAR wslCheck[] = L"wsl.exe -d Ubuntu -- uname -a";
        bool wslOk = false;
        if (CreateProcessW(nullptr, wslCheck, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                           nullptr, nullptr, &si, &pi)) {
            DWORD ec = 1;
            WaitForSingleObject(pi.hProcess, 8000);
            GetExitCodeProcess(pi.hProcess, &ec);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            wslOk = (ec == 0);
        }
        if (!wslOk) {
            append_log(L"[ERROR] WSL Ubuntu is not installed or not responding.");
            append_log(L"        Open PowerShell as Administrator and run:  wsl --install -d Ubuntu");
            MessageBoxW(m_hWnd,
                L"WSL Ubuntu is not installed.\r\n\r\n"
                L"Open PowerShell as Administrator and run:\r\n"
                L"    wsl --install -d Ubuntu\r\n\r\n"
                L"After Ubuntu is installed and started once, click Setup again.",
                L"WSL Required", MB_ICONWARNING | MB_OK);
            return;
        }
        append_log(L"[OK] WSL Ubuntu is active.");
        // Run the bootstrap script inside WSL to install packages + Retro68
        std::wstring scriptWsl = win_to_wsl_path(m_repoRoot) + L"/tools/winbuild/bootstrap_toolchain.sh";
        // Adapt install prefix to WSL home
        std::wstring wslCmd = L"wsl.exe -d Ubuntu -- bash -l \"" + scriptWsl +
            L"\" \"$HOME/Retro68-build/toolchain\" \"" + win_to_wsl_path(m_repoRoot) + L"\"";
        append_log(L"Running bootstrap_toolchain.sh inside WSL Ubuntu (~20-40 min)...");
        append_log(L"--------------------------------------------------");
        RunProcessAsyncWithLogging(m_hWnd, WM_SETUP_COMPLETED, wslCmd, L"", m_repoRoot);
    } else {
        // ── MSYS2 path ────────────────────────────────────────────────────
        append_log(L"This will install MSYS2 into tools\\winbuild\\msys2\\,");
        append_log(L"then build Retro68 GCC cross-compiler from source (~20-40 min).");
        std::wstring setupPs1 = m_repoRoot + L"\\tools\\winbuild\\setup.ps1";
        if (GetFileAttributesW(setupPs1.c_str()) == INVALID_FILE_ATTRIBUTES) {
            append_log(L"[ERROR] Cannot find tools\\winbuild\\setup.ps1 — repo may be incomplete.");
            return;
        }
        append_log(L"Launching tools\\winbuild\\setup.ps1  (live output below)...");
        append_log(L"--------------------------------------------------");
        std::wstring psCmd = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \""
            + setupPs1 + L"\"";
        RunProcessAsyncWithLogging(m_hWnd, WM_SETUP_COMPLETED, psCmd, L"", m_repoRoot);
    }
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
            // Re-probe backend now that setup.ps1 has finished
            detect_build_backend();
            append_log(L"[SUCCESS] Build environment setup complete!");
            switch (m_backend) {
            case BB_MSYS2_LOCAL:
                append_log(L"Backend: MSYS2 (portable)  " + m_backendBash);
                if (!m_localToolchain.empty())
                    append_log(L"Toolchain: " + m_localToolchain);
                break;
            case BB_MSYS2_SYSTEM:
                append_log(L"Backend: MSYS2 (system)  " + m_backendBash);
                break;
            default:
                append_log(L"[WARNING] Backend not detected after setup — check log above.");
                break;
            }
            append_log(L"You can now build PlusOS using 'Build & Install Hard Drive Image'.");
        } else {
            append_log(L"[ERROR] Setup script exited with code: " + std::to_wstring(exitCode));
            append_log(L"Check the log above for details. You may need to run setup.ps1 manually.");
            MessageBoxW(m_hWnd,
                L"Build environment setup encountered errors.\r\n\r\n"
                L"See the build log for details.\r\n\r\n"
                L"You can also run setup manually:\r\n"
                L"  powershell -ExecutionPolicy Bypass -File tools\\winbuild\\setup.ps1",
                L"Setup Warning", MB_ICONWARNING | MB_OK);
        }
        return 0;
    }

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
