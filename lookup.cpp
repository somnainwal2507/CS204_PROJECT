#include <bits/stdc++.h>
using namespace std;


unordered_map <string,vector<string>> codes_map = {

    //R format
    {"add", {"0x33", "0x0", "0x00"}},
    {"and", {"0x33", "0x7", "0x00"}},
    {"or", {"0x33", "0x6", "0x00"}},
    {"sll", {"0x33", "0x1", "0x00"}},
    {"slt", {"0x33", "0x2", "0x00"}},
    {"sra", {"0x33", "0x5", "0x20"}},
    {"srl", {"0x33", "0x5", "0x00"}},
    {"sub", {"0x33", "0x0", "0x20"}},
    {"xor", {"0x33", "0x4", "0x00"}},
    {"mul", {"0x33", "0x0", "0x01"}},
    {"div", {"0x33", "0x4", "0x01"}},
    {"rem", {"0x33", "0x6", "0x01"}},

    //I format
    {"addi", {"0x13", "0x0"}},
    {"andi", {"0x13", "0x7"}},
    {"ori", {"0x13", "0x6"}},
    {"lb", {"0x03", "0x0"}},
    {"ld", {"0x03", "0x3"}},
    {"lh", {"0x03", "0x1"}},
    {"lw", {"0x03", "0x2"}},
    {"jalr", {"0x67", "0x0"}},

    //S format
    {"sb", {"0x23", "0x0"}},
    {"sw", {"0x23", "0x2"}},
    {"sd", {"0x23", "0x3"}},
    {"sh", {"0x23", "0x1"}},

    //SB format
    {"beq", {"0x63", "0x0"}},
    {"bne", {"0x63", "0x1"}},
    {"bge", {"0x63", "0x5"}},
    {"blt", {"0x63", "0x4"}},

    //U format
    {"auipc", {"0x17"}},
    {"lui", {"0x37"}},

    //UJ format
    {"jal", {"0x6f"}}
};




map <string,int> datatype_map = {

    {".byte", 1},
    {".half", 2},
    {".word", 4},
    {".dword", 8},
    {".double", 8}
};





unordered_map <string, string> register_map = {

    {"zero", "x0"},
    {"ra", "x1"},
    {"sp", "x2"},
    {"gp", "x3"},
    {"tp", "x4"},
    {"t0", "x5"},
    {"t1", "x6"},
    {"t2", "x7"},
    {"s0", "x8"},
    {"s1", "x9"},
    {"a0", "x10"},
    {"a1", "x11"},
    {"a2", "x12"},
    {"a3", "x13"},
    {"a4", "x14"},
    {"a5", "x15"},
    {"a6", "x16"},
    {"a7", "x17"},
    {"s2", "x18"},
    {"s3", "x19"},
    {"s4", "x20"},
    {"s5", "x21"},
    {"s6", "x22"},
    {"s7", "x23"},
    {"s8", "x24"},
    {"s9", "x25"},
    {"s10", "x26"},
    {"s11", "x27"},
    {"t3", "x28"},
    {"t4", "x29"},
    {"t5", "x30"},
    {"t6", "x31"},
};

unordered_map <string,char> type_map = {
    {"add", 'r'},
    {"and", 'r'},
    {"or", 'r'},
    {"sll", 'r'},
    {"slt", 'r'},
    {"sra", 'r'},
    {"srl", 'r'},
    {"sub", 'r'},
    {"xor", 'r'},
    {"mul", 'r'},
    {"div", 'r'},
    {"rem", 'r'},

    {"addi", 'i'},
    {"andi", 'i'},
    {"ori", 'i'},
    {"lb", 'i'},
    {"ld", 'i'},
    {"lh", 'i'},
    {"lw", 'i'},
    {"jalr", 'i'},

    {"sb", 's'},
    {"sw", 's'},
    {"sd", 's'},
    {"sh", 's'},

    {"beq", 'b'},
    {"bne", 'b'},
    {"bge", 'b'},
    {"blt", 'b'},

    {"auipc", 'u'},
    {"lui", 'u'},

    {"jal", 'j'}
};
