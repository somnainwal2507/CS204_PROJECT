// hazard_detection1.h
#ifndef HAZARD_DETECTION1_H
#define HAZARD_DETECTION1_H

#include "pipeline_registers1.h"

// Returns true if the instruction in ID (id_ex) must stall,
// given the contents of EX/MEM and MEM/WB, and whether
// forwarding is enabled.
bool shouldStallIDStage(
    const ID_EX &id_ex,
    const EX_MEM &ex_mem,
    const MEM_WB &mem_wb,
    bool forwardingEnabled);

    enum ForwardingSource {
        NO_FORWARD,
        FORWARD_FROM_EX_MEM,
        FORWARD_FROM_MEM_WB
    };
    
    struct ForwardingControl {
        ForwardingSource forwardA;
        ForwardingSource forwardB;
    };
    
    // Determine which forwarding paths to use based on the pipeline register values.
    ForwardingControl getForwardingControls(const ID_EX &id_ex, const EX_MEM &ex_mem, const MEM_WB &mem_wb);

#endif // HAZARD_DETECTION1_H
