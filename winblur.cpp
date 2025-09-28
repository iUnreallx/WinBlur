#include <windows.h>
#include <windowsx.h>


enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};


struct ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};


struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    void* pvData;
    size_t cbData;
};


typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};


void EnableBlur(HWND hwnd) {
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        auto setWindowCompositionAttribute =
            reinterpret_cast<pfnSetWindowCompositionAttribute>(
                GetProcAddress(hUser, "SetWindowCompositionAttribute")
                );
        if (setWindowCompositionAttribute) {
            ACCENT_POLICY accent = {};
            accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            accent.GradientColor = 0x99000000;
            WINDOWCOMPOSITIONATTRIBDATA data = {};
            data.Attrib = WCA_ACCENT_POLICY;
            data.pvData = &accent;
            data.cbData = sizeof(accent);
            setWindowCompositionAttribute(hwnd, &data);
        }
    }
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static bool isFullScreen = false;
    static bool isTopmost = true; 
    static RECT normalRect;
    switch (msg) {
    case WM_CREATE:
        EnableBlur(hwnd);
        return 0;
    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_NCRBUTTONUP:
        if (wParam == HTCAPTION) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            SendMessage(hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELPARAM(pt.x, pt.y));
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    case WM_CONTEXTMENU: {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu) {
            AppendMenuW(hMenu, MF_STRING, 2, isFullScreen ? L"Восстановить" : L"Во весь экран");
            AppendMenuW(hMenu, MF_STRING, 3, isTopmost ? L"Обычная иерархия" : L"Поверх всех окон");
            AppendMenuW(hMenu, MF_STRING, 1, L"Закрыть");
            POINT pt;
            if (lParam == (LPARAM)-1) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                pt.x = rc.right / 2;
                pt.y = rc.bottom / 2;
                ClientToScreen(hwnd, &pt);
            } else {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            }
            SetForegroundWindow(hwnd);
            UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                                      pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            if (cmd == 1) {
                DestroyWindow(hwnd);
            } else if (cmd == 2) {
                if (!isFullScreen) {
                    GetWindowRect(hwnd, &normalRect);
                    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO mi = { sizeof(mi) };
                    GetMonitorInfo(hmon, &mi);
                    MoveWindow(hwnd, mi.rcMonitor.left, mi.rcMonitor.top,
                               mi.rcMonitor.right - mi.rcMonitor.left,
                               mi.rcMonitor.bottom - mi.rcMonitor.top, TRUE);
                    isFullScreen = true;
                } else {
                    MoveWindow(hwnd, normalRect.left, normalRect.top,
                               normalRect.right - normalRect.left,
                               normalRect.bottom - normalRect.top, TRUE);
                    isFullScreen = false;
                }
            } else if (cmd == 3) {
                isTopmost = !isTopmost;
                SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                                 isTopmost ? (GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_TOPMOST)
                                           : (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_TOPMOST));
                SetWindowPos(hwnd, isTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                             0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
        return 0;
    }
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc;
            GetClientRect(hwnd, &rc);
            const int border = 8;
            if (pt.x < border && pt.y < border) return HTTOPLEFT;
            if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
            if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;
            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;
            if (pt.y < border) return HTTOP;
            if (pt.y > rc.bottom - border) return HTBOTTOM;
            return HTCAPTION;
        }
        return hit;
    }
    case WM_SETCURSOR: {
        WORD hitTest = LOWORD(lParam);
        if (hitTest == HTCAPTION || hitTest == HTCLIENT) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


int WINAPI MyWinMain(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"WinBlurClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClassW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 250;
    int windowHeight = 250;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME,
        L"WinBlur",
        WS_POPUP | WS_VISIBLE,
        x, y, windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr
        );
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}


int main(int argc, char* argv[]) {
    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    return MyWinMain(hInstance, SW_SHOWDEFAULT);
}
