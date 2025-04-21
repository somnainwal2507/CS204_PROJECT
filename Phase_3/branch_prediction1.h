// branch_prediction1.h
#ifndef BRANCH_PREDICTION_H
#define BRANCH_PREDICTION_H

#include <unordered_map>
#include <cstdint>
#include <iostream>

class BranchPredictor {
public:
    BranchPredictor();
    bool predict(uint32_t pc);
    uint32_t getTarget(uint32_t pc) const;
    void update(uint32_t pc, bool taken, uint32_t target);
    // Add this declaration:
    void printState() const;

private:
    std::unordered_map<uint32_t, bool> PHT;
    std::unordered_map<uint32_t, uint32_t> BTP;
};

#endif // BRANCH_PREDICTION_H