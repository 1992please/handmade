#include <windows.h>

#define internal static
#define local_persist static 
#define global_variable static 

//	TODO This is global for now.
global_variable bool Running = true;
global_variable BITMAPINFO BitmapInfo;
global_variable void * BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void ResizeDIBSection(int Width, int Height)
{
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}
	
	if(!BitmapDeviceContext)
	{
		// TODO Should we recreate this under certian some special circumstances.
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader); 
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;


	BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, NULL, NULL);
}

internal void UpdateMyWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
	StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(
	HWND   Window,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT Result = 0;

	switch (uMsg)
	{
		case WM_CLOSE:
		{
			// TODO Handle this with a message to the user
			Running = false;
			break;
		}
		case WM_DESTROY:
		{
			// TODO Handle this as an error recreate window
			Running = false;
			break;
		}
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			ResizeDIBSection(Width, Height);
			OutputDebugStringA("WM_SIZE\n");
			break;
		}

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");

			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			UpdateMyWindow(DeviceContext, X, Y, Width, Height);
			EndPaint(Window, &Paint);
			break;
		}
		default:
		{
			Result = DefWindowProc(Window, uMsg, wParam, lParam);
			break;
		}
	}
	return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
	)
{
	WNDCLASS WindowClass = {};

	WindowClass.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = hInstance;
	// WindowClass.hIcon;
	// WindowClass.lpszMenuName;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	RegisterClass(&WindowClass);

	HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

	if (WindowHandle)
	{
		MSG Message;
		while (Running)
		{
			BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
			if (MessageResult > 0)
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		// TODO add error handle here
	}
	return 0;
}