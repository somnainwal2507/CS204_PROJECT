#ifndef CONTROL_PATH_H
#define CONTROL_PATH_H

#include <cstdint>

// Control signals for a single instruction.
struct ControlSignals {
    bool regWrite;
    bool memRead;
    bool memWrite;
    bool branch;
    bool aluSrc;
};

ControlSignals generateControlSignals(uint8_t opcode);

#endif // CONTROL_PATH_H