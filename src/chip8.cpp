#include "chip8.h"

#include <cassert>
#include <cstring>
#include <optional>

Chip8::Chip8(std::shared_ptr<Chip8IO> io)
    : mGen(mRd()),
      mDistrib(0, 255),
      mPC(512),
      mDelayTimer(60),
      mSoundTimer(60),
      mIO(io) {
  mRegisters.fill(0);
  mMemory.fill(0);
};

uint8_t Chip8::opcodeType() { return (*mOpcode & 0xF000) >> 12; }

uint8_t Chip8::opcodeX() { return (*mOpcode & 0x0F00) >> 8; }

uint8_t Chip8::opcodeY() { return (*mOpcode & 0x00F0) >> 4; }

uint8_t Chip8::opcodeN() { return *mOpcode & 0x000F; }

uint8_t Chip8::opcodeNN() { return *mOpcode & 0x00FF; }

uint16_t Chip8::opcodeNNN() { return *mOpcode & 0x0FFF; }

void Chip8::setVF(uint8_t value) { mRegisters[0xF] = value; }

void Chip8::execOpcodeType0() {
  switch (opcodeNNN()) {
    case 0x0E0: {
      for (uint32_t i = 0; i < mIO->DISPLAY_HEIGHT; i++) {
        for (uint32_t j = 0; j < mIO->DISPLAY_WIDTH; j++) {
          mIO->writeToDisplay(j, i, false);
        }
      }
      break;
    }
    case 0x0EE: {
      mPC = mStack.back();
      mStack.pop_back();
      break;
    }
    default:
      break;
  }
  return;
}

void Chip8::execOpcodeType1() { mPC = opcodeNNN(); }

void Chip8::execOpcodeType2() {
  mStack.push_back(mPC);
  mPC = opcodeNNN();
}

void Chip8::execOpcodeType3() {
  if (mRegisters[opcodeX()] == opcodeNN()) {
    mPC += 2;
  }
}

void Chip8::execOpcodeType4() {
  if (mRegisters[opcodeX()] != opcodeNN()) {
    mPC += 2;
  }
}

void Chip8::execOpcodeType5() {
  if (mRegisters[opcodeX()] == mRegisters[opcodeY()]) {
    mPC += 2;
  }
}

void Chip8::execOpcodeType6() { mRegisters[opcodeX()] = opcodeNN(); }

void Chip8::execOpcodeType7() {
  mRegisters[opcodeX()] = (mRegisters[opcodeX()] + opcodeNN()) & 0xFF;
}

void Chip8::execOpcodeType8() {
  switch (opcodeN()) {
    case 0x0: {
      mRegisters[opcodeX()] = mRegisters[opcodeY()];
      break;
    }
    case 0x1: {
      mRegisters[opcodeX()] |= mRegisters[opcodeY()];
      break;
    }
    case 0x2: {
      mRegisters[opcodeX()] &= mRegisters[opcodeY()];
      break;
    }
    case 0x3: {
      mRegisters[opcodeX()] ^= mRegisters[opcodeY()];
      break;
    }
    case 0x4: {
      uint8_t sum = mRegisters[opcodeX()] + mRegisters[opcodeY()];
      uint8_t VFValue;
      if (sum < mRegisters[opcodeX()]) {
        VFValue = 1;
      } else {
        VFValue = 0;
      }
      mRegisters[opcodeX()] = sum;
      setVF(VFValue);
      break;
    }
    case 0x5: {
      uint8_t VFValue;
      if (mRegisters[opcodeX()] >= mRegisters[opcodeY()]) {
        VFValue = 1;
      } else {
        VFValue = 0;
      }
      mRegisters[opcodeX()] = mRegisters[opcodeX()] - mRegisters[opcodeY()];
      setVF(VFValue);
      break;
    }
    case 0x6: {
      uint8_t VFValue = mRegisters[opcodeX()] & 0x01;
      mRegisters[opcodeX()] = mRegisters[opcodeX()] >> 1;
      setVF(VFValue);
      break;
    }
    case 0x7: {
      uint8_t VFValue;
      if (mRegisters[opcodeX()] <= mRegisters[opcodeY()]) {
        VFValue = 1;
      } else {
        VFValue = 0;
      }
      mRegisters[opcodeX()] = mRegisters[opcodeY()] - mRegisters[opcodeX()];
      setVF(VFValue);
      break;
    }
    case 0xE: {
      uint8_t VFValue = (mRegisters[opcodeX()] & 0x80) >> 7;
      mRegisters[opcodeX()] = mRegisters[opcodeX()] << 1;
      setVF(VFValue);
      break;
    }
  }
}
void Chip8::execOpcodeType9() {
  if (mRegisters[opcodeX()] != mRegisters[opcodeY()]) {
    mPC += 2;
  }
}

void Chip8::execOpcodeTypeA() { mI = opcodeNNN(); }

void Chip8::execOpcodeTypeB() { mPC = opcodeNNN() + mRegisters[0]; }

void Chip8::execOpcodeTypeC() {
  mRegisters[opcodeX()] = mDistrib(mGen) & opcodeNN();
}

void Chip8::execOpcodeTypeD() {
  uint32_t xCoord = (mRegisters[opcodeX()] % mIO->DISPLAY_WIDTH);
  uint32_t yCoord = (mRegisters[opcodeY()] % mIO->DISPLAY_HEIGHT);
  setVF(0);

  for (uint16_t i = 0; i < opcodeN(); i++) {
    uint32_t newYCoord = yCoord + i;
    if (newYCoord >= mIO->DISPLAY_HEIGHT) {
      continue;
    }
    for (uint16_t j = 0; j < 8; j++) {
      uint32_t newXCoord = xCoord + j;
      if (newXCoord >= mIO->DISPLAY_WIDTH) {
        continue;
      }
      uint8_t mask = 1 << (7 - j);
      uint8_t spriteColor = (mMemory[mI + i] & mask) >> (7 - j);
      bool prevFrameOn =
          mIO->getDisplayColor(newXCoord, newYCoord) == mIO->mOnColor;
      if (spriteColor) {
        if (prevFrameOn) {
          setVF(1);
        }
        mIO->writeToDisplay(newXCoord, newYCoord, !prevFrameOn);
      } else {
        mIO->writeToDisplay(newXCoord, newYCoord, prevFrameOn);
      }
    }
  }
}
void Chip8::execOpcodeTypeE() {
  switch (opcodeN()) {
    case 0x1: {
      // skip
      break;
    }
    case 0xE: {
      // skip
      break;
    }
  }
}
void Chip8::execOpcodeTypeF() {
  switch (opcodeNN()) {
    case 0x7: {
      mRegisters[opcodeX()] = mDelayTimer;
      break;
    }
    case 0x15: {
      mDelayTimer = mRegisters[opcodeX()];
      break;
    }
    case 0x1E: {
      mI += mRegisters[opcodeX()];
      if (mI >= 0x1000) {
        setVF(1);
      }
      break;
    }
    case 0x0A: {
      // for later
      break;
    }
    case 0x29: {
      mI = FONT_START_ADDRESS + (mRegisters[opcodeX()] * 5);
      break;
    }
    case 0x33: {
      std::vector<uint8_t> digits;
      uint8_t cur = mRegisters[opcodeX()];
      while (cur > 0) {
        digits.push_back(cur % 10);
        cur /= 10;
      }
      while (digits.size() < 3) {
        digits.push_back(0);
      }
      for (int i = digits.size() - 1; i >= 0; i--) {
        mMemory[mI + digits.size() - 1 - i] = digits[i];
      }
      break;
    }
    case 0x55: {
      for (uint8_t i = 0; i <= opcodeX(); i++) {
        mMemory[mI + i] = mRegisters[i];
      }
      break;
    }
    case 0x65: {
      for (uint8_t i = 0; i <= opcodeX(); i++) {
        mRegisters[i] = mMemory[mI + i];
      }
      break;
    }
  }
}

void Chip8::loadROM(std::ifstream &ROM, std::streamsize size) {
  if (!ROM.read(reinterpret_cast<char *>(mMemory.data() + ROM_START_ADDRESS),
                size)) {
    std::cout << "Could no read ROM\n";
    exit(1);
  }
}

void Chip8::loadFont(uint8_t font[], size_t size) {
  memcpy(mMemory.data() + FONT_START_ADDRESS, font, size);
}

void Chip8::updateTimers() {
  if (mDelayTimer > 0) {
    mDelayTimer--;
  }
  if (mSoundTimer > 0) {
    mSoundTimer--;
  }
}

void Chip8::runCycle() {
  mOpcode = (mMemory[mPC] << 8) | mMemory[mPC + 1];
  mPC += 2;

  switch (opcodeType()) {
    case 0x0: {
      execOpcodeType0();
      break;
    }
    case 0x1: {
      execOpcodeType1();
      break;
    }
    case 0x2: {
      execOpcodeType2();
      break;
    }
    case 0x3: {
      execOpcodeType3();
      break;
    }
    case 0x4: {
      execOpcodeType4();
      break;
    }
    case 0x5: {
      execOpcodeType5();
      break;
    }
    case 0x6: {
      execOpcodeType6();
      break;
    }
    case 0x7: {
      execOpcodeType7();
      break;
    }
    case 0x8: {
      execOpcodeType8();
      break;
    }
    case 0x9: {
      execOpcodeType9();
      break;
    }
    case 0xA: {
      execOpcodeTypeA();
      break;
    }
    case 0xB: {
      execOpcodeTypeB();
      break;
    }
    case 0xC: {
      execOpcodeTypeC();
      break;
    }
    case 0xD: {
      execOpcodeTypeD();
      break;
    }
    case 0xE: {
      execOpcodeTypeE();
      break;
    }
    case 0xF: {
      execOpcodeTypeF();
      break;
    }
  }
  mOpcode = std::nullopt;
}
