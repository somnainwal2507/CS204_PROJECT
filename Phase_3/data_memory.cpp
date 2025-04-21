//data_memory.cpp

#include "data_memory.h"
#include <fstream>
#include <sstream>
#include <iostream> 
#include <stdexcept>
#include <iomanip>
#include <map>

DataMemory::DataMemory(uint32_t baseAddress)
  : baseAddr(baseAddress)
{}

bool DataMemory::loadFromFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) return false;

    std::string line;
    // 1) skip to "Data Segment"
    while (std::getline(infile, line)) {
        if (line.find("Data Segment") != std::string::npos)
            break;
    }

    // 2) read all (addr, byte) pairs
    std::vector<std::pair<uint32_t,uint8_t>> entries;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string addrStr, valStr;
        iss >> addrStr >> valStr;
        uint32_t addr = std::stoul(addrStr, nullptr, 16);
        uint32_t  val  = static_cast<uint8_t>(
                          std::stoul(valStr, nullptr, 16)
                       );
        entries.emplace_back(addr, val);
    }
    if (entries.empty()) return true;  // no data segment

    

    // 5) store each byte
    for (auto &p : entries) {
        memory[p.first]=    p.second;
    }

    return true;
}


uint32_t DataMemory::readWord(uint32_t address)  {
    return  memory[address]
          | (memory[address + 1] << 8)
          | (memory[address + 2] << 16)
          | (memory[address + 3] << 24);
}

uint8_t DataMemory::readByte(uint32_t address)  {
    uint8_t x = memory[address];

    return x;
}
uint16_t DataMemory::readHalfword(uint32_t address)  {
    return  memory[address]
          | (memory[address + 1] << 8);
}


void DataMemory::writeWord(uint32_t address, uint32_t value) {
    memory[address    ] = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
    memory[address + 2] = (value >> 16) & 0xFF;
    memory[address + 3] = (value >> 24) & 0xFF;
}

void DataMemory::writeByte(uint32_t address, uint8_t value) {
    memory[address] = value;
}

void DataMemory::writeHalfword(uint32_t address, uint16_t value) {
    memory[address]     = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
}

void DataMemory::dump(uint32_t startAddr, uint32_t endAddr)  {
    for (uint32_t addr = startAddr; addr < endAddr; addr += 4) {
        uint32_t value = readWord(addr);
        std::cout << std::hex << "0x" << addr << ": 0x" << value << "\n";
    }
}

void DataMemory::reset() {
    memory.clear();
}
