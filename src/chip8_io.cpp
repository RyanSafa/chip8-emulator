#include "chip8_io.hpp"

#include <SDL2/SDL_keyboard.h>

#include <cassert>
#include <cstring>
#include <iostream>

SDL_Window* Chip8IO::createWindow() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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

SDL_AudioDeviceID Chip8IO::createAudioDeviceID() {
  SDL_AudioSpec desired, obtained;
  desired.freq = 44100;           // Sample rate
  desired.format = AUDIO_S16SYS;  // 16-bit signed audio
  desired.channels = 1;           // Mono
  desired.samples = 4096;         // Buffer size
  desired.callback = audioCallback;
  desired.userdata = &mSquareWave;

  SDL_AudioDeviceID device =
      SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

  if (!device) {
    SDL_Log("Failed to open audio: %s\n", SDL_GetError());
    exit(1);
  }

  float frequency = 440.0f;
  mSquareWave.phase_increment = frequency / obtained.freq;
  return device;
}

void Chip8IO::audioCallback(void* userdata, Uint8* stream, int len) {
  SquareWave* wave = (SquareWave*)userdata;
  Sint16* buffer = (Sint16*)stream;
  int num_samples = len / sizeof(Sint16);

  for (int i = 0; i < num_samples; i++) {
    wave->phase += wave->phase_increment;
    if (wave->phase >= 1.0f) wave->phase -= 1.0f;

    Sint16 sample =
        (wave->phase < 0.5f) ? 32767 * wave->volume : -32768 * wave->volume;
    buffer[i] = sample;
  }
}

Chip8IO::Chip8IO(uint32_t scaleFactor, uint32_t onColor, uint32_t offColor)
    : mScaleFactor(scaleFactor),
      mOnColor(onColor),
      mOffColor(offColor),
      mWindow(createWindow()),
      mRenderer(createRenderer()),
      mTexture(createTexture()),
      mSrcRect({0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT}),
      mDestRect(
          {0, 0, DISPLAY_WIDTH * mScaleFactor, DISPLAY_HEIGHT * mScaleFactor}),
      mSquareWave({.phase = 0.0f, .phase_increment = 0.0f, .volume = 0.05f}),
      mAudioDeviceID(createAudioDeviceID()) {
  mDisplayBuffer.fill(offColor);
  for (const std::string& key : KEYS) {
    mKeyPressed[key] = false;
  }
};

Chip8IO::~Chip8IO() {
  SDL_DestroyTexture(mTexture);
  SDL_DestroyRenderer(mRenderer);
  SDL_DestroyWindow(mWindow);
  SDL_CloseAudioDevice(mAudioDeviceID);
  SDL_Quit();
}

void Chip8IO::renderFrame() {
  void* pixels;
  int pitch = DISPLAY_WIDTH;
  SDL_LockTexture(mTexture, NULL, &pixels, &pitch);
  uint32_t* textureBuffer = static_cast<uint32_t*>(pixels);
  memcpy(textureBuffer, std::begin(mDisplayBuffer),
         DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint32_t));

  SDL_UnlockTexture(mTexture);
  SDL_RenderCopy(mRenderer, mTexture, &mSrcRect, &mDestRect);
  SDL_RenderPresent(mRenderer);
}

uint32_t Chip8IO::getDisplayColor(uint32_t x, uint32_t y) {
  assert(x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT);

  return mDisplayBuffer[(y * DISPLAY_WIDTH) + x];
}

void Chip8IO::writeToDisplay(uint32_t x, uint32_t y, bool on) {
  assert(x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT);

  uint32_t index = (y * DISPLAY_WIDTH) + x;
  on ? mDisplayBuffer[index] = mOnColor : mDisplayBuffer[index] = mOffColor;
}

bool Chip8IO::pollInput() {
  SDL_Event e;
  while (SDL_PollEvent(&e) > 0) {
    switch (e.type) {
      case SDL_QUIT: {
        return true;
        break;
      }
      case SDL_KEYUP: {
        SDL_Keycode key = e.key.keysym.sym;
        std::string keyName = std::string(SDL_GetKeyName(key));
        if (mKeyPressed.count(keyName) > 0) {
          mKeyPressed[keyName] = false;
        }
        break;
      }
      case SDL_KEYDOWN: {
        SDL_Keycode key = e.key.keysym.sym;
        std::string keyName = std::string(SDL_GetKeyName(key));
        if (mKeyPressed.count(keyName) > 0) {
          mKeyPressed[keyName] = true;
        }
        break;
      }
    }
  }
  return false;
}

bool Chip8IO::isKeyPressed(uint8_t keyNum) { return mKeyPressed[KEYS[keyNum]]; }

void Chip8IO::playSound() { SDL_PauseAudioDevice(mAudioDeviceID, 0); }

void Chip8IO::pauseSound() { SDL_PauseAudioDevice(mAudioDeviceID, 1); }
