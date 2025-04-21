//datapath.h
#ifndef DATAPATH_H
#define DATAPATH_H

#include <cstdint>

// Execute the ALU operation based on opcode.
// For R‑type and I‑type add/sub, load/store address calc, etc.
uint32_t executeALU(uint32_t operand1,
                    uint32_t operand2,
                    uint8_t  opcode,
                    uint8_t  funct3 = 0,
                    uint8_t  funct7 = 0);

// Evaluate a branch decision (e.g. BEQ).
bool evaluateBranch(uint32_t operand1,
                    uint32_t operand2,
                    uint8_t  funct3 = 0);

#endif // DATAPATH_H
