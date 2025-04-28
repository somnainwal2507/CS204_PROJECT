// branch_prediction1.cpp
#include "branch_prediction1.h"

BranchPredictor::BranchPredictor() { }

bool BranchPredictor::predict(uint32_t pc) {
    auto it = PHT.find(pc);
    if (it == PHT.end()) {
        PHT[pc] = false; // default: not-taken
        return false;
    }
    return it->second;
}


uint32_t BranchPredictor::getTarget(uint32_t pc) const {
    auto it = BTP.find(pc);
    return (it != BTP.end()) ? it->second : pc + 4;
}

void BranchPredictor::update(uint32_t pc, bool taken, uint32_t target) {
    PHT[pc] = taken;
    if (taken) {
        BTP[pc] = target;
    } else {
        BTP.erase(pc);
    }
}

void BranchPredictor::printState() const {
    std::cout << "Branch Predictor State:\n";
    for (auto const &e : PHT) {
        uint32_t pc  = e.first;
        bool     bit = e.second;
        uint32_t tgt = getTarget(pc);
        std::cout << "  PC=0x" << std::hex << pc
                  << std::dec << " - "
                  << (bit ? "TAKEN" : "NOT-TAKEN")
                  << ", target=0x" << std::hex << tgt
                  << std::dec << "\n";
    }
}