#include "chip8_io.h"

#include <cassert>
#include <cstring>
#include <iostream>

SDL_Window* Chip8IO::createWindow() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "Failed to initialize the SDL2 library\n";
    exit(1);
  }

  SDL_Window* window = SDL_CreateWindow(
      "SDL2 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      DISPLAY_WIDTH * mScaleFactor, DISPLAY_HEIGHT * mScaleFactor, 0);
  if (!window) {
    std::cout << "Failed to create window\n";
    exit(1);
  }

  return window;
}

SDL_Renderer* Chip8IO::createRenderer() {
  SDL_Renderer* renderer =
      SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);

  if (!renderer) {
    std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError()
              << "\n";
    SDL_Quit();
    exit(1);
  }
  return renderer;
}

SDL_Texture* Chip8IO::createTexture() {
  SDL_Texture* texture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           DISPLAY_WIDTH, DISPLAY_HEIGHT);
  if (!texture) {
    std::cout << "Texture could not be created! SDL_Error: " << SDL_GetError()
              << "\n";
    SDL_Quit();
    exit(1);
  }
  return texture;
}

Chip8IO::Chip8IO(uint32_t scaleFactor, uint32_t onColor, uint32_t offColor)
    : mScaleFactor(scaleFactor),
      mOnColor(onColor),
      mOffColor(offColor),
      mWindow(createWindow()),
      mRenderer(createRenderer()),
      mTexture(createTexture()),
      mPrevFrameDisplay(offColor){};

Chip8IO::~Chip8IO() {
  SDL_DestroyTexture(mTexture);
  SDL_DestroyRenderer(mRenderer);
  SDL_DestroyWindow(mWindow);
  SDL_Quit();
}

void Chip8IO::beginFrame() {
  void* pixels;
  int pitch = DISPLAY_WIDTH;
  SDL_LockTexture(mTexture, NULL, &pixels, &pitch);
  mCurrentFrameDisplay = static_cast<uint32_t*>(pixels);
}

void Chip8IO::endFrame() {
  SDL_Rect srcRect = {0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT};
  SDL_Rect destRect = {0, 0, DISPLAY_WIDTH * mScaleFactor,
                       DISPLAY_HEIGHT * mScaleFactor};
  SDL_UnlockTexture(mTexture);
  SDL_RenderCopy(mRenderer, mTexture, &srcRect, &destRect);
  SDL_RenderPresent(mRenderer);
  memcpy(mPrevFrameDisplay, mCurrentFrameDisplay,
         DISPLAY_WIDTH * DISPLAY_HEIGHT);
  mCurrentFrameDisplay = nullptr;
}

uint32_t Chip8IO::getDisplayColor(uint32_t x, uint32_t y) {
  assert(mPrevFrameDisplay);
  return mPrevFrameDisplay[y * DISPLAY_WIDTH + x];
}

void Chip8IO::writeToDisplay(uint32_t x, uint32_t y, bool on) {
  assert(mCurrentFrameDisplay);
  assert(x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT);

  uint32_t index = (y * DISPLAY_WIDTH) + x;
  on ? mCurrentFrameDisplay[index] = mOnColor
     : mCurrentFrameDisplay[index] = mOffColor;
}

bool Chip8IO::pollInput() {
  SDL_Event e;
  while (SDL_PollEvent(&e) > 0) {
    switch (e.type) {
      case SDL_QUIT:
        return true;
    }
  }
  return false;
}
