// control_path.cpp

#include "control_path.h"
#include <iostream>

// Generate the five control signals: 
//   regWrite, memRead, memWrite, branch, aluSrc
ControlSignals generateControlSignals(uint8_t opcode) {
    ControlSignals ctrl{ false, false, false, false, false };

    switch (opcode) {
        case 0x33:  // R‑type (ADD, SUB, SLL, SLT, etc.)
            ctrl.regWrite = true;
            break;

        case 0x13:  // I‑type ALU (ADDI, SLLI, XORI, etc.)
            ctrl.regWrite = true;
            ctrl.aluSrc   = true;
            break;

        case 0x03:  // Load (LB, LH, LW, etc.)
            ctrl.memRead  = true;
            ctrl.regWrite = true;
            ctrl.aluSrc   = true;
            break;

        case 0x23:  // Store (SB, SH, SW)
            ctrl.memWrite = true;
            ctrl.aluSrc   = true;
            break;

        case 0x63:  // Branch (BEQ, BNE, BLT, BGE, BLTU, BGEU)
            ctrl.branch   = true;
            break;

        case 0x6F:  // JAL (unconditional jump + link)
            ctrl.regWrite = true;  // write PC+4 to rd
            ctrl.branch   = true;  // update PC to target
            break;

        case 0x67:  // JALR (unconditional jump reg + link)
            ctrl.regWrite = true;  // write PC+4 to rd
            ctrl.branch   = true;  // update PC to (rs1 + imm) & ~1
            ctrl.aluSrc   = true;  // use immediate for target calculation
            break;

        case 0x17:  // AUIPC (PC‑relative immediate)
            ctrl.regWrite = true;
            ctrl.aluSrc   = true;
            break;

        case 0x37:  // LUI (load upper immediate)
            ctrl.regWrite = true;
            ctrl.aluSrc   = true;
            break;

        default:
            // all signals false → NOP
            break;
    }
    //print
    // std::cout << "Control Signals: "
    //           << "regWrite=" << ctrl.regWrite << ", "
    //           << "memRead="  << ctrl.memRead  << ", "
    //           << "memWrite=" << ctrl.memWrite << ", "
    //           << "branch="   << ctrl.branch   << ", "
    //           << "aluSrc="   << ctrl.aluSrc   << "\n";

    return ctrl;
}