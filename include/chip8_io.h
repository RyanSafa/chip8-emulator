#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

class Chip8IO {
 public:
  static const int32_t DISPLAY_WIDTH = 64;
  static const int32_t DISPLAY_HEIGHT = 32;
  uint32_t mOnColor;
  uint32_t mOffColor;

  Chip8IO(uint32_t scaleFactor, uint32_t onColor, uint32_t offColor);
  ~Chip8IO();
  void beginFrame();
  void endFrame();
  uint32_t getDisplayColor(uint32_t x, uint32_t y);
  void writeToDisplay(uint32_t x, uint32_t y, bool on);
	bool pollInput();

 private:
  int32_t mScaleFactor;
  SDL_Window* mWindow{};
  SDL_Renderer* mRenderer{};
  SDL_Texture* mTexture{};
  uint32_t mPrevFrameDisplay[DISPLAY_HEIGHT * DISPLAY_WIDTH]{};
  uint32_t* mCurrentFrameDisplay;

  SDL_Window* createWindow();
  SDL_Renderer* createRenderer();
  SDL_Texture* createTexture();
};
