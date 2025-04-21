#pragma GCC diagnostic ignored "-Woverflow"

struct ISAFields {
    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7;
    char     fmt;        // 'R', 'I', 'S', 'B', 'U', 'J'
};

const unordered_map<string, ISAFields> isaMap = {
    /* R‑type */
    {"add",  {0b0110011, 0b000, 0b0000000, 'R'}},
    {"sub",  {0b0110011, 0b000, 0b0100000, 'R'}},
    /* I‑type */
    {"addi", {0b0010011, 0b000, 0,          'I'}},
    {"lw",   {0b0000011, 0b010, 0,          'I'}},
    {"jalr", {0b1100111, 0b000, 0,          'I'}},
    /* S‑type */
    {"sw",   {0b0100011, 0b010, 0,          'S'}},
    /* B‑type */
    {"beq",  {0b1100011, 0b000, 0,          'B'}},
    /* J‑type */
    {"jal",  {0b1101111, 0,      0,         'J'}},
    /* ECALL (special) */
    {"ecall",{0b1110011, 0,      0,         'I'}}
};

