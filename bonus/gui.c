#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

typedef unsigned char u8;
typedef unsigned int u32;

#define W 1024
#define H 768
#define make_color(r, g, b, a) ((Color)((a << 24) | (r << 16) | (g << 8) | b))
#include "drawing.h"


void poll_input();

#include "simulate.h"

void poll_input()
{
    // g.input.start = GetAsyncKeyState(VK_SPACE);
    // g.input.up    = GetAsyncKeyState(VK_UP);
    // g.input.down  = GetAsyncKeyState(VK_DOWN);
    // g.input.left  = GetAsyncKeyState(VK_LEFT);
    // g.input.right = GetAsyncKeyState(VK_RIGHT);
    // g.input.fire = GetAsyncKeyState('F');
}


bool running = true;
BITMAPINFO bitmap_info;
Color *pixels;
int bitmap_width;
int bitmap_height;
int bytes_per_pixel = 4;

// Framebuffer data (replace this with your actual framebuffer data)
BYTE* framebuffer = NULL;
int framebufferWidth = 0;
int framebufferHeight = 0;

void create_frame_buffer(int width, int height)
{
    if(pixels)
    {
        VirtualFree(pixels, 0, MEM_RELEASE);
    }

    bitmap_width = width;
    bitmap_height = height;

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    int pixels_size = (width*height)*bytes_per_pixel;
    pixels = VirtualAlloc(0, pixels_size, MEM_COMMIT, PAGE_READWRITE);
}

void present_window(HDC context, RECT *client_rect, int x, int y, int width, int height)
{
    int window_width = client_rect->right - client_rect->left;
    int window_height = client_rect->bottom - client_rect->top;
    StretchDIBits
    (
        context,
        0, 0, window_width, window_height,
        0, 0, bitmap_width, bitmap_height,
        pixels,
        &bitmap_info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT WindowCallback(HWND window, UINT message, WPARAM w, LPARAM l)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_MOUSEMOVE:
        {    result = DefWindowProc(window, message, w, l);    } break;
        case WM_CLOSE:
        case WM_DESTROY:
        {    running = false;    } break;
        case WM_ACTIVATEAPP:
        {        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom-paint.rcPaint.top;
            int width = paint.rcPaint.right-paint.rcPaint.left;
            RECT client_rect;
            GetClientRect(window, &client_rect);
            present_window(device_context, &client_rect, x, y, width, height);
            EndPaint(window, &paint);
        } break;
        default:
        {    result = DefWindowProc(window, message, w, l);    } break;
    }
    return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show)
{
    WNDCLASS WindowClass = {0};
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = WindowCallback;
    WindowClass.hInstance = instance;
    WindowClass.lpszClassName = "StronkWindowClass";
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    //hIcon;
    WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClass(&WindowClass);

    RECT rect = {0, 0, W, H};

    HWND window = CreateWindowEx
    (
        0,
        WindowClass.lpszClassName,
        "Traffic",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right,
        rect.bottom,
        0,
        0,
        instance,
        0
    );

    if(window)
    {
        RECT client_rect;
        GetClientRect(window, &client_rect);
        int window_width = client_rect.right - client_rect.left;
        int window_height = client_rect.bottom - client_rect.top;
        int width_diff = W-window_width;
        int height_diff = H-window_height;
        SetWindowPos(window, 0, 0, 0, W+width_diff, H+height_diff, SWP_FRAMECHANGED | SWP_NOMOVE );
        HDC context = GetDC(window);
        //SetStretchBltMode(context, HALFTONE);

        create_frame_buffer(W, H);        

        draw_target = (Texture) {
            .width = W,
            .height = H,
            .pixels = pixels,
        };

        init();
        while(running)
        {
            MSG message;
            if(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
            {
                if(message.message == WM_QUIT)
                    running = false;
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

            if(GetAsyncKeyState(VK_ESCAPE))
                running = false;

            simulate();
            GetClientRect(window, &client_rect);
            window_width = client_rect.right - client_rect.left;
            window_height = client_rect.bottom - client_rect.top;            
            present_window(context, &client_rect, 0,0, window_width, window_height);
        }
    }

    return 0;
}