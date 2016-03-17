#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static 
#define global_variable static 

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


//	TODO This is global for now.

struct FOffscreenBuffer
{
	BITMAPINFO Info;
	void * Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};
global_variable bool Running;
global_variable FOffscreenBuffer GlobalBackBuffer;

struct FWindowDimension
{
	int Width;
	int Height;
};

FWindowDimension GetWindowDimension(HWND Window)
{
	FWindowDimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}

internal void RenderWeirdGradient(FOffscreenBuffer Buffer,int XOffset, int YOffset)
{
	uint8 *Row = (uint8*)Buffer.Memory;
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer.Width; ++X)
		{
			uint8 Blue = Y + XOffset;
			uint8 Green = X + YOffset;
			*Pixel++ = ((Green<<8)|(Blue));
		}

		Row += Buffer.Pitch;
	}
}

internal void ResizeDIBSection(FOffscreenBuffer* Buffer,int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, NULL, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader); 
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	// TODO probably clear this to black 
	
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

}

internal void DisplayBufferInWindow(HDC DeviceContext, FOffscreenBuffer Buffer, int WindowWidth, int WindowHeight)
{
	// TODO Aspect ratio correction
	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer.Width, Buffer.Height, Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
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

			FWindowDimension Dimension = GetWindowDimension(Window);

			DisplayBufferInWindow(DeviceContext, GlobalBackBuffer, Dimension.Width, Dimension.Height);
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

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS WindowClass = {};

	ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = hInstance;
	// WindowClass.hIcon;
	// WindowClass.lpszMenuName;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	RegisterClass(&WindowClass);

	HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

	if (WindowHandle)
	{
		Running = true;
		int XOffset = 0;
		int YOffset = 0;
		while (Running)
		{
			MSG Message;
			if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
			{
				if (Message.message == WM_QUIT)
				{
					Running = false;
				}
				TranslateMessage(&Message);
				DispatchMessage(&Message);

			}
			HDC DeviceContext = GetDC(WindowHandle);
			FWindowDimension Dimension = GetWindowDimension(WindowHandle);

			DisplayBufferInWindow(DeviceContext, GlobalBackBuffer, Dimension.Width, Dimension.Height);

			RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

			ReleaseDC(WindowHandle, DeviceContext);
			XOffset++;
			YOffset++;
		}
	}
	else
	{
		// TODO add error handle here
	}
	return 0;
}