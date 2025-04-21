#include <cstdint>

const uint8_t INVALID_REG = 0xFF;        // sentinel that can never equal a real register

struct StageReg {
    bool     valid = false;
    uint32_t instr = 0;
    uint32_t pc    = 0;
    uint8_t  rd    = INVALID_REG;
    uint8_t  rs1   = INVALID_REG;
    uint8_t  rs2   = INVALID_REG;
};

