// hazard_detection1.cpp
#include "hazard_detection1.h"
#include <iostream>

bool shouldStallIDStage(
    const ID_EX &id_ex,
    const EX_MEM &ex_mem,
    const MEM_WB &mem_wb,
    bool forwardingEnabled)
{   
    //print all the instructiions in the pipeline registers
    //print only instr.asmStr
    // std::cerr << "[HDU] Called: id_ex.valid=" << id_ex.valid
    //           << " ex_mem.valid=" << ex_mem.valid
    //           << " mem_wb.valid=" << mem_wb.valid
    //           << " ex_mem.rd=" << int(ex_mem.rd)
    //           << " mem_wb.rd=" << int(mem_wb.rd)
    //           << " rs1=" << int(id_ex.rs1)
    //           << " rs2=" << int(id_ex.rs2) << "\n";

    // std::cerr << "[HDU] ID_EX: " << id_ex.instr.asmStr << "\n";
    // std::cerr << "[HDU] EX_MEM: " << ex_mem.instr.asmStr << "\n";
    // std::cerr << "[HDU] MEM_WB: " << mem_wb.instr.asmStr << "\n";



    if (!id_ex.valid) 
        return false;  // nothing to stall

    auto uses_rs1 = (id_ex.rs1 != 0);
    auto uses_rs2 = (id_ex.rs2 != 0);

    // 1) **** Must stall load→use hazard: ****
    //    if prior instruction is a load in EX/MEM whose rd matches rs1/rs2
    if (ex_mem.valid && ex_mem.memRead &&
        ( (uses_rs1 && ex_mem.rd == id_ex.rs1)
       || (uses_rs2 && ex_mem.rd == id_ex.rs2) ) )
    {
        std::cerr << "[HDU] Stall: load→use hazard (EX/MEM rd="
                  << int(ex_mem.rd) << ")\n";
        return true;
    }

    // 2) If forwarding is OFF, also catch **any** RAW in EX/MEM:
    if (!forwardingEnabled &&
        ex_mem.valid && ex_mem.regWrite &&
        ( (uses_rs1 && ex_mem.rd == id_ex.rs1)
       || (uses_rs2 && ex_mem.rd == id_ex.rs2) ) )
    {
        std::cerr << "[HDU] Stall: RAW hazard (EX/MEM rd="
                  << int(ex_mem.rd) << ")\n";
        return true;
    }

    // 3) If forwarding is OFF, catch RAW in MEM/WB as well:
    if (!forwardingEnabled &&
        mem_wb.valid && mem_wb.regWrite &&
        ( (uses_rs1 && mem_wb.rd == id_ex.rs1)
       || (uses_rs2 && mem_wb.rd == id_ex.rs2) ) )
    {
        std::cerr << "[HDU] Stall: RAW hazard (MEM/WB rd="
                  << int(mem_wb.rd) << ")\n";
        return true;
    }

    return false;
}


ForwardingControl getForwardingControls(const ID_EX &id_ex, const EX_MEM &ex_mem, const MEM_WB &mem_wb) {
    ForwardingControl fc = {NO_FORWARD, NO_FORWARD};
    
    if(id_ex.valid && ex_mem.valid && ex_mem.regWrite &&
       (ex_mem.rd == id_ex.rs1) && (ex_mem.rd != 0)) {
        fc.forwardA = FORWARD_FROM_EX_MEM;
    } else if(id_ex.valid && mem_wb.valid && mem_wb.regWrite &&
              (mem_wb.rd == id_ex.rs1) && (mem_wb.rd != 0)) {
        fc.forwardA = FORWARD_FROM_MEM_WB;
    }    
    if(id_ex.valid && ex_mem.valid && ex_mem.regWrite &&
       (ex_mem.rd == id_ex.rs2) && (ex_mem.rd != 0)) {
        fc.forwardB = FORWARD_FROM_EX_MEM;
    } else if(id_ex.valid && mem_wb.valid && mem_wb.regWrite &&
              (mem_wb.rd == id_ex.rs2) && (mem_wb.rd != 0)) {
        fc.forwardB = FORWARD_FROM_MEM_WB;
    }
    if( id_ex.valid && ex_mem.valid && ex_mem.memRead &&
       (ex_mem.rd == id_ex.rs1) && (ex_mem.rd != 0)) {
        fc.forwardA = FORWARD_FROM_MEM_WB;
    }
    if(id_ex.valid && ex_mem.valid && mem_wb.memRead &&
              (mem_wb.rd == id_ex.rs2) && (ex_mem.rd != 0)) {
        fc.forwardB = FORWARD_FROM_MEM_WB;
    }
    
    return fc;
}