struct StageReg {
    bool valid = false;
    uint32_t instr = 0;
    uint32_t pc    = 0;
    uint8_t  rd    = INVALID_REG;
    uint8_t  rs1   = INVALID_REG;
    uint8_t  rs2   = INVALID_REG;	
};

class HazardUnit {
public:
    bool dataForwarding = true;

    int stallNeeded(const StageReg& id, const StageReg& ex, const StageReg& mem)
    {
							
        // if (id.rs1 && id.rs1 == ex.rd && ex.valid) return dataForwarding ? 0 : 1;
        // if (id.rs2 && id.rs2 == ex.rd && ex.valid) return dataForwarding ? 0 : 1;
        // if (id.rs1 && id.rs1 == mem.rd && mem.valid) return dataForwarding ? 0 : 1;
        // if (id.rs2 && id.rs2 == mem.rd && mem.valid) return dataForwarding ? 0 : 1;
				

				auto h = [&](uint8_t src, const StageReg& prv) {
						return  (src != 0)                       // x0 is never a hazard sink
								 && (src != INVALID_REG)             // unused field
								 && prv.valid
								 && prv.rd != 0
								 && prv.rd == src;
				};

				if (h(id.rs1, ex)  || h(id.rs2, ex) ||
						h(id.rs1, mem) || h(id.rs2, mem))
						return dataForwarding ? 0 : 1;

        return 0;
    }
};
