#include "handmade.h"

internal void GameOutputSound(GameSoundOutputBuffer *a_SoundBuffer,
                              int a_ToneHz) {
  local_persist real32 tSine;
  int16 toneVolume = 3000;
  int wavePeriod = a_SoundBuffer->samplesPerSecond / a_ToneHz;

  int16 *sampleOut = a_SoundBuffer->samples;
  for (int sampleIndex = 0; sampleIndex < a_SoundBuffer->sampleCount;
       ++sampleIndex) {
    real32 sineValue = sinf(tSine);
    int16 sampleValue = (int16)(sineValue * toneVolume);
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    tSine += 2.f * Pi32 * 1.0f / (real32)wavePeriod;
  }
}

internal void RenderWeirdGradient(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset) {
  uint8 *row = (uint8 *)a_Buffer->memory;
  for (int y = 0; y < a_Buffer->height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < a_Buffer->width; ++x) {
      uint8 blue = (x + a_BlueOffset);
      uint8 green = (y + a_GreenOffset);

      *pixel++ = ((green << 8) | blue);
    }

    row += a_Buffer->pitch;
  }
}

internal void GameUpdateAndRender(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset,
                                  GameSoundOutputBuffer *a_SoundBuffer,
                                  int a_ToneHz) {
  GameOutputSound(a_SoundBuffer, a_ToneHz);
  RenderWeirdGradient(a_Buffer, a_BlueOffset, a_GreenOffset);
}
