//main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <vector>
#include <string>
#include <iomanip>

#include "text_memory.h"
#include "data_memory.h"
#include "register_file.h"
#include "control_path.h"
#include "datapath.h"
#include "branch_prediction1.h"
#include "hazard_detection1.h"
#include "pipeline_registers1.h"

using namespace std;
// Knobs
struct Knobs {
    bool pipeline      = true;   // Knob1
    bool forwarding    = true;   // Knob2
    bool traceRegs     = false;  // Knob3
    bool tracePipe     = false;  // Knob4
    bool traceInst     = false;  // Knob5
    int  traceInstIdx  = -1;
    bool traceBP       = false;  // Knob6
};

// Simple command‑line parser for our six knobs.
Knobs parseArgs(int argc, char** argv) {
    

    Knobs k;
    for(int i = 1; i < argc; ++i) {
        string a = argv[i];
        if      (a == "--no-pipeline")    k.pipeline   = false;
        else if (a == "--no-forward")     k.forwarding = false;
        else if (a == "--trace-regs")     k.traceRegs  = true;
        else if (a == "--trace-pipe")     k.tracePipe  = true;
        else if (a == "--trace-inst" && i+1<argc) {
            k.traceInst    = true;
            k.traceInstIdx = stoi(argv[++i]);
        }
        else if (a == "--trace-bp")       k.traceBP    = true;
        else if (a == "--help") {
            cout << "Usage: " << argv[0] << " [--no-pipeline] [--no-forward]\n"
                      << "                  [--trace-regs] [--trace-pipe]\n"
                      << "                  [--trace-inst <idx>] [--trace-bp]\n";
            exit(0);
        }
    }
    return k;
}




void printExecutionTable(
    const vector<string>& instrNames,
    const vector<vector<char>>& execTable,
    int totalCycles)
{
    // Header row
    cout << setw(15) << "Instruction";
    for(int c = 1; c <= totalCycles; ++c) {
        cout << setw(4) << c;
    }
    cout << "\n\n";

    // Each instruction row
    for(size_t i = 0; i < instrNames.size(); ++i) {
        // truncate or pad the instruction name to 15 chars
        string name = instrNames[i];
        if (name.size() > 15) name = name.substr(0, 15);
        cout << setw(15) << name;

        // for each cycle, print the stage letter or blank
        for(int c = 0; c < totalCycles; ++c) {
            char stage = execTable[i][c];
            cout << setw(4) << (stage ? stage : ' ');
        }
        cout << "\n";
    }
    cout << endl;
}

int main(int argc, char** argv) {
    // 1) Parse knobs
    Knobs knobs = parseArgs(argc, argv);

    // 2) Instantiate & load memories
    TextMemory    imem;
    DataMemory    dmem;
    if (!imem.loadFromFile("output.mc") || !dmem.loadFromFile("output.mc")) {
        cerr << "ERROR: could not open output.mc\n";
        return 1;
    }
    // 3) Instantiate register file & branch predictor
    RegisterFile  regs;
    BranchPredictor bp;

    // 4) Initialize pipeline‐registers as empty
    IF_ID   if_id   = {{0,""},0,false};
    ID_EX   id_ex   = {0,{},0,0,0,0,0,0,0,0,false,false,false,false,false};
    EX_MEM  ex_mem  = {0,{},0,0,0,false,false,false,false};
    MEM_WB  mem_wb  = {0,{},0,0,0,false,false,false};

    // 5) Stat counters
    uint64_t cycle            = 0;
    uint64_t totalInsts       = 0;
    uint64_t numLoads         = 0;
    uint64_t numALU           = 0;
    uint64_t numControl       = 0;
    uint64_t numStalls        = 0;
    uint64_t numDataHazards   = 0;
    uint64_t numControlHazards= 0;
    uint64_t numMispredicts   = 0;
    uint64_t numStallsData    = 0;
    uint64_t numStallsControl = 0;

    // 6) Program counter
    uint32_t PC = 0;
    uint32_t lastAddr = imem.size() * 4;
    // one entry per instruction in text memory:
    vector<string> instrNames(imem.size());
// a table of N instructions × (maxCycles) cycles, initially zeroed:
    vector<vector<char>> execTable(
        imem.size(),
        vector<char>(/* put an upper bound on cycles, or resize later */ 100000, 0)
    );

    uint32_t exForwardedOp1 = 0;
    uint32_t exForwardedOp2 = 0;
    bool haltFetchThisCycle = false;
    // 7) Pipelined simulation
    while (true) {
        bool exForwardedEq,exForwardedNEq;
        //cout<<"[DEBUG...] cycle="<<cycle<<"\n";
        // Terminate when PC past end AND pipeline empty
        if (PC >= lastAddr &&
            !if_id.valid && !id_ex.valid && !ex_mem.valid && !mem_wb.valid)
            break;

        ++cycle;

        // Create next‐cycle copies
        IF_ID   next_if   = if_id;
        ID_EX   next_id   = id_ex;
        EX_MEM  next_ex   = ex_mem;
        MEM_WB  next_mem   = mem_wb;
        uint32_t nextPC   = PC;
        bool doStall      = false;

        // ===== WB Stage =====
        if (mem_wb.valid) {
            //cout<<"[WB STAGE] "<<mem_wb.instr.asmStr<<endl;
            //totalInsts++;
            int idx = mem_wb.pc / 4;
            execTable[idx][cycle-1] = 'W';
            instrNames[idx] = mem_wb.instr.asmStr;
            if (mem_wb.regWrite) {
                uint32_t val = mem_wb.memRead
                             ? mem_wb.memData
                             : mem_wb.aluResult;
                regs.write(mem_wb.rd, val);
            }
        }

        // ===== MEM Stage =====
if (ex_mem.valid) {
    //cout<<"[MEM STAGE] "<<ex_mem.instr.asmStr<<endl;
    int idx = ex_mem.pc / 4;
    execTable[idx][cycle-1] = 'M';
    instrNames[idx]         = ex_mem.instr.asmStr;

    next_mem.valid    = true;
    next_mem.pc       = ex_mem.pc;
    next_mem.instr    = ex_mem.instr;
    next_mem.aluResult= ex_mem.aluResult;
    next_mem.rd       = ex_mem.rd;
    next_mem.regWrite = ex_mem.regWrite;

    // extract the 3‑bit funct3 from the binary
    uint8_t funct3 = (ex_mem.instr.binary >> 12) & 0x7;
    // ------- LOADs -------
    if (ex_mem.memRead) {
        uint32_t loaded;
        switch(funct3) {
          case 0: { // LB
            int8_t b = static_cast<int8_t>( dmem.readByte(ex_mem.aluResult) );
            loaded  = static_cast<uint32_t>(b);       // sign‑extend
            break;
          }
          case 1: { // LH
            int16_t h = static_cast<int16_t>(
                          dmem.readHalfword(ex_mem.aluResult)
                        );
            loaded  = static_cast<uint32_t>(h);       // sign‑extend
            break;
          }
          case 2:   // LW
            loaded = dmem.readWord(ex_mem.aluResult);
            break;
          case 4:   // LBU
            loaded = dmem.readByte(ex_mem.aluResult); // zero‑extend
            break;
          case 5:   // LHU
            loaded = dmem.readHalfword(ex_mem.aluResult);// zero‑extend
            break;
          default:  // fallback to word
            loaded = dmem.readWord(ex_mem.aluResult);
        }
        next_mem.memRead = true;
        next_mem.memData = loaded;
    } else {
        next_mem.memRead = false;
    }

    // ------- STOREs -------
    if (ex_mem.memWrite) {
        switch(funct3) {
          case 0: // SB
            dmem.writeByte(
              ex_mem.aluResult,
              static_cast<uint8_t>(ex_mem.writeData & 0xFF)
            );
            break;
          case 1: // SH
            dmem.writeHalfword(
              ex_mem.aluResult,
              static_cast<uint16_t>(ex_mem.writeData & 0xFFFF)
            );
            break; 
          case 2: // SW
            dmem.writeWord(
              ex_mem.aluResult,
              ex_mem.writeData
            );
            break;
          default:
            // you could assert or treat as SW
            dmem.writeWord(ex_mem.aluResult, ex_mem.writeData);
        }
    }
}
else {
    next_mem.valid = false;
}


        // ===== EX Stage =====
        if (id_ex.valid) {
            //cout<<"[EX STAGE] "<<id_ex.instr.asmStr<<endl;
            int idx = id_ex.pc / 4;
            execTable[idx][cycle-1] = 'X';
            instrNames[idx] = id_ex.instr.asmStr;
            
            
            // 3) Forwarding
            ForwardingControl fc = { NO_FORWARD, NO_FORWARD };
            if (knobs.forwarding) {
                fc = getForwardingControls(id_ex, ex_mem, mem_wb);
            }

            auto getOp = [&](uint32_t origVal, ForwardingSource fs){
                if (fs == FORWARD_FROM_EX_MEM) return ex_mem.aluResult;
                if (fs == FORWARD_FROM_MEM_WB) {
                    return mem_wb.memRead ? mem_wb.memData : mem_wb.aluResult;
                }
                return origVal;
            };

            uint32_t op1 = getOp(
                id_ex.readData1, fc.forwardA
            );

            uint32_t op2 = id_ex.aluSrc
                ? static_cast<uint32_t>(id_ex.imm)
                : id_ex.readData2;

            if (!id_ex.aluSrc) {
                op2 = getOp(id_ex.readData2, fc.forwardB);
            }
            // 1) Branch evaluation
            bool taken = false;
            bool exForwardedEq = op1==op2;
            bool exForwardedNEq = op1!=op2;
            exForwardedOp1 = op1;
            exForwardedOp2 = op2;
            uint32_t targetPC = 0;

            if (id_ex.branch) {
                taken = evaluateBranch(
                    op1,
                    op2,
                    (id_ex.instr.binary >> 12) & 0x7
                );
                targetPC = id_ex.pc + id_ex.imm;
            }

            // 2) On a mispredict, we'll need to flush IF/ID & ID/EX
        bool predTaken = bp.predict(id_ex.pc);
        if (id_ex.branch && (predTaken != taken)) {
            ++numMispredicts;
            // immediately update the PHT with the real outcome
            bp.update(id_ex.pc, taken, targetPC);
            // redirect fetch
            nextPC = taken ? targetPC : (id_ex.pc + 4);
            // flush the in‐flight decode stages
            next_if.valid = false;
            next_id.valid = false;
            ++numControlHazards;
            if (!knobs.pipeline) ++numStallsControl;
            haltFetchThisCycle = true;
            // **do not** re-issue this branch into EX/MEM:
            next_ex.valid = false;
        } else {
            haltFetchThisCycle = false;
            // 3) Normal EX: ALU + forward into EX/MEM
            uint32_t aluOut = executeALU(
                op1, op2,
                id_ex.opcode,
                (id_ex.instr.binary >> 12) & 0x7,
                (id_ex.instr.binary >> 25) & 0x7F
            );
            next_ex.valid        = true;
            next_ex.pc           = id_ex.pc;
            next_ex.instr        = id_ex.instr;
            next_ex.aluResult    = aluOut;
            next_ex.writeData    = op2;
            next_ex.rd           = id_ex.rd;
            next_ex.regWrite     = id_ex.regWrite;
            next_ex.memRead      = id_ex.memRead;
            next_ex.memWrite     = id_ex.memWrite;
            next_ex.branch       = id_ex.branch;
            next_ex.imm          = id_ex.imm;
            next_ex.branchTarget = targetPC;
        }
        } else {
            next_ex.valid = false;
        }

        // ===== ID Stage =====
        if (!haltFetchThisCycle && if_id.valid) {
            //cout<<"[ID STAGE] "<<if_id.instr.asmStr<<endl;
            // 1) Decode control
            int idx = if_id.pc / 4;
            execTable[idx][cycle-1] = 'D';
            instrNames[idx] = if_id.instr.asmStr;
            uint32_t inst = if_id.instr.binary;
            uint8_t opcode = inst & 0x7F;
            

            ControlSignals ctrl = generateControlSignals(opcode);

            // 2) Extract fields
            ID_EX candidate{};
            
            int32_t imm = 0;
            switch (opcode) {
                case 0x33: // R‑type (funct3 in its 12..14)
                    candidate.valid      = true;
                    candidate.pc         = if_id.pc;
                    candidate.instr      = if_id.instr;
                    candidate.opcode     = opcode;
                    candidate.rs1        = (inst >> 15) & 0x1F;
                    candidate.rs2        = (inst >> 20) & 0x1F;
                    candidate.rd         = (inst >> 7)  & 0x1F;
                    break;
                case 0x03: // I‑type (funct3 in its 12..14)
                    candidate.valid      = true;
                    candidate.pc         = if_id.pc;
                    candidate.instr      = if_id.instr;
                    candidate.opcode     = opcode;
                    candidate.rs1        = (inst >> 15) & 0x1F;
                    candidate.rd         = (inst >> 7)  & 0x1F;
                    imm = static_cast<int32_t>(inst) >> 20; break;
                case 0x67: // JALR (funct3 in its 12..14)
                    candidate.valid      = true;
                    candidate.pc         = if_id.pc;
                    candidate.instr      = if_id.instr;
                    candidate.opcode     = opcode;
                    candidate.rs1        = (inst >> 15) & 0x1F;
                    candidate.rd         = (inst >> 7)  & 0x1F;
                    imm = static_cast<int32_t>(inst) >> 20; break;
              case 0x13:
                    candidate.valid      = true;
                    candidate.pc         = if_id.pc;
                    candidate.instr      = if_id.instr;
                    candidate.opcode     = opcode;
                    candidate.rs1        = (inst >> 15) & 0x1F;
                    //candidate.rs2        = (inst >> 20) & 0x1F;
                    candidate.rd         = (inst >> 7)  & 0x1F;
                    imm = static_cast<int32_t>(inst) >> 20; break;
              case 0x23:
              candidate.valid      = true;
            candidate.pc         = if_id.pc;
            candidate.instr      = if_id.instr;
            candidate.opcode     = opcode;
            candidate.rs1        = (inst >> 15) & 0x1F;
            candidate.rs2        = (inst >> 20) & 0x1F;
            //candidate.rd         = (inst >> 7)  & 0x1F;
                imm = ((inst >> 25) << 5) | ((inst >> 7) & 0x1F);
                if (imm & 0x800) imm |= 0xFFFFF000;
                break;
              case 0x63:
              candidate.valid      = true;
              candidate.pc         = if_id.pc;
              candidate.instr      = if_id.instr;
              candidate.opcode     = opcode;
              candidate.rs1        = (inst >> 15) & 0x1F;
              candidate.rs2        = (inst >> 20) & 0x1F;
              //candidate.rd         = (inst >> 7)  & 0x1F;
                imm = ((inst >> 31) << 12)
                    | (((inst >> 7) & 0x1) << 11)
                    | (((inst >> 25) & 0x3F) << 5)
                    | (((inst >> 8)  & 0xF) << 1);
                if (imm & 0x1000) imm |= 0xFFFFE000;
                break;
              case 0x37:
              candidate.valid      = true;
              candidate.pc         = if_id.pc;
              candidate.instr      = if_id.instr;
              candidate.opcode     = opcode;
              //candidate.rs1        = (inst >> 15) & 0x1F;
              //candidate.rs2        = (inst >> 20) & 0x1F;
              candidate.rd         = (inst >> 7)  & 0x1F;
                imm = inst & 0xFFFFF000;
                break;
              case 0x17:
              candidate.valid      = true;
            candidate.pc         = if_id.pc;
            candidate.instr      = if_id.instr;
            candidate.opcode     = opcode;
            //candidate.rs1        = (inst >> 15) & 0x1F;
            //candidate.rs2        = (inst >> 20) & 0x1F;
            candidate.rd         = (inst >> 7)  & 0x1F;
                imm = inst & 0xFFFFF000;
                break;
                case 0x6f:
                candidate.valid      = true;
            candidate.pc         = if_id.pc;
            candidate.instr      = if_id.instr;
            candidate.opcode     = opcode;
            //candidate.rs1        = (inst >> 15) & 0x1F;
            //candidate.rs2        = (inst >> 20) & 0x1F;
            candidate.rd         = (inst >> 7)  & 0x1F;
                imm = ((inst >> 12) & 0xFF) << 12
                    | ((inst >> 20) & 0x1) << 11
                    | ((inst >> 21) & 0x3FF) << 1;
                if (imm & 0x80000000) imm |= 0xFFF00000;
                break;
                default:
                    candidate.valid = false;
            }
            candidate.imm = imm;         
            candidate.regWrite = ctrl.regWrite;
            candidate.memRead  = ctrl.memRead;
            candidate.memWrite = ctrl.memWrite;
            candidate.branch   = ctrl.branch;
            candidate.aluSrc   = ctrl.aluSrc;

            // 3) Read register file
            candidate.readData1 = regs.read(candidate.rs1);
            candidate.readData2 = regs.read(candidate.rs2);


        bool stall = shouldStallIDStage(candidate, next_ex, next_mem, knobs.forwarding);
            if (stall) {
            // inject a single-cycle bubble:
            doStall       = true;
            next_id.valid = false;    // bubble in ID→EX
            next_if       = if_id;    // freeze IF/ID
        nextPC        = PC;       // do not advance PC
        ++numDataHazards;
        ++numStalls;
        ++numStallsData;
    } else {
        // no stall ⇒ push candidate forward
        next_id       = candidate;
        totalInsts++;
    }

        } else {
            next_id.valid = false;
        }

        // ===== IF Stage =====
        if (!doStall && !haltFetchThisCycle && PC < lastAddr) {
            //cout<<"[IF STAGE] "<<if_id.instr.asmStr<<endl;
            int idx = PC / 4;
            execTable[idx][cycle-1] = 'F';
            instrNames[idx] =  imem.fetch(PC).asmStr;
            // 1) Branch predictor gives us a PC
            bool take = bp.predict(PC);
            uint32_t targ   = bp.getTarget(PC);
            Instruction fetched = imem.fetch(PC);
            next_if.valid = true;
            next_if.pc    = PC;
            next_if.instr = fetched;

            PC += 4;
            nextPC = take ? targ : PC;
            if (take) ++numControlHazards;

        } else if (!doStall) {
            next_if.valid = false;
        }

        // ===== Commit updates =====
        if_id   = next_if;
        id_ex   = next_id;
        ex_mem  = next_ex;
        mem_wb  = next_mem;
        PC      = nextPC;

        // if(ex_mem.valid && ex_mem.branch){
        //     bool actual = ((ex_mem.instr.binary >> 12)& 0x7) == 0 ? exForwardedEq : ((ex_mem.instr.binary >> 12)& 0x7) == 1 ? exForwardedNEq 
        //     : evaluateBranch(exForwardedOp1, exForwardedOp2, (ex_mem.instr.binary >> 12) & 0x7);
        //     bp.update(ex_mem.pc, actual,  ex_mem.branchTarget);
        // }
        // ===== Tracing =====
        if (knobs.traceRegs) {
            regs.dump();
        }
        if (knobs.tracePipe) {
            cout << "[Cycle " << cycle << "]\n"
                      << " IF/ID: " << (if_id.valid ? if_id.instr.asmStr : "----") << "\n"
                      << " ID/EX: " << (id_ex.valid ? id_ex.instr.asmStr : "----") << "\n"
                      << " EX/MEM: " << (ex_mem.valid ? ex_mem.instr.asmStr : "----") << "\n"
                      << " MEM/WB: " << (mem_wb.valid ? mem_wb.instr.asmStr : "----") << "\n";
        }
        if (knobs.traceInst && if_id.valid && (int)(if_id.pc/4) == knobs.traceInstIdx) {
            cout << "[Trace Inst " << knobs.traceInstIdx << "] in IF/ID at cycle " << cycle << "\n";
        }else if (knobs.traceInst && id_ex.valid && (int)(id_ex.pc/4) == knobs.traceInstIdx) {
            cout << "[Trace Inst " << knobs.traceInstIdx << "] in ID/EX at cycle " << cycle << "\n";
        }else if (knobs.traceInst && ex_mem.valid && (int)(ex_mem.pc/4) == knobs.traceInstIdx) {
            cout << "[Trace Inst " << knobs.traceInstIdx << "] in EX/MEM at cycle " << cycle << "\n";
        }else if (knobs.traceInst && mem_wb.valid && (int)(mem_wb.pc/4) == knobs.traceInstIdx) {
            cout << "[Trace Inst " << knobs.traceInstIdx << "] in MEM/WB at cycle " << cycle << "\n";
        }
        if (knobs.traceBP) {
            bp.printState();
        }
    } // end while

    // 8) Stats → pipeline_sim.txt
    ofstream outf("pipeline_sim.txt");
    outf << "Total cycles: "      << cycle << "\n"
         << "Total instructions: "<< totalInsts << "\n"
         << "CPI: "               << fixed << setprecision(2)
                                 << double(cycle)/totalInsts << "\n"
         << "Data-transfer: "     << numLoads << "\n"
         << "ALU instructions: "  << numALU << "\n"
         << "Control instructions:" << numControl << "\n"
         << "Stalls/bubbles: "    << numStalls << "\n"
         << "Data hazards: "      << numDataHazards << "\n"
         << "Control hazards: "   << numControlHazards << "\n"
         << "Branch mispredictions: " << numMispredicts << "\n"
         << "Stalls due to data hazards: "    << numStallsData << "\n"
         << "Stalls due to control hazards: " << numStallsControl << "\n";
    outf.close();

    printExecutionTable(instrNames, execTable, cycle);

    return 0;
}


