/* #include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

const uint32_t MEMORY_SIZE = 4096;
const uint32_t FONT_SIZE = 80;

const uint32_t DISPLAY_WIDTH = 64;
const uint32_t DISPLAY_HEIGHT = 32;
const uint32_t DISPLAY_SCALE = 24;

const uint32_t TARGET_CLOCK_SPEED = 540;
const uint32_t TARGET_FRAME_RATE = 60;
const uint32_t TARGET_FRAME_TIME_MICRO_SECONDS = 1000000 / TARGET_FRAME_RATE;

const uint32_t ON_COLOR = 0x500000FF;
const uint32_t OFF_COLOR = 0x06402BFF;


SDL_Scancode get_scancode(uint8_t value) {
  switch (value) {
  case 0:
    return SDL_SCANCODE_0;
  case 1:
    return SDL_SCANCODE_1;
  case 2:
    return SDL_SCANCODE_2;
  case 3:
    return SDL_SCANCODE_3;
  case 4:
    return SDL_SCANCODE_4;
  case 5:
    return SDL_SCANCODE_5;
  case 6:
    return SDL_SCANCODE_6;
  case 7:
    return SDL_SCANCODE_7;
  case 8:
    return SDL_SCANCODE_8;
  case 9:
    return SDL_SCANCODE_9;
  case 10:
    return SDL_SCANCODE_A;
  case 11:
    return SDL_SCANCODE_B;
  case 12:
    return SDL_SCANCODE_C;
  case 13:
    return SDL_SCANCODE_D;
  case 14:
    return SDL_SCANCODE_E;
  case 15:
    return SDL_SCANCODE_F;
  }
  return SDL_SCANCODE_UNKNOWN;
}
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<uint8_t> distrib(0, 255);
uint16_t PC = 512;
uint16_t I = 0;
uint8_t registers[16];
bool keep_window_open = true;
uint8_t *mem(new uint8_t[MEMORY_SIZE]);
std::vector<uint16_t> stack;
uint8_t delay_timer;
const uint8_t *keys = SDL_GetKeyboardState(NULL);

void cpu_cycle(uint32_t *display) {
  uint8_t first_byte = mem[PC];
  uint8_t second_byte = mem[PC + 1];
  uint16_t opcode = (first_byte << 8) | second_byte;
  PC += 2;
  // DECODE

  uint8_t op_type = (opcode & 0xF000) >> 12;
  uint8_t X = (opcode & 0x0F00) >> 8; // look up one of the 16 registers (VX)
  uint8_t Y = (opcode & 0x00F0) >> 4; // look up one of the 16 registers (VY)
  uint8_t N = (opcode & 0x000F);      // 4 bit number
  uint8_t NN = second_byte;           // 8 bit immediate number
  uint16_t NNN = opcode & 0x0FFF;     // 12 bit immediate memory address

  printf("VF:	%u\n", registers[15]);
  printf("opcode: %hu\n", opcode);
  printf("op_type: %hu\n", op_type);
  printf("X: %hu\n", X);
  printf("Y: %hu\n", Y);
  printf("N: %hu\n", N);
  printf("NN: %hu\n", NN);
  printf("NNN: %hu\n", NNN);
  assert(PC >= 512);

  // EXECUTE
  for (int i = 0; i < 16; i++) {
    assert(registers[i] <= 255);
  }

  switch (op_type) {
  case 0: {
    if (opcode == 0x00E0) {
      for (uint32_t i = 0; i < DISPLAY_HEIGHT; i++) {
        for (uint32_t j = 0; j < DISPLAY_WIDTH; j++) {
          display[i * DISPLAY_WIDTH + j] = OFF_COLOR;
        }
      }
    } else if (opcode == 0x00EE) {
      PC = stack.back();
      stack.pop_back();
    } else {
      // 0NNN - skip
    }
    break;
  }
  case 1: {
    PC = NNN;
    break;
  }
  case 2: {
    stack.push_back(PC);
    PC = NNN;
    break;
  }
  case 3: {
    if (registers[X] == NN) {
      PC += 2;
    }
    break;
  }
  case 4: {
    if (registers[X] != NN) {
      PC += 2;
    }
    break;
  }
  case 5: {
    if (registers[X] == registers[Y]) {
      PC += 2;
    }
    break;
  }
  case 6: {
    registers[X] = NN;
    break;
  }
  case 7: {
    registers[X] = (registers[X] + NN) & 0xFF;
    break;
  }
  case 8: {
    switch (N) {
    case 0: {
      registers[X] = registers[Y];
      break;
    }
    case 1: {
      registers[X] |= registers[Y];
      break;
    }
    case 2: {
      registers[X] &= registers[Y];
      break;
    }
    case 3: {
      registers[X] ^= registers[Y];
      break;
    }
    case 4: {
      uint8_t old_x = registers[X];
      uint8_t sum = registers[X] + registers[Y];
      registers[X] = sum;
      if (sum < old_x) {
        registers[15] = 1;
      } else {
        registers[15] = 0;
      }
      break;
    }
    case 5: {
      uint8_t old_x, old_y;
      old_x = registers[X];
      old_y = registers[Y];
      uint8_t subtr = registers[X] - registers[Y];
      registers[X] = subtr;
      if (old_x >= old_y) {
        registers[15] = 1;
      } else {
        registers[15] = 0;
      }
      break;
    }
    case 6: {
      uint8_t tmp = registers[X] & 0x01;
      uint8_t res = registers[X] >> 1;
      registers[X] = res;
      registers[15] = tmp;
      break;
    }
    case 7: {
      uint8_t old_x, old_y;
      old_x = registers[X];
      old_y = registers[Y];
      uint8_t subtr = registers[Y] - registers[X];
      registers[X] = subtr;
      if (old_y >= old_x) {
        registers[15] = 1;
      } else {
        registers[15] = 0;
      }
      break;
    }
    case 0xE: {
      uint8_t tmp = (registers[X] & 0x80) >> 7;
      uint8_t res = registers[X] << 1;
      registers[X] = res;
      registers[15] = tmp;
      break;
    }
    }
    break;
  }
  case 9: {
    if (registers[X] != registers[Y]) {
      PC += 2;
    }
    break;
  }
  case 0xA: {
    I = NNN;
    break;
  }
  case 0xB: {
    PC = NNN + registers[0];
    break;
  }
  case 0xC: {
    registers[X] = distrib(gen) & NN;
    break;
  }
  case 0xD: {
    uint32_t x_coord = (registers[X] % DISPLAY_WIDTH);
    uint32_t y_coord = (registers[Y] % DISPLAY_HEIGHT);
    registers[15] = 0;
    for (uint16_t i = 0; i < N; i++) {
      if (y_coord + i >= DISPLAY_HEIGHT) {
        continue;
      }
      for (uint16_t j = 0; j < 8; j++) {
        if (x_coord + j >= DISPLAY_WIDTH) {
          continue;
        }
        uint8_t mask = 1 << (7 - j);
        uint8_t sprite_color = (mem[I + i] & mask) >> (7 - j);
        if (sprite_color) {
          uint32_t index = (y_coord + i) * DISPLAY_WIDTH + (x_coord + j);
          if (display[index] == ON_COLOR) {
            registers[15] = 1;
            display[index] = OFF_COLOR;
          } else {
            display[index] = ON_COLOR;
          }
        }
      }
    }
    break;
  }
  case 0xE: {
    if (N == 0xE) {
      if (keys[get_scancode(registers[X])]) {
        PC += 2;
      }
    } else if (N == 1) {
      if (!keys[get_scancode(registers[X])]) {
        PC += 2;
      }
    }
    break;
  }
  case 0xF: {
    if (NN == 7) {
      registers[X] = delay_timer;
    } else if (NN == 0x15) {
      delay_timer = registers[X];
    } else if (NN == 0x18) {
      // sound timer bs
    } else if (NN == 0x1E) {
      I += registers[X];
      if (I >= 0x1000) {
        registers[15] = 1;
      }
    } else if (NN == 0x0A) {
      bool key_presed = false;
      for (uint8_t i = 0; i < 16; i++) {
        if (keys[get_scancode(i)]) {
          registers[X] = i;
          key_presed = true;
        }
      }
      if (!key_presed) {
        PC -= 2;
      }
    } else if (NN == 0x29) {
      I = 80 + (registers[X] * 5);
    } else if (NN == 0x33) {
      std::vector<uint8_t> digits;
      uint8_t cur = registers[X];
      while (cur > 0) {
        digits.push_back(cur % 10);
        cur /= 10;
      }
      while (digits.size() < 3) {
        digits.push_back(0);
      }
      for (int i = digits.size() - 1; i >= 0; i--) {
        mem[I + digits.size() - 1 - i] = digits[i];
      }
    } else if (NN == 0x55) {
      for (uint8_t i = 0; i <= X; i++) {
        mem[I + i] = registers[i];
      }
    } else if (NN == 0x65) {
      for (uint8_t i = 0; i <= X; i++) {
        registers[i] = mem[I + i];
      }
    }
    break;
  }
  default: {
    assert(false);
  }
  }
}

SDL_Window *create_window() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "Failed to initialize the SDL2 library\n";
    return nullptr;
  }

  SDL_Window *window = SDL_CreateWindow(
      "SDL2 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      DISPLAY_WIDTH * DISPLAY_SCALE, DISPLAY_HEIGHT * DISPLAY_SCALE, 0);
  if (!window) {
    std::cout << "Failed to create window\n";
    return nullptr;
  }

  return window;
}

SDL_Renderer *create_renderer(SDL_Window *window) {
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  if (!renderer) {
    SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return nullptr;
  }
  return renderer;
}

int main() {
  SDL_Window *window = create_window();
  if (!window) {
    return -1;
  }

  SDL_Renderer *renderer = create_renderer(window);
  if (!renderer) {
    return -1;
  }
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           DISPLAY_WIDTH, DISPLAY_HEIGHT);
  int pitch = DISPLAY_WIDTH;
  void *pixels;
  uint32_t *display;
  SDL_LockTexture(texture, NULL, &pixels, &pitch);
  display = static_cast<uint32_t *>(pixels);
  for (int i = 0; i < DISPLAY_HEIGHT; i++) {
    for (int j = 0; j < DISPLAY_WIDTH; j++) {
      display[i * DISPLAY_WIDTH + j] = OFF_COLOR;
    }
  }
  SDL_UnlockTexture(texture);

  SDL_Rect srcRect = {0, 0, DISPLAY_WIDTH,
                      DISPLAY_HEIGHT}; // Use the entire texture
  SDL_Rect destRect = {
      0, 0,
      DISPLAY_WIDTH * DISPLAY_SCALE, // Scaled width
      DISPLAY_HEIGHT * DISPLAY_SCALE // Scaled height
  };

  uint8_t font[FONT_SIZE]{
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  for (int i = 0; i < 16; i++) {
    registers[i] = 0;
  }
  memcpy(&mem[80], font, FONT_SIZE * sizeof(uint8_t));

  std::ifstream file("../test-roms/ibm_logo.ch8",
                     std::ios::binary |
                         std::ios::ate); // Open in binary mode & move to end
  if (!file.is_open()) {
    std::cout << "File was not openned\n";
    return -1;
  }

  std::streamsize size = file.tellg(); // Get file size
  file.seekg(0, std::ios::beg);        // Move back to the start

  printf("file size: %zu\n", size);

  if (!file.read(reinterpret_cast<char *>(&mem[512]),
                 size)) { // Read file into buffer
    std::cout << "File was not read\n";
    return -1;
  }

  const auto targetFrameDuration =
      std::chrono::microseconds(TARGET_FRAME_TIME_MICRO_SECONDS);

  while (PC < 512 + size - 1 && keep_window_open) {
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    display = static_cast<uint32_t *>(pixels);

    auto frameStart = std::chrono::steady_clock::now();
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
      switch (e.type) {
      case SDL_QUIT:
        keep_window_open = false;
        break;
      }
    }

    if (delay_timer > 0) {
      delay_timer--;
    }

    for (int i = 0; i < 11; i++) {
      cpu_cycle(display);
    }

    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    SDL_RenderPresent(renderer);

    auto frameEnd = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        frameEnd - frameStart);
    auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(
        targetFrameDuration - elapsed);
    printf("sleep for: %ld\n", tmp.count());
    std::this_thread::sleep_for(tmp);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
} */

#include <chrono>
#include <fstream>
#include <memory>
#include <thread>

#include "chip8.h"
#include "chip8_io.h"

const uint32_t TARGET_FRAME_RATE = 60;
const uint32_t TARGET_FRAME_TIME_MICRO_SECONDS = 1000000 / TARGET_FRAME_RATE;

int main() {
  std::shared_ptr<Chip8IO> chip8IO =
      std::make_unique<Chip8IO>(12, 0xFFFFFFFF, 0x000000FF);
  Chip8 chip8 = Chip8(chip8IO);

  // Load ROM
  std::ifstream file(
      "/home/ryan/chip8-emulator/test-roms/ibm_logo.ch8",
      std::ios::binary | std::ios::ate);  // Open in binary mode & move to end

  if (!file.is_open()) {
    std::cout << "File was not openned\n";
    return -1;
  }

  std::streamsize size = file.tellg();  // Get file size
  file.seekg(0, std::ios::beg);         // Move back to the start
  chip8.loadROM(file, size);

  // Load font
	const uint32_t FONT_SIZE = 80;
  uint8_t font[FONT_SIZE]{
      0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
      0x20, 0x60, 0x20, 0x20, 0x70,  // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
      0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
      0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
      0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
      0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
      0xF0, 0x80, 0xF0, 0x80, 0x80   // F
  };
  chip8.loadFont(font, FONT_SIZE);

  const auto targetFrameDuration =
      std::chrono::microseconds(TARGET_FRAME_TIME_MICRO_SECONDS);

  while (!chip8IO->pollInput()) {
    auto frameStart = std::chrono::steady_clock::now();
    chip8IO->beginFrame();
    chip8.updateTimers();

    for (int i = 0; i < 11; i++) {
      chip8.runCycle();
    }

    chip8IO->endFrame();

    auto frameEnd = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        frameEnd - frameStart);
    auto timeLeft = std::chrono::duration_cast<std::chrono::microseconds>(
        targetFrameDuration - elapsed);
		printf("timeLeft, %ld\n", timeLeft.count());
    if (timeLeft.count() > 0) {
      std::this_thread::sleep_for(timeLeft);
    }
  }
  return 0;
}
