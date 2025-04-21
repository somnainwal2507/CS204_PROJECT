//register_file.cpp
#include "register_file.h"
#include <iostream>

RegisterFile::RegisterFile() {
    regs.fill(0);
}

uint32_t RegisterFile::read(uint8_t reg) const {
    // x0 is hardwired to zero
    if (reg == 0) return 0;
    return regs[reg];
}

void RegisterFile::write(uint8_t reg, uint32_t value) {
    // ignore writes to x0
    if (reg == 0) return;
    regs[reg] = value;
}

void RegisterFile::dump() const {
    std::cout << "=== Register File Dump ===\n";
    for (int i = 0; i < 32; ++i) {
        std::cout << "x" << i << ": 0x"
                  << std::hex << regs[i] << std::dec << "\n";
    }
    std::cout << "==========================\n";
}
