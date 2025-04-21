//text_memory.cpp
#include "text_memory.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;
static inline string trim(const string& s) {
    auto b = s.find_first_not_of(" \t");
    auto e = s.find_last_not_of(" \t");
    return (b == string::npos) ? "" : s.substr(b, e - b + 1);
}

bool TextMemory::loadFromFile(const string& filename) {
    ifstream infile(filename);
    if (!infile.is_open()) return false;

    string line;
    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line == "Data Segment") break;

        // Expected: 0xADDR 0xBIN , asmStr # ...
        istringstream iss(line);
        string addrStr, binStr, comma;
        iss >> addrStr >> binStr >> comma;
        string rest;
        getline(iss, rest);
        // Drop trailing decode bits (after '#')
        auto hashPos = rest.find('#');
        string asmStr = trim((hashPos == string::npos)
                                  ? rest
                                  : rest.substr(0, hashPos));

        Instruction instr;
        instr.binary = static_cast<uint32_t>(stoul(binStr, nullptr, 16));
        instr.asmStr = asmStr;
        instructions.push_back(instr);
    }

    return true;
}

Instruction TextMemory::fetch(uint32_t pc) const {
    size_t idx = pc / 4;
    if (idx < instructions.size()) {
        return instructions[idx];
    }
    // off‐end: supply a NOP
    return {0x00000013, "NOP"};
}
