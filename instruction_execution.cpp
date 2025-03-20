#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cstdlib>

using namespace std;

struct Simulator {
    uint32_t PC;
    uint32_t IR;
    uint32_t registers[32];
    unordered_map<uint32_t, uint8_t> dataMemory;
    unordered_map<uint32_t, uint32_t> instrMemory;
    uint32_t RM;
    uint32_t RY;
    uint32_t clock;
    ofstream log;

    Simulator() : PC(0), IR(0), RM(0), RY(0), clock(1) {
        for (int i = 0; i < 32; i++) {
            registers[i] = 0;
        }
    }
};

Simulator sim;

string toHex(uint32_t num) {
    stringstream ss;
    ss << setw(8) << setfill('0') << hex << uppercase << num;
    return ss.str();
}

void logMessage(const string &msg) {
    sim.log << msg << endl;
}

void logRegisterStates() {
    stringstream ss;
    ss << "Register States:" << endl;
    for (int i = 0; i < 32; i++) {
        ss << "x" << i << " = 0x" << toHex(sim.registers[i]);
        if ((i + 1) % 4 == 0)
            ss << endl;
        else
            ss << "\t";
    }
    logMessage(ss.str());
}

void fetch() {
    sim.IR = sim.instrMemory[sim.PC];
    logMessage("Fetch: PC = 0x" + toHex(sim.PC) + ", IR = 0x" + toHex(sim.IR));
}

void decode() {
    uint32_t opcode = sim.IR & 0x7F;
    logMessage("Decode: IR = 0x" + toHex(sim.IR) + ", Opcode = 0x" + toHex(opcode));
}

void execute() {
    uint32_t opcode  = sim.IR & 0x7F;
    uint32_t rd      = (sim.IR >> 7)  & 0x1F;
    uint32_t funct3  = (sim.IR >> 12) & 0x7;
    uint32_t rs1     = (sim.IR >> 15) & 0x1F;
    uint32_t rs2     = (sim.IR >> 20) & 0x1F;
    uint32_t funct7  = (sim.IR >> 25) & 0x7F;
    int32_t imm_i  = ((int32_t)sim.IR) >> 20;
    int32_t imm_s  = (((sim.IR >> 25) & 0x7F) << 5) | ((sim.IR >> 7) & 0x1F);
    if (imm_s & 0x800) { imm_s |= 0xFFFFF000; }
    int32_t imm_b = (((sim.IR >> 31) & 0x1) << 12) |
                    (((sim.IR >> 7)  & 0x1) << 11) |
                    (((sim.IR >> 25) & 0x3F) << 5) |
                    (((sim.IR >> 8)  & 0xF) << 1);
    if (imm_b & 0x1000) { imm_b |= 0xFFFFE000; }
    int32_t imm_u = sim.IR & 0xFFFFF000;
    int32_t imm_j = (((sim.IR >> 31) & 0x1) << 20) |
                    (((sim.IR >> 12) & 0xFF) << 12) |
                    (((sim.IR >> 20) & 0x1) << 11) |
                    (((sim.IR >> 21) & 0x3FF) << 1);
    if (imm_j & 0x100000) { imm_j |= 0xFFF00000; }

    uint32_t currentPC = sim.PC;
    sim.PC += 4;

    switch (opcode) {
        case 0x33:
            if (funct7 == 0x01 && funct3 == 0x0) { // mul
                sim.RM = sim.registers[rs1] * sim.registers[rs2];
                logMessage("Execute: mul x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " * 0x" + toHex(sim.registers[rs2]));
            } else if (funct7 == 0x01 && funct3 == 0x4) { // div
                sim.RM = (sim.registers[rs2] != 0) ? (sim.registers[rs1] / sim.registers[rs2]) : 0;
                logMessage("Execute: div x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " / 0x" + toHex(sim.registers[rs2]));
            } else if (funct7 == 0x01 && funct3 == 0x6) { // rem
                sim.RM = (sim.registers[rs2] != 0) ? (sim.registers[rs1] % sim.registers[rs2]) : 0;
                logMessage("Execute: rem x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " % 0x" + toHex(sim.registers[rs2]));
            } else if (funct3 == 0x0 && funct7 == 0x00) { // add
                sim.RM = sim.registers[rs1] + sim.registers[rs2];
                logMessage("Execute: add x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " + 0x" + toHex(sim.registers[rs2]));
            } else if (funct3 == 0x0 && funct7 == 0x20) { // sub
                sim.RM = sim.registers[rs1] - sim.registers[rs2];
                logMessage("Execute: sub x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " - 0x" + toHex(sim.registers[rs2]));
            } else if (funct3 == 0x7) { // and
                sim.RM = sim.registers[rs1] & sim.registers[rs2];
                logMessage("Execute: and x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " & 0x" + toHex(sim.registers[rs2]));
            } else if (funct3 == 0x6) { // or
                sim.RM = sim.registers[rs1] | sim.registers[rs2];
                logMessage("Execute: or x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " | 0x" + toHex(sim.registers[rs2]));
            } else if (funct3 == 0x1) { // sll
                sim.RM = sim.registers[rs1] << (sim.registers[rs2] & 0x1F);
                logMessage("Execute: sll x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " << (0x" + toHex(sim.registers[rs2]) + ")");
            } else if (funct3 == 0x2) { // slt
                sim.RM = ((int32_t)sim.registers[rs1] < (int32_t)sim.registers[rs2]) ? 1 : 0;
                logMessage("Execute: slt x" + to_string(rd) + " = (0x" + toHex(sim.registers[rs1]) +
                           " < 0x" + toHex(sim.registers[rs2]) + ")");
            } else if (funct3 == 0x5 && funct7 == 0x20) { // sra
                sim.RM = ((int32_t)sim.registers[rs1]) >> (sim.registers[rs2] & 0x1F);
                logMessage("Execute: sra x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " >>_arith (0x" + toHex(sim.registers[rs2]) + ")");
            } else if (funct3 == 0x5 && funct7 == 0x00) { // srl
                sim.RM = sim.registers[rs1] >> (sim.registers[rs2] & 0x1F);
                logMessage("Execute: srl x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " >> (0x" + toHex(sim.registers[rs2]) + ")");
            } else if (funct3 == 0x4) { // xor
                sim.RM = sim.registers[rs1] ^ sim.registers[rs2];
                logMessage("Execute: xor x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " ^ 0x" + toHex(sim.registers[rs2]));
            }
            break;

        case 0x13:
            if (funct3 == 0x0) { // addi
                sim.RM = sim.registers[rs1] + imm_i;
                logMessage("Execute: addi x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " + 0x" + toHex(imm_i));
            } else if (funct3 == 0x7) { // andi
                sim.RM = sim.registers[rs1] & imm_i;
                logMessage("Execute: andi x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " & 0x" + toHex(imm_i));
            } else if (funct3 == 0x6) { // ori
                sim.RM = sim.registers[rs1] | imm_i;
                logMessage("Execute: ori x" + to_string(rd) + " = 0x" + toHex(sim.registers[rs1]) +
                           " | 0x" + toHex(imm_i));
            }
            break;

        case 0x03:
            sim.RY = sim.registers[rs1] + imm_i;
            logMessage("Execute: Load effective address = 0x" + toHex(sim.registers[rs1]) +
                       " + 0x" + toHex(imm_i) + " = 0x" + toHex(sim.RY));
            break;

        case 0x23:
            sim.RY = sim.registers[rs1] + imm_s;
            logMessage("Execute: Store effective address = 0x" + toHex(sim.registers[rs1]) +
                       " + 0x" + toHex(imm_s) + " = 0x" + toHex(sim.RY));
            break;

        case 0x63: {
            logMessage("Execute: Branch evaluation for 0x" + toHex(sim.registers[rs1]) +
                       " and 0x" + toHex(sim.registers[rs2]));
            bool takeBranch = false;
            if (funct3 == 0x0) {
                takeBranch = (sim.registers[rs1] == sim.registers[rs2]);
                logMessage("beq: " + string(takeBranch ? "taken" : "not taken"));
            } else if (funct3 == 0x1) {
                takeBranch = (sim.registers[rs1] != sim.registers[rs2]);
                logMessage("bne: " + string(takeBranch ? "taken" : "not taken"));
            } else if (funct3 == 0x4) {
                takeBranch = ((int32_t)sim.registers[rs1] < (int32_t)sim.registers[rs2]);
                logMessage("blt: " + string(takeBranch ? "taken" : "not taken"));
            } else if (funct3 == 0x5) {
                takeBranch = ((int32_t)sim.registers[rs1] >= (int32_t)sim.registers[rs2]);
                logMessage("bge: " + string(takeBranch ? "taken" : "not taken"));
            }
            if (takeBranch) {
                sim.PC = currentPC + imm_b;
                logMessage("Branch taken. New PC = 0x" + toHex(sim.PC));
            }
            break;
        }

        case 0x67: {
            uint32_t returnAddr = sim.PC;
            sim.PC = (sim.registers[rs1] + imm_i) & ~1;
            logMessage("Execute: jalr x" + to_string(rd) + ", 0x" + toHex(sim.registers[rs1]) +
                       ", 0x" + toHex(imm_i) + " => New PC = 0x" + toHex(sim.PC));
            sim.RM = returnAddr;
            break;
        }

        case 0x37:
            sim.RM = imm_u;
            logMessage("Execute: lui x" + to_string(rd) + " = 0x" + toHex(imm_u));
            break;

        case 0x17:
            sim.RM = currentPC + imm_u;
            logMessage("Execute: auipc x" + to_string(rd) + " = PC + 0x" +
                       toHex(imm_u) + " = 0x" + toHex(sim.RM));
            break;

        case 0x6F: {
            uint32_t returnAddr = sim.PC;
            sim.PC = currentPC + imm_j;
            logMessage("Execute: jal x" + to_string(rd) + " => New PC = 0x" + toHex(sim.PC));
            sim.RM = returnAddr;
            break;
        }

        default:
            logMessage("Execute: Unknown opcode 0x" + toHex(opcode));
            break;
    }
}

void memoryAccess() {
    uint32_t opcode = sim.IR & 0x7F;
    uint32_t funct3 = (sim.IR >> 12) & 0x7;
    if (opcode == 0x03) { // Load instructions
        uint32_t addr = sim.RY;
        if (funct3 == 0x0) { // lb
            sim.RM = sim.dataMemory[addr];
            logMessage("Memory Access: lb from address 0x" + toHex(addr) +
                       " => 0x" + toHex(sim.RM));
        } else if (funct3 == 0x1) { // lh
            uint16_t val = sim.dataMemory[addr] | (sim.dataMemory[addr + 1] << 8);
            sim.RM = val;
            logMessage("Memory Access: lh from address 0x" + toHex(addr) +
                       " => 0x" + toHex(sim.RM));
        } else if (funct3 == 0x2) { // lw
            uint32_t val = sim.dataMemory[addr] |
                           (sim.dataMemory[addr + 1] << 8) |
                           (sim.dataMemory[addr + 2] << 16) |
                           (sim.dataMemory[addr + 3] << 24);
            sim.RM = val;
            logMessage("Memory Access: lw from address 0x" + toHex(addr) +
                       " => 0x" + toHex(sim.RM));
        } else if (funct3 == 0x3) { // ld (simulate as lw)
            uint32_t val = sim.dataMemory[addr] |
                           (sim.dataMemory[addr + 1] << 8) |
                           (sim.dataMemory[addr + 2] << 16) |
                           (sim.dataMemory[addr + 3] << 24);
            sim.RM = val;
            logMessage("Memory Access: ld from address 0x" + toHex(addr) +
                       " => 0x" + toHex(sim.RM));
        }
    }
    else if (opcode == 0x23) { // Store instructions
        uint32_t addr = sim.RY;
        uint32_t rs2 = (sim.IR >> 20) & 0x1F;
        uint32_t value = sim.registers[rs2];
        if (funct3 == 0x0) { // sb
            sim.dataMemory[addr] = value & 0xFF;
            logMessage("Memory Access: sb store 0x" + toHex(value & 0xFF) +
                       " to address 0x" + toHex(addr));
        } else if (funct3 == 0x1) { // sh
            sim.dataMemory[addr] = value & 0xFF;
            sim.dataMemory[addr + 1] = (value >> 8) & 0xFF;
            logMessage("Memory Access: sh store 0x" + toHex(value) +
                       " to addresses 0x" + toHex(addr) + " and 0x" + toHex(addr + 1));
        } else if (funct3 == 0x2) { // sw
            sim.dataMemory[addr] = value & 0xFF;
            sim.dataMemory[addr + 1] = (value >> 8) & 0xFF;
            sim.dataMemory[addr + 2] = (value >> 16) & 0xFF;
            sim.dataMemory[addr + 3] = (value >> 24) & 0xFF;
            logMessage("Memory Access: sw store 0x" + toHex(value) +
                       " to addresses starting at 0x" + toHex(addr));
        } else if (funct3 == 0x3) { // sd (simulate as sw)
            sim.dataMemory[addr] = value & 0xFF;
            sim.dataMemory[addr + 1] = (value >> 8) & 0xFF;
            sim.dataMemory[addr + 2] = (value >> 16) & 0xFF;
            sim.dataMemory[addr + 3] = (value >> 24) & 0xFF;
            logMessage("Memory Access: sd store 0x" + toHex(value) +
                       " to addresses starting at 0x" + toHex(addr));
        }
    }
}

void writeBack() {
    uint32_t opcode = sim.IR & 0x7F;
    uint32_t rd = (sim.IR >> 7) & 0x1F;
    if (opcode == 0x33 || opcode == 0x13 || opcode == 0x67 ||
        opcode == 0x37 || opcode == 0x17 || opcode == 0x6F) {
        if (rd != 0) {
            sim.registers[rd] = sim.RM;
            logMessage("WriteBack: x" + to_string(rd) + " = 0x" + toHex(sim.RM));
        } else {
            logMessage("WriteBack: x0 remains 0");
        }
    }
    if (opcode == 0x03) {
        if (rd != 0) {
            sim.registers[rd] = sim.RM;
            logMessage("WriteBack: x" + to_string(rd) + " = 0x" + toHex(sim.RM) + " (loaded)");
        }
    }
}

void simulate() {
    while (true) {
        if (sim.instrMemory.find(sim.PC) == sim.instrMemory.end()) {
            logMessage("No instruction found at PC = 0x" + toHex(sim.PC) + ". Halting.");
            break;
        }
        fetch();
        decode();
        execute();
        memoryAccess();
        writeBack();
        logRegisterStates();
        logMessage("Clock Cycle: 0x" + toHex(sim.clock));
        sim.clock++;
    }
}

int main() {
    sim.log.open("simulator_log.txt");
    if (!sim.log.is_open()) {
        cerr << "Error opening simulator log file." << endl;
        return EXIT_FAILURE;
    }

    ifstream infile("output.mc");
    if (!infile.is_open()) {
        sim.log << "Error: Could not open output.mc file." << endl;
        return EXIT_FAILURE;
    }

    string line;
    bool inDataSegment = false;
    while (getline(infile, line)) {
        if (line.find("Data Segment") != string::npos) {
            inDataSegment = true;
            continue;
        }
        if (line.empty())
            continue;

        stringstream ss(line);
        string addrStr, valueStr;
        ss >> addrStr >> valueStr;
        if (!valueStr.empty() && valueStr.back() == ',') {
            valueStr.pop_back();
        }
        uint32_t addr = stoul(addrStr, nullptr, 16);
        if (!inDataSegment) {
            uint32_t machineCode = stoul(valueStr, nullptr, 16);
            sim.instrMemory[addr] = machineCode;
        } else {
            uint8_t data = static_cast<uint8_t>(stoul(valueStr, nullptr, 16));
            sim.dataMemory[addr] = data;
        }
    }
    infile.close();
    
    sim.registers[2]  = 0x7FFFFFDC;
    sim.registers[11] = 0x7FFFFFDC;
    sim.registers[10] = 0x00000001;
    sim.registers[3]  = 0x10000000;

    simulate();

    ofstream dataOut("final_data.mc");
    if (!dataOut.is_open()) {
        sim.log << "Error: Could not open final_data.mc for writing." << endl;
        return EXIT_FAILURE;
    }
    for (const auto &pair : sim.dataMemory) {
        dataOut << "0x" << toHex(pair.first) << " 0x"
                << setw(2) << setfill('0') << hex << uppercase
                << static_cast<unsigned int>(pair.second) << dec << endl;
    }
    dataOut.close();
    sim.log.close();
    return EXIT_SUCCESS;
}
