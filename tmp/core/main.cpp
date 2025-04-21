#include <bits/stdc++.h>
using namespace std;

#include "isa_map.cpp"
#include "utils.cpp"
#include "hazard_unit.cpp"
#include "processor.cpp"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: ./riscv_sim <input.mc> [--pipeline]\n";
        return 1;
    }

    string mcFile = argv[1];
    bool pipeline = (argc >= 3 && string(argv[2]) == "--pipeline");

    Processor cpu(pipeline);
    cpu.loadMachineCode(mcFile);
    cpu.run();                    // executes until ecall 10 or EOF

    cpu.dumpRegisters("reg_out.mc");
    cpu.dumpMemory   ("data_out.mc");
    cpu.dumpStats    ("stats.txt");

    cout << "Simulation complete.  Stats written to stats.txt\n";
    return 0;
}

