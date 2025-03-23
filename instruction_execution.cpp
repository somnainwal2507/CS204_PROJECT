#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>
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
// We still store the comment for potential future use, but we won't print it.
unordered_map<uint32_t, string> instrComments;
// Global vector to record memory change events per cycle.
vector<string> memChangeLog;

string toHex(uint32_t num) {
    stringstream ss;
    ss << setw(8) << setfill('0') << hex << uppercase << num;
    return ss.str();
}

// Helper function to format a byte (two-digit hex)
string toHexByte(uint8_t byte) {
    stringstream ss;
    ss << setw(2) << setfill('0') << hex << uppercase << static_cast<unsigned int>(byte);
    return ss.str();
}

void logMessage(const string &msg) {
    sim.log << msg << endl;
}

void logRegisterStates() {
    stringstream ss;
    ss << "-----------------------------" << endl;
    ss << "Register States:" << endl;
    for (int i = 0; i < 32; i++) {
        ss << "x" << i << " = 0x" << toHex(sim.registers[i]);
        if ((i + 1) % 4 == 0)
            ss << endl;
        else
            ss << "\t";
    }
    ss << "-----------------------------";
    logMessage(ss.str());
}

void fetch() {
    sim.IR = sim.instrMemory[sim.PC];
    logMessage("\n==== Fetch Stage ====");
    logMessage("PC: 0x" + toHex(sim.PC) + "  |  Fetched IR: 0x" + toHex(sim.IR));
}

void decode() {
    uint32_t opcode = sim.IR & 0x7F;
    uint32_t rd     = (sim.IR >> 7)  & 0x1F;
    uint32_t funct3 = (sim.IR >> 12) & 0x7;
    uint32_t rs1    = (sim.IR >> 15) & 0x1F;
    uint32_t rs2    = (sim.IR >> 20) & 0x1F;
    uint32_t funct7 = (sim.IR >> 25) & 0x7F;

    // Compute immediate values
    int32_t imm_i = ((int32_t)sim.IR) >> 20;
    int32_t imm_s = (((sim.IR >> 25) & 0x7F) << 5) | ((sim.IR >> 7) & 0x1F);
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

    // Build a presentable output for the decode stage
    stringstream ss;
    ss << "\n==== Decode Stage ====" << endl;
    ss << "Instruction (IR): 0x" << toHex(sim.IR) << endl;
    ss << "Opcode: 0x" << toHex(opcode) << endl;

    if (opcode == 0x33) { // R-type instructions
        ss << "Type: R-Type" << endl;
        ss << "rd: x" << rd << "    rs1: x" << rs1 << "    rs2: x" << rs2 << endl;
        ss << "funct3: 0x" << toHex(funct3) << "    funct7: 0x" << toHex(funct7);
    } else if (opcode == 0x03 || opcode == 0x13 || opcode == 0x67) { // I-type instructions
        ss << "Type: I-Type" << endl;
        ss << "rd: x" << rd << "    rs1: x" << rs1 << endl;
        ss << "funct3: 0x" << toHex(funct3) << "    Immediate: 0x" << toHex(imm_i);
    } else if (opcode == 0x23) { // S-type instructions
        ss << "Type: S-Type" << endl;
        ss << "rs1: x" << rs1 << "    rs2: x" << rs2 << endl;
        ss << "funct3: 0x" << toHex(funct3) << "    Immediate: 0x" << toHex(imm_s);
    } else if (opcode == 0x63) { // B-type instructions
        ss << "Type: B-Type" << endl;
        ss << "rs1: x" << rs1 << "    rs2: x" << rs2 << endl;
        ss << "funct3: 0x" << toHex(funct3) << "    Immediate: 0x" << toHex(imm_b);
    } else if (opcode == 0x37 || opcode == 0x17) { // U-type instructions
        ss << "Type: U-Type" << endl;
        ss << "rd: x" << rd << endl;
        ss << "Immediate: 0x" << toHex(imm_u);
    } else if (opcode == 0x6F) { // J-type instructions
        ss << "Type: J-Type" << endl;
        ss << "rd: x" << rd << endl;
        ss << "Immediate: 0x" << toHex(imm_j);
    } else {
        ss << "Unknown instruction format.";
    }
    ss << "\n-----------------------------";
    logMessage(ss.str());
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

    stringstream ss;
    ss << "\n==== Execute Stage ====" << endl;
    switch (opcode) {
        case 0x33:
            if (funct7 == 0x01 && funct3 == 0x0) { // mul
                sim.RM = sim.registers[rs1] * sim.registers[rs2];
                ss << "mul: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " * 0x" << toHex(sim.registers[rs2]);
            } else if (funct7 == 0x01 && funct3 == 0x4) { // div
                sim.RM = (sim.registers[rs2] != 0) ? (sim.registers[rs1] / sim.registers[rs2]) : 0;
                ss << "div: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " / 0x" << toHex(sim.registers[rs2]);
            } else if (funct7 == 0x01 && funct3 == 0x6) { // rem
                sim.RM = (sim.registers[rs2] != 0) ? (sim.registers[rs1] % sim.registers[rs2]) : 0;
                ss << "rem: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " % 0x" << toHex(sim.registers[rs2]);
            } else if (funct3 == 0x0 && funct7 == 0x00) { // add
                sim.RM = sim.registers[rs1] + sim.registers[rs2];
                ss << "add: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " + 0x" << toHex(sim.registers[rs2]);
            } else if (funct3 == 0x0 && funct7 == 0x20) { // sub
                sim.RM = sim.registers[rs1] - sim.registers[rs2];
                ss << "sub: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " - 0x" << toHex(sim.registers[rs2]);
            } else if (funct3 == 0x7) { // and
                sim.RM = sim.registers[rs1] & sim.registers[rs2];
                ss << "and: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " & 0x" << toHex(sim.registers[rs2]);
            } else if (funct3 == 0x6) { // or
                sim.RM = sim.registers[rs1] | sim.registers[rs2];
                ss << "or: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " | 0x" << toHex(sim.registers[rs2]);
            } else if (funct3 == 0x1) { // sll
                sim.RM = sim.registers[rs1] << (sim.registers[rs2] & 0x1F);
                ss << "sll: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " << (0x" << toHex(sim.registers[rs2]) << ")";
            } else if (funct3 == 0x2) { // slt
                sim.RM = ((int32_t)sim.registers[rs1] < (int32_t)sim.registers[rs2]) ? 1 : 0;
                ss << "slt: x" << rd << " = (0x" << toHex(sim.registers[rs1])
                   << " < 0x" << toHex(sim.registers[rs2]) << ")";
            } else if (funct3 == 0x5 && funct7 == 0x20) { // sra
                sim.RM = ((int32_t)sim.registers[rs1]) >> (sim.registers[rs2] & 0x1F);
                ss << "sra: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " >>_arith (0x" << toHex(sim.registers[rs2]) << ")";
            } else if (funct3 == 0x5 && funct7 == 0x00) { // srl
                sim.RM = sim.registers[rs1] >> (sim.registers[rs2] & 0x1F);
                ss << "srl: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " >> (0x" << toHex(sim.registers[rs2]) << ")";
            } else if (funct3 == 0x4) { // xor
                sim.RM = sim.registers[rs1] ^ sim.registers[rs2];
                ss << "xor: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " ^ 0x" << toHex(sim.registers[rs2]);
            }
            break;

        case 0x13:
            if (funct3 == 0x0) { // addi
                sim.RM = sim.registers[rs1] + imm_i;
                ss << "addi: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " + 0x" << toHex(imm_i);
            } else if (funct3 == 0x7) { // andi
                sim.RM = sim.registers[rs1] & imm_i;
                ss << "andi: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " & 0x" << toHex(imm_i);
            } else if (funct3 == 0x6) { // ori
                sim.RM = sim.registers[rs1] | imm_i;
                ss << "ori: x" << rd << " = 0x" << toHex(sim.registers[rs1])
                   << " | 0x" << toHex(imm_i);
            }
            break;

        case 0x03:
            sim.RY = sim.registers[rs1] + imm_i;
            ss << "Load Effective Address: 0x" << toHex(sim.registers[rs1])
               << " + 0x" << toHex(imm_i) << " = 0x" << toHex(sim.RY);
            break;

        case 0x23:
            sim.RY = sim.registers[rs1] + imm_s;
            ss << "Store Effective Address: 0x" << toHex(sim.registers[rs1])
               << " + 0x" << toHex(imm_s) << " = 0x" << toHex(sim.RY);
            break;

        case 0x63:
            ss << "Branch Evaluation: comparing 0x" << toHex(sim.registers[rs1])
               << " and 0x" << toHex(sim.registers[rs2]);
            {
                bool takeBranch = false;
                if (funct3 == 0x0) {
                    takeBranch = (sim.registers[rs1] == sim.registers[rs2]);
                    ss << "\nbeq: " << (takeBranch ? "taken" : "not taken");
                } else if (funct3 == 0x1) {
                    takeBranch = (sim.registers[rs1] != sim.registers[rs2]);
                    ss << "\nbne: " << (takeBranch ? "taken" : "not taken");
                } else if (funct3 == 0x4) {
                    takeBranch = ((int32_t)sim.registers[rs1] < (int32_t)sim.registers[rs2]);
                    ss << "\nblt: " << (takeBranch ? "taken" : "not taken");
                } else if (funct3 == 0x5) {
                    takeBranch = ((int32_t)sim.registers[rs1] >= (int32_t)sim.registers[rs2]);
                    ss << "\nbge: " << (takeBranch ? "taken" : "not taken");
                }
                if (takeBranch) {
                    sim.PC = currentPC + imm_b;
                    ss << "\nBranch taken. New PC: 0x" << toHex(sim.PC);
                }
            }
            break;

        case 0x67:
            {
                uint32_t returnAddr = sim.PC;
                sim.PC = (sim.registers[rs1] + imm_i) & ~1;
                ss << "jalr: x" << rd << ", 0x" << toHex(sim.registers[rs1])
                   << ", 0x" << toHex(imm_i) << " => New PC: 0x" << toHex(sim.PC);
                sim.RM = returnAddr;
            }
            break;

        case 0x37:
            sim.RM = imm_u;
            ss << "lui: x" << rd << " = 0x" << toHex(imm_u);
            break;

        case 0x17:
            sim.RM = currentPC + imm_u;
            ss << "auipc: x" << rd << " = PC + 0x" << toHex(imm_u)
               << " = 0x" << toHex(sim.RM);
            break;

        case 0x6F:
            {
                uint32_t returnAddr = sim.PC;
                sim.PC = currentPC + imm_j;
                ss << "jal: x" << rd << " => New PC: 0x" << toHex(sim.PC);
                sim.RM = returnAddr;
            }
            break;

        default:
            ss << "Unknown opcode: 0x" << toHex(opcode);
            break;
    }
    ss << "\n-----------------------------";
    logMessage(ss.str());
}

void memoryAccess() {
    uint32_t opcode = sim.IR & 0x7F;
    uint32_t funct3 = (sim.IR >> 12) & 0x7;
    stringstream ss;
    ss << "\n==== Memory Access Stage ====" << endl;
    if (opcode == 0x03) { // Load instructions
        uint32_t addr = sim.RY;
        if (funct3 == 0x0) { // lb
            sim.RM = sim.dataMemory[addr];
            ss << "lb from address 0x" << toHex(addr)
               << " => 0x" << toHex(sim.RM);
        } else if (funct3 == 0x1) { // lh
            uint16_t val = sim.dataMemory[addr] | (sim.dataMemory[addr + 1] << 8);
            sim.RM = val;
            ss << "lh from address 0x" << toHex(addr)
               << " => 0x" << toHex(sim.RM);
        } else if (funct3 == 0x2) { // lw
            uint32_t val = sim.dataMemory[addr] |
                           (sim.dataMemory[addr + 1] << 8) |
                           (sim.dataMemory[addr + 2] << 16) |
                           (sim.dataMemory[addr + 3] << 24);
            sim.RM = val;
            ss << "lw from address 0x" << toHex(addr)
               << " => 0x" << toHex(sim.RM);
        } else if (funct3 == 0x3) { // ld (simulate as lw)
            uint32_t val = sim.dataMemory[addr] |
                           (sim.dataMemory[addr + 1] << 8) |
                           (sim.dataMemory[addr + 2] << 16) |
                           (sim.dataMemory[addr + 3] << 24);
            sim.RM = val;
            ss << "ld from address 0x" << toHex(addr)
               << " => 0x" << toHex(sim.RM);
        }
    }
    else if (opcode == 0x23) { // Store instructions
        uint32_t addr = sim.RY;
        uint32_t rs2 = (sim.IR >> 20) & 0x1F;
        uint32_t value = sim.registers[rs2];
        if (funct3 == 0x0) { // sb
            sim.dataMemory[addr] = value & 0xFF;
            ss << "sb store 0x" << toHex(value & 0xFF)
               << " to address 0x" << toHex(addr);
            // Record memory change event:
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr) << "  0x" << toHexByte(value & 0xFF);
                memChangeLog.push_back(event.str());
            }
        } else if (funct3 == 0x1) { // sh
            sim.dataMemory[addr] = value & 0xFF;
            sim.dataMemory[addr + 1] = (value >> 8) & 0xFF;
            ss << "sh store 0x" << toHex(value)
               << " to addresses 0x" << toHex(addr)
               << " and 0x" << toHex(addr + 1);
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr) << "  0x" << toHexByte(value & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 1) << "  0x" << toHexByte((value >> 8) & 0xFF);
                memChangeLog.push_back(event.str());
            }
        } else if (funct3 == 0x2) { // sw
            sim.dataMemory[addr] = value & 0xFF;
            sim.dataMemory[addr + 1] = (value >> 8) & 0xFF;
            sim.dataMemory[addr + 2] = (value >> 16) & 0xFF;
            sim.dataMemory[addr + 3] = (value >> 24) & 0xFF;
            ss << "sw store 0x" << toHex(value)
               << " to addresses starting at 0x" << toHex(addr);
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr) << "  0x" << toHexByte(value & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 1) << "  0x" << toHexByte((value >> 8) & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 2) << "  0x" << toHexByte((value >> 16) & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 3) << "  0x" << toHexByte((value >> 24) & 0xFF);
                memChangeLog.push_back(event.str());
            }
        } else if (funct3 == 0x3) { // sd (simulate as sw)
            sim.dataMemory[addr] = value & 0xFF;
            sim.dataMemory[addr + 1] = (value >> 8) & 0xFF;
            sim.dataMemory[addr + 2] = (value >> 16) & 0xFF;
            sim.dataMemory[addr + 3] = (value >> 24) & 0xFF;
            ss << "sd store 0x" << toHex(value)
               << " to addresses starting at 0x" << toHex(addr);
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr) << "  0x" << toHexByte(value & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 1) << "  0x" << toHexByte((value >> 8) & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 2) << "  0x" << toHexByte((value >> 16) & 0xFF);
                memChangeLog.push_back(event.str());
            }
            {
                stringstream event;
                event << toHex(sim.clock) << ": 0x" << toHex(addr + 3) << "  0x" << toHexByte((value >> 24) & 0xFF);
                memChangeLog.push_back(event.str());
            }
        }
    }
    ss << "\n-----------------------------";
    logMessage(ss.str());
}

void writeBack() {
    uint32_t opcode = sim.IR & 0x7F;
    uint32_t rd = (sim.IR >> 7) & 0x1F;
    stringstream ss;
    ss << "\n==== Write Back Stage ====" << endl;
    if (opcode == 0x33 || opcode == 0x13 || opcode == 0x67 ||
        opcode == 0x37 || opcode == 0x17 || opcode == 0x6F) {
        if (rd != 0) {
            sim.registers[rd] = sim.RM;
            ss << "x" << rd << " updated to 0x" << toHex(sim.RM);
        } else {
            ss << "x0 remains 0";
        }
    }
    if (opcode == 0x03) {
        if (rd != 0) {
            sim.registers[rd] = sim.RM;
            ss << "x" << rd << " updated to 0x" << toHex(sim.RM) << " (loaded)";
        }
    }
    ss << "\n-----------------------------";
    logMessage(ss.str());
}

void simulate() {
    while (true) {
        if (sim.instrMemory.find(sim.PC) == sim.instrMemory.end()) {
            logMessage("\nNo instruction found at PC = 0x" + toHex(sim.PC) + ". Halting.");
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

        // Expected line format example:
        // 0x0 0x00800513 , addi x10,x0,8 # 0010011-000-NULL-01010-00000-NULL-000000001000
        size_t posHash = line.find("#");
        string comment;
        if (posHash != string::npos) {
            comment = line.substr(posHash + 1);
            size_t start = comment.find_first_not_of(" ");
            if (start != string::npos) {
                comment = comment.substr(start);
            }
        }

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
            // Store the comment in case it is needed later.
            if (!comment.empty()) {
                instrComments[addr] = comment;
            }
        } else {
            uint8_t data = static_cast<uint8_t>(stoul(valueStr, nullptr, 16));
            sim.dataMemory[addr] = data;
        }
    }
    infile.close();
    
    // Set initial register values.
    sim.registers[2]  = 0x7FFFFFDC;
    sim.registers[11] = 0x7FFFFFDC;
    sim.registers[10] = 0x00000001;
    sim.registers[3]  = 0x10000000;

    simulate();

    // Write memory change events to memory_log.mc in the specified format:
    ofstream dataOut("memory_log.mc");
    if (!dataOut.is_open()) {
        sim.log << "Error: Could not open memory_log.mc for writing." << endl;
        return EXIT_FAILURE;
    }
    for (const auto &event : memChangeLog) {
        dataOut << event << endl;
    }
    dataOut.close();
    sim.log.close();
    return EXIT_SUCCESS;
}
