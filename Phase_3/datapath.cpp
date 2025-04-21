// datapath.cpp
#include "datapath.h"
#include <cstdint>
#include <cassert>

// Execute the core ALU operation. Returns the ALU result or (for JAL/JALR) the link address.
uint32_t executeALU(uint32_t op1,
                    uint32_t op2,
                    uint8_t  opcode,
                    uint8_t  funct3,
                    uint8_t  funct7)
{
    switch (opcode) {
    case 0x37:  // LUI: rd = imm << 12
        return op2;
    case 0x17:  // AUIPC: rd = PC + (imm << 12)
        return op1 + op2;
    case 0x33:  // R‑type
        switch (funct3) {
        case 0x0: // ADD / SUB
            return (funct7 == 0x20) ? (op1 - op2) : (op1 + op2);
        case 0x1: // SLL
            return op1 << (op2 & 0x1F);
        case 0x2: // SLT
            return (int32_t)op1 < (int32_t)op2;
        case 0x3: // SLTU
            return op1 < op2;
        case 0x4: // XOR
            return op1 ^ op2;
        case 0x5: // SRL / SRA
            return (funct7 == 0x20)
                       ? ((int32_t)op1 >> (op2 & 0x1F))
                       : (op1 >> (op2 & 0x1F));
        case 0x6: // OR
            return op1 | op2;
        case 0x7: // AND
            return op1 & op2;
        default:
            assert(false && "Unsupported R‑type funct3");
        }

    case 0x13:  // I‑type ALU
        switch (funct3) {
        case 0x0: // ADDI
            return op1 + op2;
        case 0x1: // SLLI
            return op1 << (op2 & 0x1F);
        case 0x2: // SLTI
            return (int32_t)op1 < (int32_t)op2;
        case 0x3: // SLTIU
            return op1 < op2;
        case 0x4: // XORI
            return op1 ^ op2;
        case 0x5: // SRLI / SRAI
            return (funct7 == 0x20)
                       ? ((int32_t)op1 >> (op2 & 0x1F))
                       : (op1 >> (op2 & 0x1F));
        case 0x6: // ORI
            return op1 | op2;
        case 0x7: // ANDI
            return op1 & op2;
        default:
            assert(false && "Unsupported I‑type funct3");
        }

    case 0x03:  // Load (address calculation)
    case 0x23:  // Store (address calculation)
        return op1 + op2;

    case 0x63:  // Branches: ALU not used for target, handled separately
        return 0;

    case 0x6F:  // JAL: link = PC + 4
    case 0x67:  // JALR: link = PC + 4
        return op1 + 4;

    default:
        // Unsupported opcode — could raise error or log
        return 0;
    }
}

// Evaluate branch condition (BEQ, BNE, BLT, BGE, BLTU, BGEU)
bool evaluateBranch(uint32_t op1,
                    uint32_t op2,
                    uint8_t  funct3)
{
    switch (funct3) {
    case 0x0: return op1 == op2;                   // BEQ
    case 0x1: return op1 != op2;                   // BNE
    case 0x4: return (int32_t)op1 < (int32_t)op2;  // BLT
    case 0x5: return (int32_t)op1 >= (int32_t)op2; // BGE
    case 0x6: return op1 < op2;                    // BLTU
    case 0x7: return op1 >= op2;                   // BGEU
    default:  return false;                        // Undefined branch — default not taken
    }
}

// Compute branch target: PC + sign-extended B-type immediate
uint32_t computeBranchTarget(uint32_t pc, int32_t imm)
{
    return pc + imm;
}

// Compute jump target for JALR: (op1 + imm) & ~1
uint32_t computeJALRTarget(uint32_t op1, int32_t imm)
{
    return (op1 + imm) & ~1;
}
