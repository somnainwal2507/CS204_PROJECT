//data_memory.h
#ifndef DATA_MEMORY_H
#define DATA_MEMORY_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>

class DataMemory {
public:
    // Base address (e.g. 0x10000000) and size in bytes.
    DataMemory(uint32_t baseAddress = 0x10000000);

    // Load the data segment out of the same output.mc file
    // (skips everything until "Data Segment").
    bool loadFromFile(const std::string& filename);

    uint32_t readWord(uint32_t address);
    uint8_t readByte(uint32_t address) ;
    uint16_t readHalfword(uint32_t address);
    void     writeWord(uint32_t address, uint32_t value);
    void     writeByte(uint32_t address, uint8_t value);
    void     writeHalfword(uint32_t address, uint16_t value);
    void     dump(uint32_t start, uint32_t end);
    void     reset();

private:
    uint32_t baseAddr;
    std::map<uint32_t, uint8_t> memory; // address -> byte value
};

#endif // DATA_MEMORY_H
