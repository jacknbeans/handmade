#include "handmade.h"

internal void RenderWeirdGradient(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset) {
  uint8 *row = (uint8 *)a_Buffer->Memory;
  for (int y = 0; y < a_Buffer->Height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < a_Buffer->Width; ++x) {
      uint8 blue = (x + a_BlueOffset);
      uint8 green = (y + a_GreenOffset);

      *pixel++ = ((green << 8) | blue);
    }

    row += a_Buffer->Pitch;
  }
}

internal void GameUpdateAndRender(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset) {
  RenderWeirdGradient(a_Buffer, a_BlueOffset, a_GreenOffset);
}
