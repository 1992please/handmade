#include <windows.h>

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
			OutputDebugStringA("WM_ClOSE\n");
			break;
		}
		case WM_DESTROY:
		{
			OutputDebugStringA("WM_DESTROY\n");

			break;
		}
		case WM_SIZE:
		{
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
			static DWORD Operation = WHITENESS;

			PatBlt(DeviceContext, X, Y, Width, Height, Operation);
			if (Operation == WHITENESS)
			{
				Operation = BLACKNESS;
			}
			else
			{
				Operation = WHITENESS;
			}
			EndPaint(Window, &Paint);
			OutputDebugStringA("Paint\n");

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
		while (true)
		{
			BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
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