#include <chrono>
#include <iostream>
#include <windows.h>
#include <cstdio>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt::Windows::Media::Control;

static GlobalSystemMediaTransportControlsSessionManager session_manager { nullptr };

HWND makewindow(HINSTANCE hInst);
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
#if _DEBUG
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
#endif

    winrt::init_apartment();
    session_manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

    HWND wnd = makewindow(hInst);
    std::cout << "wnd: " << wnd << std::endl;

    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid.hwndTarget = wnd;

    if (RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)) == FALSE) {
        DWORD err = GetLastError();
        std::cout << GetLastError() << std::endl; 
        MessageBoxA(NULL, ("GetLastError() -> " + std::to_string(err)).c_str(), "mediakeyfix shits fucked", MB_OK | MB_ICONERROR);
        return 0;
    }

    MSG msg;
    while (GetMessage(&msg, wnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

HWND makewindow(HINSTANCE hInst) {
    WNDCLASS wndc = {};
    wndc.style = 0;
    wndc.hInstance = hInst;
    wndc.lpfnWndProc = WindowProcedure;
    wndc.lpszClassName = L"bruhClass";

    RegisterClass(&wndc);

    return CreateWindowExW(0, L"bruhClass", L"bruh", 0, 0, 0, 0, 0, 0, 0, hInst, 0);
}

LRESULT CALLBACK WindowProcedure(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) {
    switch (uMsg) {
        case WM_INPUT:
            UINT dwSize;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            if (dwSize > 0) {
                RAWINPUT* raw = (RAWINPUT*)malloc(dwSize);
                if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                    if (raw->header.dwType == RIM_TYPEKEYBOARD && raw->data.keyboard.Message == WM_KEYUP) {
                        switch (raw->data.keyboard.VKey) {
                            case VK_MEDIA_PLAY_PAUSE: {
                                auto s = session_manager.GetCurrentSession();
                                switch (s.GetPlaybackInfo().PlaybackStatus()) {
                                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                                        s.TryPauseAsync();
                                        std::cout << "Pause" << std::endl;
                                        break;
                                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                                        s.TryPlayAsync();
                                        std::cout << "Play" << std::endl;
                                        break;
                                }
                            } break;
                            case VK_MEDIA_NEXT_TRACK: {
                                session_manager.GetCurrentSession().TrySkipNextAsync();
                                std::cout << "Next" << std::endl;
                            } break;
                            case VK_MEDIA_PREV_TRACK: {
                                session_manager.GetCurrentSession().TrySkipPreviousAsync();
                                std::cout << "Prev" << std::endl;
                            } break;
                        }
                    }
                }
                free(raw);
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
};