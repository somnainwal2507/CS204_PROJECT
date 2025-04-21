//register_file.h
#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H

#include <array>
#include <cstdint>

class RegisterFile {
public:
    RegisterFile();

    // Read the value of register x[reg].  x0 always reads 0.
    uint32_t read(uint8_t reg) const;

    // Write value into register x[reg].  Write to x0 is ignored.
    void write(uint8_t reg, uint32_t value);

    // For debugging: dump all 32 registers to stdout.
    void dump() const;

private:
    std::array<uint32_t, 32> regs;
};

#endif // REGISTER_FILE_H
