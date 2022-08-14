#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <math.h>
#include <time.h>

#define GLOBAL_MUTEX_STRING "watermark_process_mutex"
#define BACKGROUND_COLOR 0xff000000
#define DEFAULT_TRANSPARENCY 2  // 0-255
#define DEFAULT_ROTATE_ANGLE 15 // 0-90
#define DEFAULT_FONT_SIZE 20
#define DEFAULT_FONT_COLOR 0x00ffffff
#define INIT_ONE_MARK_WIDTH 320
#define ONE_LINE_HEIGHT 21 // text height

#define MATH_PI 3.1415926 //π
#define MAX_BUFFER_LEN 1024
#define AUTO_REFRESH_INTERVAL 1000 * 30 // 30s
#define SET_TOP_INTERVAL 1000           // 1s
#define TIMER_1 1
#define TIMER_2 2

typedef struct Geometry
{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    std::string print()
    {
        std::stringstream s;
        s << "[" << x << "," << y << "," << w << "," << h << "]";
        return s.str();
    }
} Geometry;

typedef struct WaterMarkConfig
{
    bool isInvisible;
    Geometry wndGeometry;
    uint32_t fontColor; // 0x0 ~ 0xffffffff
    uint32_t fontSize;
    uint32_t transparency; // 0~255
    uint32_t angle;        // 0~90
    bool showComputerName;
    bool showLocalTime;
    std::string customText;
} WaterMarkConfig;

Geometry GetDesktopGeometry()
{
    int x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return {x, y, w, h};
}

WaterMarkConfig g_waterMarkconf = {false, GetDesktopGeometry(), DEFAULT_FONT_COLOR, DEFAULT_FONT_SIZE, DEFAULT_TRANSPARENCY, DEFAULT_ROTATE_ANGLE, true, true, "This is a watermark."};

std::string GetComputerNameStr()
{
    DWORD bufLen = MAX_BUFFER_LEN;
    CHAR buf[MAX_BUFFER_LEN] = {0};
    if (!GetComputerNameA(buf, &bufLen))
    {
        std::cout << "get computer name failed,err code:" << GetLastError() << std::endl;
        return std::string("unknown");
    }
    return std::string(buf);
}

std::string GetLocaltimeStr()
{
    time_t tt = time(NULL);
    tm *ptm = localtime(&tt);
    char buf[128] = {0};
    sprintf(buf, "%d-%d-%d %d:%02d:%02d",
            (int)ptm->tm_year + 1900,
            (int)ptm->tm_mon + 1,
            (int)ptm->tm_mday,
            (int)ptm->tm_hour,
            (int)ptm->tm_min,
            (int)ptm->tm_sec);
    return std::string(buf);
}

COLORREF argb2abgr(const int &argb)
{
    return (((argb & 0x000000ff) << 16) | (argb & 0x0000ff00) | ((argb & 0x00ff0000) >> 16));
}

void CreateOffScreenSingleWatermark(HDC hdc, WaterMarkConfig &conf)
{
    double alpha = (double)conf.angle / 180 * MATH_PI;
    int left = 0;
    int top = (int)(INIT_ONE_MARK_WIDTH * sin(alpha));
    int xStep = (int)(conf.fontSize * sin(alpha));
    int yStep = (int)(conf.fontSize * cos(alpha)) + 1;

    if (conf.showComputerName)
    {
        std::string computerName = GetComputerNameStr();
        if (!TextOutA(hdc, left, top, computerName.c_str(), computerName.size()))
        {
            std::cout << "draw computer name failed,err:" << GetLastError() << std::endl;
        }
        left += xStep;
        top += yStep;
    }
    if (conf.showLocalTime)
    {
        std::string localTime = GetLocaltimeStr();
        if (!TextOutA(hdc, left, top, localTime.c_str(), localTime.size()))
        {
            std::cout << "draw time failed,err:" << GetLastError() << std::endl;
        }
        left += xStep;
        top += yStep;
    }
    if (!conf.customText.empty())
    {
        std::string &tmp = conf.customText;
        int lineChars = INIT_ONE_MARK_WIDTH / conf.fontSize; // coculate one line character number.

        for (int i = 0; i < conf.customText.size();)
        {
            std::string line;
            if ((i + lineChars) < conf.customText.size())
            {
                line = std::string(tmp.begin() + i, tmp.begin() + i + lineChars);
                i += lineChars;
            }
            else
            {
                line = std::string(tmp.begin() + i, tmp.end());
                i = conf.customText.size();
            }
            if (!TextOutA(hdc, left, top, line.c_str(), line.size()))
            {
                std::cout << "draw user define string failed,err:" << GetLastError() << std::endl;
            }
            left += xStep;
            top += yStep;
        }
    }
}

void DrawWatermark(HDC hdc, WaterMarkConfig &conf)
{
    if (conf.isInvisible)
    {
        conf.transparency = DEFAULT_TRANSPARENCY;
        conf.fontColor = DEFAULT_FONT_COLOR;
    }
    int textHeight = 0;
    if (conf.showComputerName)
    {
        textHeight += ONE_LINE_HEIGHT;
    }
    if (conf.showLocalTime)
    {
        textHeight += ONE_LINE_HEIGHT;
    }
    if (!conf.customText.empty())
    {
        int lineChars = INIT_ONE_MARK_WIDTH / conf.fontSize;
        textHeight += ONE_LINE_HEIGHT * (conf.customText.size() / lineChars + 1);
    }

    // caculate real height of one mark.
    double alpha = (double)conf.angle / 180.0 * MATH_PI;
    int oneWidth = INIT_ONE_MARK_WIDTH * cos(alpha) + textHeight * sin(alpha) + conf.fontSize; // Add one more character width.
    int oneHeight = INIT_ONE_MARK_WIDTH * sin(alpha) + textHeight * cos(alpha) + conf.fontSize;

    static bool enablePrint = true;
    if (enablePrint)
    {
        std::cout << "Mark info:"
                  << "\n    isInvisible:" << conf.isInvisible
                  << "\n    window geometry:" << conf.wndGeometry.print()
                  << "\n    font color:" << std::hex << conf.fontColor << std::dec
                  << "\n    font size:" << conf.fontSize
                  << "\n    traqnsparency:" << conf.transparency
                  << "\n    angle:" << conf.angle
                  << "\n    show computer name:" << conf.showComputerName << (conf.showComputerName ? "(" + GetComputerNameStr() + ")" : "")
                  << "\n    show localtime:" << conf.showLocalTime << (conf.showLocalTime ? "(" + GetLocaltimeStr() + ")" : "")
                  << "\n    custom text:" << conf.customText
                  << "\n    off-screen single watermark:[" << oneWidth << "," << oneHeight << "]"
                  << std::endl;
        enablePrint = false;
    }

    HFONT hfont = CreateFontA(
        conf.fontSize,
        conf.fontSize / 2,
        conf.angle * 10,
        0,
        FW_HEAVY,
        0,
        0,
        0,
        GB2312_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_MODERN,
        "font");

    HDC offscreenDc = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, oneWidth, oneHeight);

    SelectObject(offscreenDc, hBitmap);
    SelectObject(offscreenDc, hfont);
    SetTextColor(offscreenDc, argb2abgr(conf.fontColor));
    SetBkColor(offscreenDc, BACKGROUND_COLOR);

    CreateOffScreenSingleWatermark(offscreenDc, conf);
    for (int i = conf.wndGeometry.y; i < conf.wndGeometry.h; i += oneHeight)
    {
        for (int j = conf.wndGeometry.x; j < conf.wndGeometry.w; j += oneWidth)
        {
            BitBlt(hdc, j, i, oneWidth, oneHeight, offscreenDc, 0, 0, SRCCOPY);
        }
    }

    DeleteObject(hfont);
    DeleteObject(hBitmap);
    DeleteObject(offscreenDc);
    offscreenDc = NULL;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        SetWindowLongW(hwnd, GWL_EXSTYLE, (GetWindowLongW(hwnd, GWL_EXSTYLE)) & ~WS_EX_APPWINDOW | WS_EX_TOOLWINDOW);
        SetTimer(hwnd, TIMER_1, AUTO_REFRESH_INTERVAL, NULL);
        SetTimer(hwnd, TIMER_2, SET_TOP_INTERVAL, NULL);
        std::cout << "create blind water mark window success." << std::endl;
        return 0;
    }
    case WM_TIMER:
        switch (wParam)
        {
        case TIMER_1:
        {
            int x = 0, y = 0, w = 0, h = 0;
            Geometry g = GetDesktopGeometry();
            MoveWindow(hwnd, g.x, g.y, g.w, g.h, true);
            InvalidateRgn(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        }
        case TIMER_2:
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            break;
        }
        return 0;
    case WM_PAINT:
    {
        DWORD time = GetTickCount();
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawWatermark(hdc, g_waterMarkconf);
        // set window transparency
        // DWORD flag = g_waterMarkconf.isInvisible ? (LWA_ALPHA) : (LWA_COLORKEY | LWA_ALPHA);
        DWORD flag = LWA_ALPHA;
        SetLayeredWindowAttributes(hwnd, BACKGROUND_COLOR, g_waterMarkconf.transparency, flag);
        // Remove borders and title blocks
        SetWindowLongW(hwnd, GWL_STYLE, GetWindowLongW(hwnd, GWL_STYLE) & ~WS_CAPTION);
        // Set the key-rat event penetration
        SetWindowLongW(hwnd, GWL_EXSTYLE, (GetWindowLongW(hwnd, GWL_EXSTYLE)) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
        // TOPMOST
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        std::cout << "WM_PAINT use :" << GetTickCount() - time << " ms" << std::endl;
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int iCmdShow)
{
    // Ensure global uniqueness
    HANDLE g_hGlobalMutex = CreateMutexA(NULL, FALSE, GLOBAL_MUTEX_STRING);
    if (g_hGlobalMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(g_hGlobalMutex);
        g_hGlobalMutex = NULL;
        return -1;
    }

    WNDCLASSA winClass = {
        CS_SAVEBITS,
        WndProc,
        0,
        0,
        hThisInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        CreateSolidBrush(BACKGROUND_COLOR),
        NULL,
        "BlindMarkDilog"};

    if (!RegisterClassA(&winClass))
    {
        std::cout << "register window failed,BlindMarkDilog,err:" << GetLastError() << std::endl;
        return 0;
    }

    Geometry g = GetDesktopGeometry();

    std::cout << "desktop grometry:" << g.print() << std::endl;
    HWND hwnd = CreateWindowW(L"BlindMarkDilog", L"BlindMarkWnd", WS_OVERLAPPED,
                              g.x, g.y, g.w, g.h,
                              NULL, NULL, hThisInstance, NULL);
    if (hwnd == NULL)
    {
        std::cout << "create window failed,err:" << GetLastError();
        return 0;
    }

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    std::cout << "blind water mark quit" << std::endl;
    return msg.wParam;
}