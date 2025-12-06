#include <windows.h>
#include <cmath>
#include <cstdio>

// map
const int mapWidth  = 12;
const int mapHeight = 12;

unsigned char worldMap[mapWidth][mapHeight] = {
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,1},
    {1,0,1,1,0,0,0,1,1,0,0,1},
    {1,0,0,0,0,1,0,0,0,0,1,1},
    {1,0,0,1,1,0,0,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,0,0,0,1,0,0,0,0,1,1},
    {1,0,0,1,0,0,1,0,0,0,0,1},
    {1,0,0,1,1,1,0,0,1,1,0,1},
    {1,0,0,0,0,0,1,1,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1}
};

// player
// starting position
double posX = 6.0, posY = 6.0;
// initial direction
double dirX = -1.0, dirY = 0.0;
// camera plane perpendicular to direction
double planeX = 0.0, planeY = 0.66;

// keyboard
bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;

// render
void RenderScene(HDC hdc, RECT& rc)
{
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // Ceiling (top half)
    RECT ceiling = rc;
    ceiling.bottom = h / 2;
    HBRUSH ceilingBrush = CreateSolidBrush(RGB(50, 150, 255));
    FillRect(hdc, &ceiling, ceilingBrush);
    DeleteObject(ceilingBrush);

    // Floor (bottom half)
    RECT floorR = rc;
    floorR.top = h / 2;
    HBRUSH floorBrush = CreateSolidBrush(RGB(0, 155, 0));
    FillRect(hdc, &floorR, floorBrush);
    DeleteObject(floorBrush);

    // Wall pen
    HPEN wallPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 50));
    HPEN oldPen  = (HPEN)SelectObject(hdc, wallPen);

    for (int x = 0; x < w; ++x)
    {
        double cameraX = 2.0 * x / double(w) - 1.0; // -1 to 1
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirY + planeY * cameraX;

        int mapX = int(posX);
        int mapY = int(posY);

        double sideDistX, sideDistY;

        double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1.0 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1.0 / rayDirY);
        double perpWallDist;

        int stepX, stepY;
        int hit  = 0;
        int side = 0; // 0 = x-side, 1 = y-side

        if (rayDirX < 0)
        {
            stepX    = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        }
        else
        {
            stepX    = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }

        if (rayDirY < 0)
        {
            stepY    = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        }
        else
        {
            stepY    = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        // DDA
        while (hit == 0)
        {
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            // Bounds check
            if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
            {
                hit = 1;
                break;
            }

            if (worldMap[mapX][mapY] > 0) hit = 1;
        }

        if (side == 0)
            perpWallDist = sideDistX - deltaDistX;
        else
            perpWallDist = sideDistY - deltaDistY;

        if (perpWallDist <= 0.0)
            perpWallDist = 0.1;

        int lineHeight = int(h / perpWallDist);

        int drawStart = -lineHeight / 2 + h / 2;
        if (drawStart < 0) drawStart = 0;

        int drawEnd = lineHeight / 2 + h / 2;
        if (drawEnd >= h) drawEnd = h - 1;

        COLORREF color = (side == 1) ? RGB(155, 155, 100) : RGB(100, 100, 50);
        HPEN colPen = CreatePen(PS_SOLID, 1, color);
        SelectObject(hdc, colPen);

        MoveToEx(hdc, x, drawStart, NULL);
        LineTo(hdc, x, drawEnd);

        SelectObject(hdc, wallPen);
        DeleteObject(colPen);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(wallPen);
}

// message handler
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == 'W') keyW = true;
        if (wParam == 'S') keyS = true;
        if (wParam == 'A') keyA = true;
        if (wParam == 'D') keyD = true;
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;

    case WM_KEYUP:
        if (wParam == 'W') keyW = false;
        if (wParam == 'S') keyS = false;
        if (wParam == 'A') keyA = false;
        if (wParam == 'D') keyD = false;
        return 0;

    case WM_ERASEBKGND:
        // Stop Windows from clearing the background; we draw everything
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        //DOUBLE BUFFERING START, draw entire frame off-screen to avoid flickering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        // Draw entire frame into memory DC
        RenderScene(memDC, rc);

        // Blit to the real window in one go
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        //DOUBLE BUFFERING END

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// main function
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    // Register window class
    WNDCLASS wc = {};
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "RaycastClass";
    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        "RaycastClass",
        "Basic Raycasting (W/S move, A/D turn, ESC quit)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInst, NULL
    );

    if (!hwnd) return -1;

    // Timing setup
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    LARGE_INTEGER lastTime;
    QueryPerformanceCounter(&lastTime);

    MSG msg = {};
    bool running = true;

    while (running)
    {
        // Time step
        LARGE_INTEGER current;
        QueryPerformanceCounter(&current);
        double frameTime =
            (double)(current.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        lastTime = current;

        double moveSpeed = frameTime * 2.0; // units per second
        double rotSpeed  = frameTime * 2.0; // radians per second

        // Handle Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // control
        // Forward
        if (keyW)
        {
            double newX = posX + dirX * moveSpeed;
            double newY = posY + dirY * moveSpeed;
            if (worldMap[int(newX)][int(posY)] == 0) posX = newX;
            if (worldMap[int(posX)][int(newY)] == 0) posY = newY;
        }

        // Backward
        if (keyS)
        {
            double newX = posX - dirX * moveSpeed;
            double newY = posY - dirY * moveSpeed;
            if (worldMap[int(newX)][int(posY)] == 0) posX = newX;
            if (worldMap[int(posX)][int(newY)] == 0) posY = newY;
        }

        // Turn right (D)
        if (keyD)
        {
            double oldDirX = dirX;
            dirX = dirX * std::cos(-rotSpeed) - dirY * std::sin(-rotSpeed);
            dirY = oldDirX * std::sin(-rotSpeed) + dirY * std::cos(-rotSpeed);

            double oldPlaneX = planeX;
            planeX = planeX * std::cos(-rotSpeed) - planeY * std::sin(-rotSpeed);
            planeY = oldPlaneX * std::sin(-rotSpeed) + planeY * std::cos(-rotSpeed);
        }

        // Turn left (A)
        if (keyA)
        {
            double oldDirX = dirX;
            dirX = dirX * std::cos(rotSpeed) - dirY * std::sin(rotSpeed);
            dirY = oldDirX * std::sin(rotSpeed) + dirY * std::cos(rotSpeed);

            double oldPlaneX = planeX;
            planeX = planeX * std::cos(rotSpeed) - planeY * std::sin(rotSpeed);
            planeY = oldPlaneX * std::sin(rotSpeed) + planeY * std::cos(rotSpeed);
        }

        // Redraw
        InvalidateRect(hwnd, NULL, FALSE);

        // Optional: reduce CPU usage a bit
        // Sleep(1);
    }

    return 0;
}
