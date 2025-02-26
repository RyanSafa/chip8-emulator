#pragma once
#include <SDL2/SDL.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

class Chip8IO {
 public:
  static const int32_t DISPLAY_WIDTH = 64;
  static const int32_t DISPLAY_HEIGHT = 32;
  static const size_t NUM_KEYS = 16;

  uint32_t mOnColor;
  uint32_t mOffColor;

  Chip8IO(uint32_t scaleFactor, uint32_t onColor, uint32_t offColor);
  ~Chip8IO();
  void renderFrame();
  uint32_t getDisplayColor(uint32_t x, uint32_t y);
  void writeToDisplay(uint32_t x, uint32_t y, bool on);
  bool isKeyPressed(uint8_t);
  bool pollInput();

 private:
  static constexpr std::array<std::string, NUM_KEYS> KEYS = {
      "0", "1", "2", "3", "4", "5", "6", "7",
      "8", "9", "A", "B", "C", "D", "E", "F"};
  int32_t mScaleFactor;
  SDL_Window* mWindow{};
  SDL_Renderer* mRenderer{};
  SDL_Texture* mTexture{};
  std::array<uint32_t, DISPLAY_HEIGHT * DISPLAY_WIDTH> mDisplayBuffer;
  SDL_Rect srcRect;
  SDL_Rect destRect;
  std::unordered_map<std::string, bool> mKeyPressed;

  SDL_Window* createWindow();
  SDL_Renderer* createRenderer();
  SDL_Texture* createTexture();
};
