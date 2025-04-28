


//pipeline_registers.h
#ifndef PIPELINE_REGISTERS_H
#define PIPELINE_REGISTERS_H

#include <cstdint>
#include <string>

// Represents one instruction word + optional asm.
struct Instruction {
    uint32_t binary;
    std::string asmStr;
};

// IF → ID
struct IF_ID {
    Instruction instr;
    uint32_t    pc;
    bool        valid;
};

// ID → EX
struct ID_EX {
    uint32_t    pc;
    Instruction instr;
    uint8_t     opcode;
    uint8_t     rs1;
    uint8_t     rs2;
    uint8_t     rd;
    int32_t     imm;

    // **New**: the register‐read data
    uint32_t    readData1;
    uint32_t    readData2;

    // control signals
    bool        regWrite;
    bool        memRead;
    bool        memWrite;
    bool        branch;
    bool        aluSrc;

    bool        valid;
};

// EX → MEM
struct EX_MEM {
    uint32_t    pc;
    Instruction instr;
    uint32_t    aluResult;
    uint32_t    writeData;  // for stores
    uint8_t     rd;
    int32_t     imm;           // the same immediate that ID/EX had
    uint32_t    branchTarget;

    // control signals
    bool        regWrite;
    bool        memRead;
    bool        memWrite;
    bool        branch;
    bool        valid;
};

// MEM → WB
struct MEM_WB {
    uint32_t    pc;
    Instruction instr;
    uint32_t    memData;    // loaded from memory
    uint32_t    aluResult;
    uint8_t     rd;

    // control signals
    bool        regWrite;
    bool        memRead;    // **New**: so we know which data to write back

    bool        valid;
};

#endif // PIPELINE_REGISTERS_H
