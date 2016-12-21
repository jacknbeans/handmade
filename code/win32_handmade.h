#if !defined(WIN32_HANDMADE_H)

#include "defines.h"
#include <windows.h>

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

struct Win32SoundOutput {
  int samplesPerSecond;
  uint32 runningSampleIndex;
  int bytesPerSample;
  int secondaryBufferSize;
  real32 tSine;
  int latencySampleCount;
};

#define WIN32_HANDMADE_H
#endif
