#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static 
#define global_variable static 

#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;
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

// this is global for now
global_variable bool GlobalRunning;
global_variable FOffscreenBuffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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

	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibrary("xinput9_1_0.dll");
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

struct FWindowDimension
{
	int Width;
	int Height;
};

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
		direct_sound_create *DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

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
					if (PrimaryBuffer->SetFormat(&WaveFormat) == DS_OK)
					{
						// Now we have finally set the format
						OutputDebugString("Creating primary buffer succedded\n");
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

			if (DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0) == DS_OK)
			{
				OutputDebugString("Creating secondary buffer succedded\n");
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

internal void InitializeBitBltBuffer(FOffscreenBuffer* Buffer, int Width, int Height)
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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	// TODO probably clear this to black 

	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

internal void DisplayBufferInWindow(HDC DeviceContext, FOffscreenBuffer* Buffer, int WindowWidth, int WindowHeight)
{
	// TODO Aspect ratio correction
	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

struct FSoundOutput
{
	int SamplesPerSec;
	int BytesPerSample;
	int SecondaryBufferSize;
	int ToneHz;
	int16 ToneVolume;
	uint32 RunningSampleIndex;
	int WavePeriod;
	real32 tSin = 0;
	int LatencySampleCount;
};

void FillSoundBuffer(FSoundOutput *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;
	if (GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0) == DS_OK)
	{
		// TODO assert that Region1Size/Region2Size is valid 
		int16 *SampleOut = (int16*)Region1;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
		{
			real32 SampleValue = sinf(SoundOutput->tSin) * SoundOutput->ToneVolume;
			*SampleOut++ = (int16)SampleValue;
			*SampleOut++ = (int16)SampleValue;
			SoundOutput->tSin += 2.0f * Pi32 * (real32)1.0f / (real32)SoundOutput->WavePeriod;
			SoundOutput->RunningSampleIndex++;
		}

		SampleOut = (int16*)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
		{
			real32 SampleValue = sinf(SoundOutput->tSin) *SoundOutput->ToneVolume;
			*SampleOut++ = (int16)SampleValue;
			*SampleOut++ = (int16)SampleValue;
			SoundOutput->tSin += 2.0f * Pi32 * (real32)1.0f / (real32)SoundOutput->WavePeriod;
			SoundOutput->RunningSampleIndex++;
		}
	}
	GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	LARGE_INTEGER PerformanceFrequencyResult;
	QueryPerformanceFrequency(&PerformanceFrequencyResult);
	int64 PerfCounterFreq = PerformanceFrequencyResult.QuadPart;

	LoadXInput();

	WNDCLASS WindowClass = {};
	/******** Initialize the draw buffer ********/
	InitializeBitBltBuffer(&GlobalBackBuffer, 1280, 720);
	/************************************************/

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = hInstance;
	// WindowClass.hIcon;
	// WindowClass.lpszMenuName;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

		if (WindowHandle)
		{
			GlobalRunning = true;
			// values for animatiing the gradient
			int XOffset = 0;
			int YOffset = 0;
			/**************************************** Initializing Sound ****************************************/

			FSoundOutput SoundOutput = {};
			SoundOutput.SamplesPerSec = 48000;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSec * SoundOutput.BytesPerSample;
			SoundOutput.ToneHz = 200;
			SoundOutput.ToneVolume = 6000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSec / SoundOutput.ToneHz;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSec / 15;
			InitDSound(WindowHandle, SoundOutput.SamplesPerSec, SoundOutput.SecondaryBufferSize);
			FillSoundBuffer(&SoundOutput, 0, (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			/***************************************************************************************************/
			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			int64 LastCycleCount = __rdtsc();
			while (GlobalRunning)
			{
				LARGE_INTEGER BeginCounter;
				QueryPerformanceCounter(&BeginCounter);

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

#pragma region Gamepad
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
						if (UserIndex == 0)
						{
							int16 StickX = Pad->sThumbLX;
							int16 StickY = Pad->sThumbLY;

							XOffset += StickX / 10000;
							YOffset += StickY / 10000;

							SoundOutput.ToneHz = 512 + (int)(512.0f * ((real32)StickX / 30000.0f));
							SoundOutput.WavePeriod = SoundOutput.SamplesPerSec / SoundOutput.ToneHz;
						}
					}
					else
					{
						// the controller is not available
					}

				}
#pragma endregion

#pragma region Drawing Gradient

				HDC DeviceContext = GetDC(WindowHandle);

				FWindowDimension Dimension = GetWindowDimension(WindowHandle);
				DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);
				RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
				ReleaseDC(WindowHandle, DeviceContext);
#pragma endregion

#pragma region Direct sound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
				{
					DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
					DWORD BytesToWrite;

					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}

					FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);

				}
#pragma endregion
				int64 EndCycleCount = __rdtsc();


				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				// TODO Display the value here
				int64 CycleElapsed = EndCycleCount - LastCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				int32 MSPerFrame = (int32)(1000 * CounterElapsed)/PerfCounterFreq;
				int32 FPS = PerfCounterFreq / CounterElapsed;
				int32 MCPF = (int32) (CycleElapsed / (1000000));
				char Buffer[256];
				wsprintf(Buffer, "%dms/f, %df/s, %dmc/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugString(Buffer);

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;
			}
		}
		else
		{
			// TODO Logging
		}
	}
	else
	{
		// TODO Logging
	}
	return 0;
}