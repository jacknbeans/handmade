#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32_t bool32;
bool test;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"
#include "handmade.h"

#include <Windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <stdio.h>

#include <math.h>

struct Win32OffscreenBuffer {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct Win32WindowDimension {
  int Width;
  int Height;
};

global_variable bool32 g_Running;
global_variable Win32OffscreenBuffer g_Backbuffer;
global_variable LPDIRECTSOUNDBUFFER g_SecondaryBuffer;

#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name)                                              \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS,               \
                      LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void *PlatformLoadFile(char *a_FileName) { return (0); }

internal void Win32LoadXInput(void) {
  HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!xInputLibrary) {
    xInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }

  if (!xInputLibrary) {
    xInputLibrary = LoadLibraryA("xinput1_3.dll");
  }

  if (xInputLibrary) {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
    if (!XInputGetState) {
      XInputGetState = XInputGetStateStub;
    }

    XInputSetState =
        (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    if (!XInputSetState) {
      XInputSetState = XInputSetStateStub;
    }
  } else {
  }
}

internal void Win32InitDSound(HWND a_Window, int32 a_SamplesPerSecond,
                              int32 a_BufferSize) {
  HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
  if (dSoundLibrary) {
    direct_sound_create *DirectSoundCreate =
        (direct_sound_create *)GetProcAddress(dSoundLibrary,
                                              "DirectSoundCreate");

    LPDIRECTSOUND directSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0))) {
      WAVEFORMATEX waveFormat = {};
      waveFormat.wFormatTag = WAVE_FORMAT_PCM;
      waveFormat.nChannels = 2;
      waveFormat.nSamplesPerSec = a_SamplesPerSecond;
      waveFormat.wBitsPerSample = 16;
      waveFormat.nBlockAlign =
          (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
      waveFormat.nAvgBytesPerSec =
          waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
      waveFormat.cbSize = 0;

      if (SUCCEEDED(
              directSound->SetCooperativeLevel(a_Window, DSSCL_PRIORITY))) {
        DSBUFFERDESC bufferDescription = {};
        bufferDescription.dwSize = sizeof(bufferDescription);
        bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER primaryBuffer;
        if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription,
                                                     &primaryBuffer, 0))) {
          HRESULT error = primaryBuffer->SetFormat(&waveFormat);
          if (SUCCEEDED(error)) {
            OutputDebugStringA("Primary buffer format was set.\n");
          } else {
          }
        } else {
        }
      } else {
      }

      DSBUFFERDESC bufferDescription = {};
      bufferDescription.dwSize = sizeof(bufferDescription);
      bufferDescription.dwFlags = 0;
      bufferDescription.dwBufferBytes = a_BufferSize;
      bufferDescription.lpwfxFormat = &waveFormat;
      HRESULT error = directSound->CreateSoundBuffer(&bufferDescription,
                                                     &g_SecondaryBuffer, 0);
      if (SUCCEEDED(error)) {
        OutputDebugStringA("Secondary buffer created successfully.\n");
      }
    } else {
    }
  } else {
  }
}

internal Win32WindowDimension Win32GetWindowDimension(HWND a_Window) {
  Win32WindowDimension result;

  RECT clientRect;
  GetClientRect(a_Window, &clientRect);
  result.Width = clientRect.right - clientRect.left;
  result.Height = clientRect.bottom - clientRect.top;

  return result;
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *a_Buffer, int a_Width,
                                    int a_Height) {
  if (a_Buffer->Memory) {
    VirtualFree(a_Buffer->Memory, 0, MEM_RELEASE);
  }

  a_Buffer->Width = a_Width;
  a_Buffer->Height = a_Height;

  int bytesPerPixel = 4;

  a_Buffer->Info.bmiHeader.biSize = sizeof(a_Buffer->Info.bmiHeader);
  a_Buffer->Info.bmiHeader.biWidth = a_Buffer->Width;
  a_Buffer->Info.bmiHeader.biHeight = -a_Buffer->Height;
  a_Buffer->Info.bmiHeader.biPlanes = 1;
  a_Buffer->Info.bmiHeader.biBitCount = 32;
  a_Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = (a_Buffer->Width * a_Buffer->Height) * bytesPerPixel;
  a_Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
                                  PAGE_READWRITE);
  a_Buffer->Pitch = a_Width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer *a_Buffer,
                                         HDC a_DeviceContext, int a_WindowWidth,
                                         int a_WindowHeight) {
  StretchDIBits(a_DeviceContext, 0, 0, a_WindowWidth, a_WindowHeight, 0, 0,
                a_Buffer->Width, a_Buffer->Height, a_Buffer->Memory,
                &a_Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND a_Window, UINT a_Message,
                                                  WPARAM a_WParam,
                                                  LPARAM a_LParam) {
  LRESULT result = 0;

  switch (a_Message) {
  case WM_CLOSE: {
    g_Running = false;
  } break;

  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;

  case WM_DESTROY: {
    g_Running = false;
  } break;

  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    uint32 VKCode = a_WParam;
    bool32 WasDown = ((a_LParam & (1 << 30)) != 0);
    bool32 IsDown = ((a_LParam & (1 << 31)) == 0);
    if (WasDown != IsDown) {
      if (VKCode == 'W') {
      } else if (VKCode == 'A') {
      } else if (VKCode == 'S') {
      } else if (VKCode == 'D') {
      } else if (VKCode == 'Q') {
      } else if (VKCode == 'E') {
      } else if (VKCode == VK_UP) {
      } else if (VKCode == VK_LEFT) {
      } else if (VKCode == VK_DOWN) {
      } else if (VKCode == VK_RIGHT) {
      } else if (VKCode == VK_ESCAPE) {
        OutputDebugStringA("ESCAPE: ");
        if (IsDown) {
          OutputDebugStringA("IsDown ");
        }
        if (WasDown) {
          OutputDebugStringA("WasDown");
        }
        OutputDebugStringA("\n");
      } else if (VKCode == VK_SPACE) {
      }
    }

    bool32 AltKeyWasDown = (a_LParam & (1 << 29));
    if ((VKCode == VK_F4) && AltKeyWasDown) {
      g_Running = false;
    }
  } break;

  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(a_Window, &Paint);
    Win32WindowDimension Dimension = Win32GetWindowDimension(a_Window);
    Win32DisplayBufferInWindow(&g_Backbuffer, DeviceContext, Dimension.Width,
                               Dimension.Height);
    EndPaint(a_Window, &Paint);
  } break;

  default: {
    result = DefWindowProcA(a_Window, a_Message, a_WParam, a_LParam);
  } break;
  }

  return result;
}

struct Win32SoundOutput {
  int SamplesPerSecond;
  int ToneHz;
  int16 ToneVolume;
  uint32 RunningSampleIndex;
  int WavePeriod;
  int BytesPerSample;
  int SecondaryBufferSize;
  real32 tSine;
  int LatencySampleCount;
};

internal void Win32FillSoundBuffer(Win32SoundOutput *a_SoundOutput,
                                   DWORD a_ByteToLock, DWORD a_BytesToWrite) {
  VOID *region1;
  DWORD region1Size;
  VOID *region2;
  DWORD region2Size;
  if (SUCCEEDED(g_SecondaryBuffer->Lock(a_ByteToLock, a_BytesToWrite, &region1,
                                        &region1Size, &region2, &region2Size,
                                        0))) {
    DWORD region1SampleCount = region1Size / a_SoundOutput->BytesPerSample;
    int16 *sampleOut = (int16 *)region1;
    for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount;
         ++sampleIndex) {
      real32 sineValue = sinf(a_SoundOutput->tSine);
      int16 sampleValue = (int16)(sineValue * a_SoundOutput->ToneVolume);
      *sampleOut++ = sampleValue;
      *sampleOut++ = sampleValue;

      a_SoundOutput->tSine +=
          2.0f * Pi32 * 1.0f / (real32)a_SoundOutput->WavePeriod;
      ++a_SoundOutput->RunningSampleIndex;
    }

    DWORD region2SampleCount = region2Size / a_SoundOutput->BytesPerSample;
    sampleOut = (int16 *)region2;
    for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount;
         ++sampleIndex) {
      real32 sineValue = sinf(a_SoundOutput->tSine);
      int16 sampleValue = (int16)(sineValue * a_SoundOutput->ToneVolume);
      *sampleOut++ = sampleValue;
      *sampleOut++ = sampleValue;

      a_SoundOutput->tSine +=
          2.0f * Pi32 * 1.0f / (real32)a_SoundOutput->WavePeriod;
      ++a_SoundOutput->RunningSampleIndex;
    }

    g_SecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

int CALLBACK WinMain(HINSTANCE a_Instance, HINSTANCE a_PrevInstance,
                     LPSTR a_CommandLine, int a_ShowCode) {
  LARGE_INTEGER perfCountFrequencyResult;
  QueryPerformanceFrequency(&perfCountFrequencyResult);
  int64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

  Win32LoadXInput();

  WNDCLASSA windowClass = {};

  Win32ResizeDIBSection(&g_Backbuffer, 1280, 720);

  windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpfnWndProc = Win32MainWindowCallback;
  windowClass.hInstance = a_Instance;
  windowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClassA(&windowClass)) {
    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, a_Instance, 0);
    if (window) {
      HDC deviceContext = GetDC(window);

      int xOffset = 0;
      int yOffset = 0;

      Win32SoundOutput soundOutput = {};

      soundOutput.SamplesPerSecond = 48000;
      soundOutput.ToneHz = 256;
      soundOutput.ToneVolume = 3000;
      soundOutput.WavePeriod =
          soundOutput.SamplesPerSecond / soundOutput.ToneHz;
      soundOutput.BytesPerSample = sizeof(int16) * 2;
      soundOutput.SecondaryBufferSize =
          soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
      soundOutput.LatencySampleCount = soundOutput.SamplesPerSecond / 15;
      Win32InitDSound(window, soundOutput.SamplesPerSecond,
                      soundOutput.SecondaryBufferSize);
      Win32FillSoundBuffer(&soundOutput, 0,
                           soundOutput.LatencySampleCount *
                               soundOutput.BytesPerSample);
      g_SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      g_Running = true;

      LARGE_INTEGER lastCounter;
      QueryPerformanceCounter(&lastCounter);
      uint64 lastCycleCount = __rdtsc();
      while (g_Running) {
        MSG message;

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
          if (message.message == WM_QUIT) {
            g_Running = false;
          }

          TranslateMessage(&message);
          DispatchMessageA(&message);
        }

        for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT;
             ++controllerIndex) {
          XINPUT_STATE controllerState;
          if (XInputGetState(controllerIndex, &controllerState) ==
              ERROR_SUCCESS) {
            XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

            bool32 up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool32 down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool32 left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool32 right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool32 start = (pad->wButtons & XINPUT_GAMEPAD_START);
            bool32 back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool32 leftShoulder =
                (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool32 rightShoulder =
                (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool32 aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
            bool32 bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
            bool32 xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
            bool32 yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

            int16 stickX = pad->sThumbLX;
            int16 stickY = pad->sThumbLY;

            xOffset += stickX / 4096;
            yOffset += stickY / 4096;

            soundOutput.ToneHz =
                512 + (int)(256.0f * ((real32)stickY / 30000.0f));
            soundOutput.WavePeriod =
                soundOutput.SamplesPerSecond / soundOutput.ToneHz;
          } else {
          }
        }

        GameOffscreenBuffer buffer = {};
        buffer.Memory = g_Backbuffer.Memory;
        buffer.Width = g_Backbuffer.Width;
        buffer.Height = g_Backbuffer.Height;
        buffer.Pitch = g_Backbuffer.Pitch;
        GameUpdateAndRender(&buffer, xOffset, yOffset);

        DWORD playCursor;
        DWORD writeCursor;
        if (SUCCEEDED(g_SecondaryBuffer->GetCurrentPosition(&playCursor,
                                                            &writeCursor))) {
          DWORD byteToLock =
              ((soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) %
               soundOutput.SecondaryBufferSize);

          DWORD targetCursor = ((playCursor + (soundOutput.LatencySampleCount *
                                               soundOutput.BytesPerSample)) %
                                soundOutput.SecondaryBufferSize);
          DWORD bytesToWrite;
          if (byteToLock > targetCursor) {
            bytesToWrite = (soundOutput.SecondaryBufferSize - byteToLock);
            bytesToWrite += targetCursor;
          } else {
            bytesToWrite = targetCursor - byteToLock;
          }

          Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
        }

        Win32WindowDimension dimension = Win32GetWindowDimension(window);
        Win32DisplayBufferInWindow(&g_Backbuffer, deviceContext,
                                   dimension.Width, dimension.Height);

        uint64 endCycleCount = __rdtsc();

        LARGE_INTEGER endCounter;
        QueryPerformanceCounter(&endCounter);

        uint64 cyclesElapsed = endCycleCount - lastCycleCount;
        int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
        real64 msPerFrame =
            (((1000.0f * (real64)counterElapsed) / (real64)perfCountFrequency));
        real64 fps = (real64)perfCountFrequency / (real64)counterElapsed;
        real64 mcpf = ((real64)cyclesElapsed / (1000.0f * 1000.0f));

#if 0
                char buffer[256];
                sprintf(buffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", msPerFrame, fps, mcpf);
                OutputDebugStringA(buffer);
#endif

        lastCounter = endCounter;
        lastCycleCount = endCycleCount;
      }
    } else {
    }
  } else {
  }

  return (0);
}
