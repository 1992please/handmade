#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

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

struct FWindowDimension
{
	int Width;
	int Height;
};

// this is global for now
global_variable bool GlobalRunning;
global_variable FOffscreenBuffer GlobalBackBuffer;

// This is our support for XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// This is our support for XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);



internal void LoadXInput()
{
	// TODO test this on windows 10
	HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibrary("xinput1_4.dll");
		// TODO : Diagnostic
	}

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		// TODO Diagnostic
	}
}

internal FWindowDimension GetWindowDimension(HWND Window)
{
	FWindowDimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}

internal void InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	// Load the sound library.
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

	if (DSoundLibrary)
	{
		// Get DirectSound object!
		direct_sound_create *DirectSoundCreate = (direct_sound_create* )GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && DirectSoundCreate(0, &DirectSound, 0) == DS_OK)
		{
			WAVEFORMATEX WaveFormat;
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = WaveFormat.nChannels * WaveFormat.wBitsPerSample / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * WaveFormat.nSamplesPerSec;
			WaveFormat.cbSize = 0;

			if (DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY) == DS_OK)
			{
				// Create a primary buffer
				DSBUFFERDESC  BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				
				if (DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0) == DS_OK)
				{
					HRESULT Error= PrimaryBuffer->SetFormat(&WaveFormat);
					if (Error == DS_OK)
					{
						// Now we have finally set the format
						OutputDebugString("LOL\n");
					}
					else
					{
						// TODO Diagnostic
					}
				}
				else
				{
					// TODO Diagnostic 
				}
			}
			else
			{
				// TODO Diagnostic
			}
			// Create a secondary buffer
			DSBUFFERDESC  BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			LPDIRECTSOUNDBUFFER SecondaryBuffer;
			HRESULT Error = (DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0));
			if (Error == DS_OK)
			{
				OutputDebugString("LOL1\n");
			}
			// Start it playing!
		}
		else
		{
			// TODO Diagnostic
		}

	}
	else
	{
		// TODO Diagnostic
	}
	

}

internal void RenderWeirdGradient(FOffscreenBuffer* Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8*)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Blue = Y + YOffset;
			uint8 Green = X + XOffset;
			*Pixel++ = ((Green << 8) | (Blue));
		}

		Row += Buffer->Pitch;
	}
}

internal void ResizeDIBSection(FOffscreenBuffer* Buffer, int Width, int Height)
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

internal void DisplayBufferInWindow(HDC DeviceContext, FOffscreenBuffer* Buffer, int WindowWidth, int WindowHeight)
{
	// TODO Aspect ratio correction
	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
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
			GlobalRunning = false;
			break;
		}
		case WM_DESTROY:
		{
			// TODO Handle this as an error recreate window
			GlobalRunning = false;
			break;
		}
		case WM_SIZE:
		{

			OutputDebugString("WM_SIZE\n");
			break;
		}
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			bool WasDown = (lParam & 1 << 30) != 0;
			bool IsDown = (lParam & 1 << 31) == 0;
			bool AltKeyWasDown = (lParam & 1 << 29) != 0;
			if (WasDown != IsDown)
			{
				if (wParam == 'W')
				{
				}
				else if (wParam == 'A')
				{
				}
				else if (wParam == 'S')
				{
				}
				else if (wParam == 'D')
				{
				}
				else if (wParam == 'E')
				{
				}
				else if (wParam == 'Q')
				{
				}
				else if (wParam == VK_UP)
				{
				}
				else if (wParam == VK_DOWN)
				{
				}
				else if (wParam == VK_RIGHT)
				{
				}
				else if (wParam == VK_LEFT)
				{
				}
				else if (wParam == VK_ESCAPE)
				{
				}
				else if (wParam == VK_SPACE)
				{
				}
				else if (AltKeyWasDown && wParam == VK_F4)
				{
					GlobalRunning = false;
				}
			}
			break;
		}

		case WM_ACTIVATEAPP:
		{
			OutputDebugString("WM_ACTIVATEAPP\n");

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

			DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);
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
	LoadXInput();

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
		GlobalRunning = true;
		int XOffset = 0;
		int YOffset = 0;

		InitDSound(WindowHandle, 48000, 48000 * sizeof(int16) * 2);
		while (GlobalRunning)
		{
			MSG Message;
			while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
			{
				if (Message.message == WM_QUIT)
				{
					GlobalRunning = false;
				}
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}

			// TODO Should we pull more frequently.
			for (DWORD UserIndex = 0; UserIndex < XUSER_MAX_COUNT; UserIndex++)
			{
				XINPUT_STATE ControllerState;
				if (XInputGetState(UserIndex, &ControllerState) == ERROR_SUCCESS)
				{
					// This controller is plugged in
					XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

					bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
					bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
					bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
					bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
					bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START) != 0;
					bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK) != 0;
					bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
					bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
					bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A) != 0;
					bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B) != 0;
					bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X) != 0;
					bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y) != 0;

					int16 StickX = Pad->sThumbLX;
					int16 StickY = Pad->sThumbLY;
					if(AButton)
						XOffset++;
				}
				else
				{
					// the controller is not available
				}

			}

			HDC DeviceContext = GetDC(WindowHandle);
			FWindowDimension Dimension = GetWindowDimension(WindowHandle);

			DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);

			RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

			ReleaseDC(WindowHandle, DeviceContext);
			YOffset++;
		}
	}
	else
	{
		// TODO add error handle here
	}
	return 0;
}