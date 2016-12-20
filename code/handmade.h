#if !defined(HANDMADE_H)

struct GameOffscreenBuffer {
  void *memory;
  int width;
  int height;
  int pitch;
};

struct GameSoundOutputBuffer {
  int samplesPerSecond;
  int sampleCount;
  int16 *samples;
};

internal void GameUpdateAndRender(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset,
                                  GameSoundOutputBuffer *a_SoundBuffer,
                                  int a_ToneHz);

#define HANDMADE_H
#endif
