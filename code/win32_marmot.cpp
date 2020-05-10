#include <windows.h>
#define internal static
#define local_persist static
#define global static

typedef char u8;
typedef unsigned int u32;

const u32 keyRectCount = 4;
const u32 keyRectWidth = 20;
const u32 keyRectHeight = 20;
const u32 keyRectToggledHeight = 40;
const u32 winColor = 0xffffff;
const u32 ctrlColor = 0xff0000;
const u32 shiftColor = 0x00ff00;
const u32 altColor = 0x0000ff;

struct Win32OffscreenBuffer {
    // NOTE(Marchin): pixels are little endinan, 32-bit wide (RGB + padding)
    BITMAPINFO info;
    void* pMemory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};

struct Win32WindowDimension {
    int width;
    int height;
};

global Win32OffscreenBuffer gBackBuffer;

inline Win32WindowDimension
getWindowDimension(HWND window) {
    Win32WindowDimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

inline void
win32ResizeDIBSection(Win32OffscreenBuffer* pBackBuffer,
                      int width, int height) {
    if (pBackBuffer->pMemory) {
        VirtualFree(pBackBuffer->pMemory, 0, MEM_RELEASE);
    }
    pBackBuffer->width = width;
    pBackBuffer->height = height;
    pBackBuffer->bytesPerPixel = 4;
    pBackBuffer->info.bmiHeader.biSize = sizeof(pBackBuffer->info.bmiHeader);
    pBackBuffer->info.bmiHeader.biWidth = pBackBuffer->width;
    // NOTE(Marchin): biHeight is negative to tell windows that y increases
    //when going down, y = 0 -> top of imageBuffer
    pBackBuffer->info.bmiHeader.biHeight = -pBackBuffer->height;
    pBackBuffer->info.bmiHeader.biPlanes = 1;
    pBackBuffer->info.bmiHeader.biBitCount = 32;
    pBackBuffer->info.bmiHeader.biCompression = BI_RGB;
    int bitmapMemorySize = (pBackBuffer->width * pBackBuffer->height) * pBackBuffer->bytesPerPixel;
    pBackBuffer->pMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT,PAGE_READWRITE);
    pBackBuffer->pitch = width * pBackBuffer->bytesPerPixel;
}

inline void
win32DisplayBufferInWindow(Win32OffscreenBuffer* pBackBuffer,
                           HDC deviceContext,
                           int windowWidth, int windowHeight) {
    if ((windowWidth >= pBackBuffer->width*2) &&
        (windowHeight >= pBackBuffer->height*2)) {
        
        StretchDIBits(deviceContext,
                      0, 0, 2*pBackBuffer->width, 2*pBackBuffer->height,
                      0, 0, pBackBuffer->width, pBackBuffer->height,
                      pBackBuffer->pMemory, &pBackBuffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    } else {
        PatBlt(deviceContext, 0, 0, windowWidth, 0, BLACKNESS);
        PatBlt(deviceContext, 0, pBackBuffer->height,
               windowWidth, windowHeight, BLACKNESS);
        PatBlt(deviceContext, 0, 0, 0, windowHeight, BLACKNESS);
        PatBlt(deviceContext, pBackBuffer->width, 0,
               windowWidth, windowHeight, BLACKNESS);
        
        StretchDIBits(deviceContext,
                      0, 0, pBackBuffer->width, pBackBuffer->height,
                      0, 0, pBackBuffer->width, pBackBuffer->height,
                      pBackBuffer->pMemory, &pBackBuffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
}

inline void
win32ProcessPendingMessages() {
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)){
        switch(message.message) {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_QUIT: {
            } break;
            default: {
                DispatchMessageA(&message);
                TranslateMessage(&message);
            } break;
        }
    }
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND   window,
                        UINT   message,
                        WPARAM wParam,
                        LPARAM lParam) {
    LRESULT result = 0;
    switch(message) {
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            Win32WindowDimension dimension = getWindowDimension(window);
            win32DisplayBufferInWindow(&gBackBuffer, deviceContext, dimension.width,
                                       dimension.height);
            EndPaint(window, &paint);
        } break;
        case WM_NCCALCSIZE:
        case WM_SIZE:
        case WM_DESTROY:
        case WM_CLOSE:
        case WM_ACTIVATEAPP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SETCURSOR:{
        } break;
        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }
    return result;
}

inline void
win32DebugDrawRect(Win32OffscreenBuffer* pBackBuffer,
                   int left, int top,
                   int width, int height,
                   u32 color) {
    if (left <= 0 ) { left = 0; }
    if (top <= 0 ) { top = 0; }
    if (left + width > pBackBuffer->width) { width = pBackBuffer->width - left; }
    if (top + height > pBackBuffer->height) { height = pBackBuffer->height - top; }
    
    int right = left + width;
    int bottom = top + height;
    for (int y = top; y < bottom; ++y) {
        u8* pPixel = (u8*)pBackBuffer->pMemory +
            left*pBackBuffer->bytesPerPixel + y*pBackBuffer->pitch;
        for (int x = left; x < right; ++x) {
            *(u32*)pPixel = color;
            pPixel += pBackBuffer->bytesPerPixel;
        }
        pPixel += pBackBuffer->pitch;
    }
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prevInstance,
        LPSTR pCmdLine,
        int cmdShow) {
    int totalWidth = keyRectWidth*keyRectCount;
    
    WNDCLASSA windowClass = {};
    win32ResizeDIBSection(&gBackBuffer, totalWidth, keyRectToggledHeight);
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.lpszClassName = "MarmotEngineWindowClass";
    if (RegisterClassA(&windowClass)){
        HWND window = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
                                      windowClass.lpszClassName,
                                      "Marmot Engine",
                                      WS_VISIBLE,
                                      0, 0,
                                      totalWidth, keyRectToggledHeight,
                                      0,
                                      0,
                                      instance,
                                      0);
        if (window) {
            HDC refreshDC = GetDC(window);
            int monitorRefreshRate = 60;
            int win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
            if (win32RefreshRate > 1) {
                monitorRefreshRate = win32RefreshRate;
            }
            float gameUpdateHz = (monitorRefreshRate / 2.f);
            float targetSecondsPerFrame = 1.f / gameUpdateHz;
            ReleaseDC(window, refreshDC);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(MonitorFromWindow(window,
                                             MONITOR_DEFAULTTOPRIMARY), &mi);
            
            SetWindowPos(window, HWND_TOP,
                         mi.rcMonitor.right - mi.rcMonitor.left - totalWidth,
                         0,
                         totalWidth,
                         keyRectToggledHeight,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            SetLayeredWindowAttributes(window, RGB(0, 0, 0), 0, LWA_COLORKEY);
            
            
            while (true) {
                win32ProcessPendingMessages();
                
                STICKYKEYS stickyInfo = { sizeof(STICKYKEYS), 0 };
                SystemParametersInfoA(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &stickyInfo, 0);
                
                win32DebugDrawRect(&gBackBuffer, 0, 0, totalWidth, keyRectToggledHeight, 0);
                if (stickyInfo.dwFlags & SKF_STICKYKEYSON) {
                    int xOffset = 0;
                    if (stickyInfo.dwFlags & (SKF_LSHIFTLOCKED | SKF_RSHIFTLOCKED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectToggledHeight,
                                           shiftColor);
                    } else if (stickyInfo.dwFlags & (SKF_LSHIFTLATCHED | SKF_RSHIFTLATCHED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectHeight,
                                           shiftColor);
                    }
                    
                    xOffset += keyRectWidth;
                    
                    if (stickyInfo.dwFlags & (SKF_LCTLLOCKED | SKF_RCTLLOCKED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectToggledHeight,
                                           ctrlColor);
                    } else if (stickyInfo.dwFlags & (SKF_LCTLLATCHED | SKF_RCTLLATCHED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectHeight,
                                           ctrlColor);
                    }
                    
                    xOffset += keyRectWidth;
                    
                    if (stickyInfo.dwFlags & (SKF_LWINLOCKED | SKF_RWINLOCKED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectToggledHeight,
                                           winColor);
                    } else if (stickyInfo.dwFlags & (SKF_LWINLATCHED | SKF_RWINLATCHED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectHeight,
                                           winColor);
                    }
                    
                    xOffset += keyRectWidth;
                    
                    if (stickyInfo.dwFlags & (SKF_LALTLOCKED | SKF_RALTLOCKED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectToggledHeight,
                                           altColor);
                    } else if (stickyInfo.dwFlags & (SKF_LALTLATCHED | SKF_RALTLATCHED)) {
                        win32DebugDrawRect(&gBackBuffer,
                                           xOffset, 0,
                                           keyRectWidth, keyRectHeight,
                                           altColor);
                    }
                }
                
                Sleep(100);
                
                Win32WindowDimension dimension = getWindowDimension(window);
                HDC deviceContext = GetDC(window);
                win32DisplayBufferInWindow(&gBackBuffer, deviceContext,
                                           dimension.width, dimension.height);
                ReleaseDC(window, deviceContext);
            }
        }
    }
    return 0;
}