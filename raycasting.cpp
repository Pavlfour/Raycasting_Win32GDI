#include <windows.h>
#include <cmath>

// map
const int mapWidth  = 13;
const int mapHeight = 12;

unsigned char worldMap[mapHeight][mapWidth] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,0,0,0,1,1,0,0,0,1},
    {1,0,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,0,1,1,0,0,1,0,1,0,0,1},
    {1,0,0,0,1,0,0,0,0,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,0,1,0,0,1,0,0,0,0,0,1},
    {1,0,0,1,1,1,0,0,1,1,0,0,1},
    {1,0,0,0,0,0,1,1,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// player
float posX{6.f},posY{6.f};
float dirX{-1.f},dirY{0.f};
float planeX{0.f},planeY{0.66f};

// keyboard
bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;

// render
void RenderScene(HDC hdc,RECT& rc)
{
	RECT tempRect = rc;

	// top half
	tempRect.bottom >>= 1;
	HBRUSH ceilingBrush = CreateSolidBrush(RGB(50,150,255));
	FillRect(hdc,&tempRect,ceilingBrush);
	DeleteObject(ceilingBrush);

	// bottom half
	tempRect.top = tempRect.bottom;
	tempRect.bottom <<= 1;
	HBRUSH floorBrush = CreateSolidBrush(RGB(0,155,0));
	FillRect(hdc,&tempRect,floorBrush);
	DeleteObject(floorBrush);

	// walls
	SelectObject(hdc, GetStockObject(DC_PEN));

	for(int w=0; w<tempRect.right; ++w)
	{
		// -1 to 1 values
		float cameraX = 2.f*w/static_cast<float>(tempRect.right) - 1.f;
		float rayDirX = dirX + planeX*cameraX;
		float rayDirY = dirY + planeY*cameraX;

		int mapX = int(posX);
		int mapY = int(posY);

		float sideDistX,sideDistY,perpWallDist;
		char stepX,stepY,hit{0},side{0};

		float deltaDistX = (rayDirX == 0.f) ? 1e30f : std::fabs(1.f / rayDirX);
		float deltaDistY = (rayDirY == 0.f) ? 1e30f : std::fabs(1.f / rayDirY);

		
		if(rayDirX < 0.f)
		{
			stepX = -1;
			sideDistX = (posX - mapX)*deltaDistX;
		}
		else
		{
			stepX = 1;
			sideDistX = (mapX + 1 - posX)*deltaDistX;
		}

		if(rayDirY < 0.f)
		{
			stepY = -1;
			sideDistY = (posY - mapY)*deltaDistY;
		}
		else
		{
			stepY = 1;
			sideDistY = (mapY + 1 - posY)*deltaDistY;
		}

		// DDA
		while(!hit)
		{
			if(sideDistX < sideDistY)
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
			
			// bounds check
			if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
			{
				hit = 1;
				break;
			}

			if(worldMap[mapY][mapX] > 0)
				hit = 1;		
		}
	
		COLORREF color;
		if(!side)
		{
			perpWallDist = sideDistX - deltaDistX;
			color = RGB(100,100,50);
		}
		else
		{
			perpWallDist = sideDistY - deltaDistY;
			color = RGB(155,155,100);
		}

		if(perpWallDist < 0.1f)
			perpWallDist = 0.1f;
		
		int screenHeight = tempRect.bottom;
		int lineHeight = static_cast<int>(screenHeight/perpWallDist);
		
		screenHeight >>= 1;
		lineHeight >>= 1;
		int drawStart = -lineHeight + screenHeight;
		if(drawStart < 0) drawStart = 0;
		
		int drawEnd = lineHeight + screenHeight;
		if(drawEnd >= tempRect.bottom) drawEnd = tempRect.bottom - 1;
		
		SetDCPenColor(hdc,color);
		MoveToEx(hdc,w,drawStart,NULL);
		LineTo(hdc,w,drawEnd);
	}

}

// message handler
LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
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

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd,&ps);
			RECT rc;
			GetClientRect(hwnd,&rc);
			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;
			HDC memDC = CreateCompatibleDC(hdc);
			HBITMAP memBmp = CreateCompatibleBitmap(hdc,w,h);
			HGDIOBJ oldBmp = SelectObject(memDC,memBmp);
			
			// first draw into memory device context
			RenderScene(memDC,rc);

			// blit to the real window
			BitBlt(hdc,0,0,w,h,memDC,0,0,SRCCOPY);
			
			// clean up
			SelectObject(memDC,oldBmp);
			DeleteObject(memBmp);
			DeleteDC(memDC);
			
			EndPaint(hwnd,&ps);
			return 0;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
			
	}

	return DefWindowProc(hwnd,msg,wParam,lParam);
}

// main function
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int)
{
	WNDCLASS wc = {};
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInst;
	wc.lpszClassName = "test";
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0,"test","Nothing Special",WS_OVERLAPPEDWINDOW | WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,800,600,NULL,NULL,hInst,NULL);
	if(!hwnd) return -1;
	
	MSG msg = {};
	bool running{true};
	//movement and rotation speed
	float moveSpeed,rotSpeed;
	
	// time setup
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	LARGE_INTEGER lastTime;
	QueryPerformanceCounter(&lastTime);	

	while(running)
	{
		// timestep
		LARGE_INTEGER current;
		QueryPerformanceCounter(&current);
		// seconds
		double elapsedTime = static_cast<double>(current.QuadPart - lastTime.QuadPart) / static_cast<double>(freq.QuadPart);
		lastTime = current;

		moveSpeed = rotSpeed = 2.0*elapsedTime;
		
		// handle windows messages
		while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
				running = false;
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		// move
		if(keyW)
		{
			float newX = posX + dirX*moveSpeed;
			float newY = posY + dirY*moveSpeed;
			if(!worldMap[(unsigned char)newY][(unsigned char)posX]) posY=newY;
			if(!worldMap[(unsigned char)posY][(unsigned char)newX]) posX=newX;
		}
		else if(keyS)
		{
			float newX = posX - dirX*moveSpeed;
			float newY = posY - dirY*moveSpeed;
			if(!worldMap[(unsigned char)newY][(unsigned char)posX]) posY=newY;
			if(!worldMap[(unsigned char)posY][(unsigned char)newX]) posX=newX;
		}
		
		// turn
		if(keyA)
		{
			float oldDirX = dirX;
			dirX = dirX * std::cos(rotSpeed) - dirY * std::sin(rotSpeed);
			dirY = oldDirX * std::sin(rotSpeed) + dirY * std::cos(rotSpeed);

            		float oldPlaneX = planeX;
			planeX = planeX * std::cos(rotSpeed) - planeY * std::sin(rotSpeed);
			planeY = oldPlaneX * std::sin(rotSpeed) + planeY * std::cos(rotSpeed);		
		}
		else if(keyD)
		{
			float oldDirX = dirX;
			dirX = dirX * std::cos(-rotSpeed) - dirY * std::sin(-rotSpeed);
			dirY = oldDirX * std::sin(-rotSpeed) + dirY * std::cos(-rotSpeed);

            		float oldPlaneX = planeX;
			planeX = planeX * std::cos(-rotSpeed) - planeY * std::sin(-rotSpeed);
			planeY = oldPlaneX * std::sin(-rotSpeed) + planeY * std::cos(-rotSpeed);		
		}

		// redraw
		InvalidateRect(hwnd,NULL,FALSE);
	}

	return 0;
}