//text_memory.h
#ifndef TEXT_MEMORY_H
#define TEXT_MEMORY_H

#include <vector>
#include <string>
#include "pipeline_registers1.h"  // for Instruction

class TextMemory {
public:
    // Load only the instruction (text) portion of output.mc.
    // Stops when it sees the "Data Segment" marker.
    bool loadFromFile(const std::string& filename);

    // Fetch instruction at byte‑address PC.
    Instruction fetch(uint32_t pc) const;

    size_t size() const { return instructions.size(); }

private:
    std::vector<Instruction> instructions;
};

#endif // TEXT_MEMORY_H
