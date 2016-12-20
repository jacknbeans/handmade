#if !defined(HANDMADE_H)

struct GameOffscreenBuffer {
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};
internal void GameUpdateAndRender(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset);

#define HANDMADE_H
#endif
