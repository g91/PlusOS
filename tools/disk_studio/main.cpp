#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include "builder_window.h"
#include "explorer_window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Check for CLI flags: --inject <image> [scsi_id], --help
    if (argv && argc >= 2) {
        std::wstring arg1 = argv[1];
        if (arg1 == L"--inject" && argc >= 3) {
            std::wstring imgPath = argv[2];
            uint32_t scsi_id = (argc >= 4) ? (uint32_t)_wtoi(argv[3]) : 6;
            std::string msg;
            bool ok = HFSFileSystem::inject_into_external_image(imgPath, scsi_id, &msg);
            LocalFree(argv);
            return ok ? 0 : 1;
        } else if (arg1 == L"--help" || arg1 == L"-h") {
            MessageBoxW(nullptr,
                L"PlusOS OS Studio & Disk Explorer\r\n\r\n"
                L"Usage:\r\n"
                L"  PlusOSDiskStudio.exe [image.hda]\r\n\r\n"
                L"CLI Injection:\r\n"
                L"  PlusOSDiskStudio.exe --inject <image.hda> [scsi_id]\r\n"
                L"  PlusOSDiskStudio.exe --help",
                L"PlusOS Studio Help", MB_OK);
            LocalFree(argv);
            return 0;
        }
    }

    BuilderWindow builderWin;
    ExplorerWindow explorerWin;

    if (!explorerWin.create(hInstance, &builderWin)) {
        MessageBoxW(nullptr, L"Failed to initialize Hard Drive Explorer window.", L"Startup Error", MB_ICONERROR | MB_OK);
        if (argv) LocalFree(argv);
        return 1;
    }

    if (!builderWin.create(hInstance, nCmdShow, &explorerWin)) {
        MessageBoxW(nullptr, L"Failed to initialize OS Builder window.", L"Startup Error", MB_ICONERROR | MB_OK);
        if (argv) LocalFree(argv);
        return 1;
    }

    // Check if an image path was passed on command line to open directly in Explorer
    if (argv && argc > 1) {
        std::wstring imgPath = argv[1];
        if (!imgPath.empty() && imgPath[0] != L'-' && imgPath[0] != L'/') {
            explorerWin.show(imgPath);
        }
    }
    if (argv) LocalFree(argv);

    // Win32 Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

int main()
{
    return wWinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineW(), SW_SHOW);
}
