class Processor {
    vector<uint8_t> MEM;          // simple byte‑addressable memory
    int32_t R[32] {};
    uint32_t PC = 0;

    bool pipelineMode;
    HazardUnit HU;

    /* pipeline latches */
    StageReg IF_ID, ID_EX, EX_MEM, MEM_WB;

    /* stats */
    uint64_t cycles = 0, instrCnt = 0, stalls = 0;

public:
    explicit Processor(bool pipe) : MEM(1<<20, 0), pipelineMode(pipe) { R[0] = 0; }

    void loadMachineCode(const string& fname)
    {
        ifstream fin(fname);
        if (!fin) { cerr << "Cannot open " << fname << endl; exit(1); }
        string addr, word, comma;
        while (fin >> addr >> word) {
            uint32_t a = hexToUint(addr);
            uint32_t w = hexToUint(word);
            memcpy(&MEM[a], &w, 4);
        }
    }

    /* ----- helpers for decoding individual fields ----- */
    inline uint32_t getOpcode(uint32_t inst) { return inst & 0x7f; }
    inline uint32_t getRd    (uint32_t inst) { return (inst >> 7)  & 0x1f; }
    inline uint32_t getFunct3(uint32_t inst) { return (inst >> 12) & 0x7; }
    inline uint32_t getRs1   (uint32_t inst) { return (inst >> 15) & 0x1f; }
    inline uint32_t getRs2   (uint32_t inst) { return (inst >> 20) & 0x1f; }
    inline uint32_t getFunct7(uint32_t inst) { return (inst >> 25) & 0x7f; }

    int32_t immI(uint32_t inst) { return signExtend(inst >> 20, 12); }
    int32_t immS(uint32_t inst) { return signExtend(((inst>>25)<<5)|getRd(inst), 12); }
    int32_t immB(uint32_t inst)
    {
        return signExtend(((inst>>31)<<12)|(((inst>>7)&1)<<11)|(((inst>>25)&0x3f)<<5)|((inst>>8)&0xf)<<1, 13);
    }
    int32_t immJ(uint32_t inst)
    {
        return signExtend(((inst>>31)<<20)|(((inst>>12)&0xff)<<12)|(((inst>>20)&1)<<11)|(((inst>>21)&0x3ff)<<1), 21);
    }

    /* ---------- core run loop ---------- */
    void run()
    {
        while (true) {
            if (pipelineMode) pipelineStep();
            else              singleCycleStep();

            if (MEM_WB.valid && MEM_WB.instr==0x0000006b) break; // ecall with a7=10
        }
    }

    /* ---------- single‑cycle (easy) ---------- */
    void singleCycleStep()
    {
        uint32_t inst = fetch();
        exec(inst);
        cycles++;
    }

    /* ---------- pipeline ---------- */
    void pipelineStep()
    {
        cycles++;

        /* 5. Write‑back */
        if (MEM_WB.valid && MEM_WB.rd!=0 && MEM_WB.rd!=INVALID_REG) [MEM_WB.rd] = MEM_WB.rs1;

        /* 4. Memory */
        if (EX_MEM.valid) {
            MEM_WB = EX_MEM;
            // lw / sw handled inside exec()
        } else MEM_WB = {};

        /* 3. Execute */
        if (ID_EX.valid) {
            EX_MEM      = ID_EX;
            uint32_t i  = ID_EX.instr;
            uint32_t op = getOpcode(i);

            if (op == 0b0110011) {          // R‑type
                int32_t a = R[ID_EX.rs1], b = R[ID_EX.rs2];
                if (getFunct3(i)==0 && getFunct7(i)==0)    EX_MEM.rs1 = a + b;        // add
                else if (getFunct3(i)==0 && getFunct7(i)==0x20) EX_MEM.rs1 = a - b;   // sub
            }
            else if (op == 0b0010011) {     // addi
                EX_MEM.rs1 = R[ID_EX.rs1] + immI(i);
            }
            else if (op == 0b0000011) {     // lw
                int32_t addr = R[ID_EX.rs1] + immI(i);
                EX_MEM.rs1 = *reinterpret_cast<int32_t*>(&MEM[addr]);
            }
            else if (op == 0b0100011) {     // sw
                int32_t addr = R[ID_EX.rs1] + immS(i);
                *reinterpret_cast<int32_t*>(&MEM[addr]) = R[ID_EX.rs2];
            }
            else if (op == 0b1100011) {     // beq
                if (R[ID_EX.rs1]==R[ID_EX.rs2]) PC = ID_EX.pc + immB(i);
            }
            else if (op == 0b1101111) {     // jal
                EX_MEM.rs1 = ID_EX.pc + 4;
                PC = ID_EX.pc + immJ(i);
            }
            else if (i==0x00000073) {       // ecall
                if (R[17]==10) {            // a7==10
                    EX_MEM.instr = 0x0000006b; // custom HALT
                }
            }
        } else EX_MEM = {};

        /* 2. Decode  + hazard check */
        if (IF_ID.valid) {
            int needed = HU.stallNeeded(IF_ID, ID_EX, EX_MEM);
            if (needed) { stalls += needed; /* simple stall: keep IF_ID */ }
            else {
                ID_EX = IF_ID;
                ID_EX.valid = true;
                decodeFill(ID_EX);
                IF_ID = {};
            }
        }

        /* 1. Fetch */
        if (!IF_ID.valid) {
            uint32_t inst = fetch();
            // IF_ID = {true, inst, PC-4, getRd(inst), getRs1(inst), getRs2(inst)};

						IF_ID = {true, inst, PC-4,
										 static_cast<uint8_t>(getRd(inst)),
										 static_cast<uint8_t>(getRs1(inst)),
										 static_cast<uint8_t>(getRs2(inst))};
        }
    }

    void decodeFill(StageReg& s) { /* nothing extra yet */ }

    uint32_t fetch()
    {
        uint32_t inst = *reinterpret_cast<uint32_t*>(&MEM[PC]);
        PC += 4;
        instrCnt++;
        return inst;
    }

    void exec(uint32_t inst)
    {
        uint32_t op = getOpcode(inst);
        if (op == 0b0110011) {              // R‑type
            int32_t a = R[getRs1(inst)], b = R[getRs2(inst)];
            if   (getFunct3(inst)==0 && getFunct7(inst)==0)      R[getRd(inst)] = a + b;  // add
            else if (getFunct3(inst)==0 && getFunct7(inst)==0x20) R[getRd(inst)] = a - b; // sub
        }
        else if (op == 0b0010011) {         // addi
            R[getRd(inst)] = R[getRs1(inst)] + immI(inst);
        }
        else if (op == 0b0000011) {         // lw
            int32_t addr = R[getRs1(inst)] + immI(inst);
            R[getRd(inst)] = *reinterpret_cast<int32_t*>(&MEM[addr]);
        }
        else if (op == 0b0100011) {         // sw
            int32_t addr = R[getRs1(inst)] + immS(inst);
            *reinterpret_cast<int32_t*>(&MEM[addr]) = R[getRs2(inst)];
        }
        else if (op == 0b1100011) {         // beq
            if (R[getRs1(inst)] == R[getRs2(inst)]) PC = PC + immB(inst);
        }
        else if (op == 0b1101111) {         // jal
            R[getRd(inst)] = PC;
            PC = PC + immJ(inst);
        }
        else if (inst==0x00000073) {        // ecall
            if (R[17]==10) exit(0);
        }
    }

    /* dumps */
    void dumpRegisters(const string& file)
    {
        ofstream fout(file);
        for (int i=0;i<32;i++) fout << "x" << i << " 0x" << to_hex(R[i]) << "\n";
    }
    void dumpMemory(const string& file)
    {
        ofstream fout(file);
        for (uint32_t a=0; a<MEM.size(); a+=4) {
            uint32_t w = *reinterpret_cast<uint32_t*>(&MEM[a]);
            if (w) fout << "0x" << to_hex(a) << " 0x" << to_hex(w) << "\n";
        }
    }
    void dumpStats(const string& file)
    {
        ofstream s(file);
        s << "Cycles: " << cycles << "\n";
        s << "Instructions: " << instrCnt << "\n";
        s << "CPI: " << fixed << setprecision(3) << (double)cycles/instrCnt << "\n";
        s << "Stalls: " << stalls << "\n";
    }
};

