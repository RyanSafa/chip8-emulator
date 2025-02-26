#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include "chip8.h"
#include "chip8_io.h"

const uint32_t TARGET_FRAME_RATE = 60;
const uint32_t TARGET_FRAME_TIME_MICRO_SECONDS = 1000000 / TARGET_FRAME_RATE;

void loadROM(Chip8& chip8, const char* ROMPath) {
	printf("%s\n", ROMPath);
  std::ifstream file(ROMPath,
                     std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    std::cout << "File was not openned\n";
    exit(1);
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  chip8.loadROM(file, size);
}

void loadFont(Chip8& chip8)	{
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
}

void runEmulator(Chip8& chip8, std::shared_ptr<Chip8IO> chip8IO, uint32_t instructionsPerSecond) {
  const auto targetFrameDuration =
      std::chrono::microseconds(TARGET_FRAME_TIME_MICRO_SECONDS);

  while (!chip8IO->pollInput()) {
    auto frameStart = std::chrono::steady_clock::now();
    chip8.updateTimers();

    for (int i = 0; i < instructionsPerSecond; i++) {
      chip8.runCycle();
    }

    chip8IO->renderFrame();
    auto frameEnd = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        frameEnd - frameStart);
    auto timeLeft = std::chrono::duration_cast<std::chrono::microseconds>(
        targetFrameDuration - elapsed);

    if (timeLeft.count() > 0) {
      std::this_thread::sleep_for(timeLeft);
    }
  }
}

int main(int argc, char* argv[]) {
  char* ROMPath;
  uint32_t instructionsPerFrame = 11;
  uint32_t scaleFactor = 12;
  uint32_t primaryColor = 0xFFFFFFFF;
  uint32_t backgroundColor = 0x000000FF;

  if (argc < 2) {
    std::cout << "Please enter the full path to a ROM\n";
    return 1;
  }
  ROMPath = argv[1];

  if (argc >= 3) {
    instructionsPerFrame = std::stoi(argv[2]);
  }
  if (argc >= 4) {
    scaleFactor = std::stoi(argv[3]);
  }
  if (argc >= 5) {
    primaryColor = std::stoul(argv[4], nullptr, 16);
  }
  if (argc >= 6) {
    backgroundColor = std::stoul(argv[5], nullptr, 16);
  }

  std::shared_ptr<Chip8IO> chip8IO =
      std::make_unique<Chip8IO>(scaleFactor, primaryColor, backgroundColor);
  Chip8 chip8 = Chip8(chip8IO);

	loadROM(chip8, ROMPath);
	loadFont(chip8);
	
	runEmulator(chip8, chip8IO, instructionsPerFrame);
  return 0;
}
