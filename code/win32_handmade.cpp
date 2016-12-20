#include <math.h>
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
#include <malloc.h>
#include <stdio.h>

struct Win32OffscreenBuffer {
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
};

struct Win32WindowDimension {
  int width;
  int height;
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
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;

  return result;
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *a_Buffer, int a_Width,
                                    int a_Height) {
  if (a_Buffer->memory) {
    VirtualFree(a_Buffer->memory, 0, MEM_RELEASE);
  }

  a_Buffer->width = a_Width;
  a_Buffer->height = a_Height;

  int bytesPerPixel = 4;

  a_Buffer->info.bmiHeader.biSize = sizeof(a_Buffer->info.bmiHeader);
  a_Buffer->info.bmiHeader.biWidth = a_Buffer->width;
  a_Buffer->info.bmiHeader.biHeight = -a_Buffer->height;
  a_Buffer->info.bmiHeader.biPlanes = 1;
  a_Buffer->info.bmiHeader.biBitCount = 32;
  a_Buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = (a_Buffer->width * a_Buffer->height) * bytesPerPixel;
  a_Buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
                                  PAGE_READWRITE);
  a_Buffer->pitch = a_Width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer *a_Buffer,
                                         HDC a_DeviceContext, int a_WindowWidth,
                                         int a_WindowHeight) {
  StretchDIBits(a_DeviceContext, 0, 0, a_WindowWidth, a_WindowHeight, 0, 0,
                a_Buffer->width, a_Buffer->height, a_Buffer->memory,
                &a_Buffer->info, DIB_RGB_COLORS, SRCCOPY);
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
    Win32DisplayBufferInWindow(&g_Backbuffer, DeviceContext, Dimension.width,
                               Dimension.height);
    EndPaint(a_Window, &Paint);
  } break;

  default: {
    result = DefWindowProcA(a_Window, a_Message, a_WParam, a_LParam);
  } break;
  }

  return result;
}

struct Win32SoundOutput {
  int samplesPerSecond;
  int toneHz;
  int16 toneVolume;
  uint32 runningSampleIndex;
  int wavePeriod;
  int bytesPerSample;
  int secondaryBufferSize;
  real32 tSine;
  int latencySampleCount;
};

internal void Win32ClearBuffer(Win32SoundOutput *a_SoundOutput) {
  VOID *region1;
  DWORD region1Size;
  VOID *region2;
  DWORD region2Size;
  if (SUCCEEDED(g_SecondaryBuffer->Lock(0, a_SoundOutput->secondaryBufferSize,
                                        &region1, &region1Size, &region2,
                                        &region2Size, 0))) {
    uint8 *destSample = (uint8 *)region1;
    for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
      *destSample++ = 0;
    }

    destSample = (uint8 *)region2;
    for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
      *destSample++ = 0;
    }

    g_SecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

internal void Win32FillSoundBuffer(Win32SoundOutput *a_SoundOutput,
                                   DWORD a_ByteToLock, DWORD a_BytesToWrite,
                                   GameSoundOutputBuffer *a_SourceBuffer) {
  VOID *region1;
  DWORD region1Size;
  VOID *region2;
  DWORD region2Size;
  if (SUCCEEDED(g_SecondaryBuffer->Lock(a_ByteToLock, a_BytesToWrite, &region1,
                                        &region1Size, &region2, &region2Size,
                                        0))) {
    DWORD region1SampleCount = region1Size / a_SoundOutput->bytesPerSample;
    int16 *destSample = (int16 *)region1;
    int16 *sourceSample = a_SourceBuffer->samples;
    for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount;
         ++sampleIndex) {
      *destSample++ = *sourceSample++;
      *destSample++ = *sourceSample++;
      ++a_SoundOutput->runningSampleIndex;
    }

    DWORD region2SampleCount = region2Size / a_SoundOutput->bytesPerSample;
    destSample = (int16 *)region2;
    for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount;
         ++sampleIndex) {
      *destSample++ = *sourceSample++;
      *destSample++ = *sourceSample++;
      ++a_SoundOutput->runningSampleIndex;
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

      soundOutput.samplesPerSecond = 48000;
      soundOutput.toneHz = 256;
      soundOutput.toneVolume = 3000;
      soundOutput.wavePeriod =
          soundOutput.samplesPerSecond / soundOutput.toneHz;
      soundOutput.bytesPerSample = sizeof(int16) * 2;
      soundOutput.secondaryBufferSize =
          soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
      soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
      Win32InitDSound(window, soundOutput.samplesPerSecond,
                      soundOutput.secondaryBufferSize);
      Win32ClearBuffer(&soundOutput);
      g_SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      g_Running = true;

      int16 *samples =
          (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

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

            soundOutput.toneHz =
                512 + (int)(256.0f * ((real32)stickY / 30000.0f));
            soundOutput.wavePeriod =
                soundOutput.samplesPerSecond / soundOutput.toneHz;
          } else {
          }
        }

        DWORD byteToLock;
        DWORD targetCursor;
        DWORD bytesToWrite;
        DWORD playCursor;
        DWORD writeCursor;
        bool32 soundIsValid = false;
        if (SUCCEEDED(g_SecondaryBuffer->GetCurrentPosition(&playCursor,
                                                            &writeCursor))) {
          byteToLock =
              ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
               soundOutput.secondaryBufferSize);

          targetCursor = ((playCursor + (soundOutput.latencySampleCount *
                                         soundOutput.bytesPerSample)) %
                          soundOutput.secondaryBufferSize);
          if (byteToLock > targetCursor) {
            bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
            bytesToWrite += targetCursor;
          } else {
            bytesToWrite = targetCursor - byteToLock;
          }

          soundIsValid = true;
        }

        GameSoundOutputBuffer soundBuffer = {};
        soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
        soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
        soundBuffer.samples = samples;

        GameOffscreenBuffer buffer = {};
        buffer.memory = g_Backbuffer.memory;
        buffer.width = g_Backbuffer.width;
        buffer.height = g_Backbuffer.height;
        buffer.pitch = g_Backbuffer.pitch;
        GameUpdateAndRender(&buffer, xOffset, yOffset, &soundBuffer,
                            soundOutput.toneHz);

        if (soundIsValid) {
          Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite,
                               &soundBuffer);
        }

        Win32WindowDimension dimension = Win32GetWindowDimension(window);
        Win32DisplayBufferInWindow(&g_Backbuffer, deviceContext,
                                   dimension.width, dimension.height);

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
