#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <vector>
#include <fstream>
#include <iostream>

#include "chip8_io.h"

class Chip8 {
 private:
  static const uint32_t REGISTERS_COUNT = 0x10;
  static const uint32_t MEMORY_SIZE = 0x1000;
  static const uint32_t FONT_START_ADDRESS = 0x50;
  static const uint32_t ROM_START_ADDRESS = 0x200;

  std::random_device mRd;
  std::mt19937 mGen;
  std::uniform_int_distribution<uint8_t> mDistrib;

  std::optional<uint16_t> mOpcode{};
  uint16_t mPC{};  // program counter
  uint16_t mI{};   // void pointer to memory address
  std::array<uint8_t, REGISTERS_COUNT> mRegisters;  // registers V0, ..., VF
  uint8_t mDelayTimer{};                            // delay timer
  uint8_t mSoundTimer{};                            // sound timer
  std::array<uint8_t, MEMORY_SIZE> mMemory;         // RAM
  std::vector<uint16_t> mStack;  // stack containing program counters
  std::shared_ptr<Chip8IO> mIO;  // Interface for display and keypad

  uint8_t opcodeType();
  uint8_t opcodeX();
  uint8_t opcodeY();
  uint8_t opcodeN();
  uint8_t opcodeNN();
  uint16_t opcodeNNN();

  void setVF(uint8_t value);

  void execOpcodeType0();
  void execOpcodeType1();
  void execOpcodeType2();
  void execOpcodeType3();
  void execOpcodeType4();
  void execOpcodeType5();
  void execOpcodeType6();
  void execOpcodeType7();
  void execOpcodeType8();
  void execOpcodeType9();
  void execOpcodeTypeA();
  void execOpcodeTypeB();
  void execOpcodeTypeC();
  void execOpcodeTypeD();
  void execOpcodeTypeE();
  void execOpcodeTypeF();

 public:
  Chip8(std::shared_ptr<Chip8IO> io);
  ~Chip8() = default;
  void loadROM(std::ifstream &ROM, std::streamsize size);
  void loadFont(uint8_t font[], size_t size);
  void updateTimers();
  void runCycle();
};
