#include <bits/stdc++.h>
#include "format.cpp"    // Your existing Rformat, Iformat, Sformat, etc.
using namespace std;
#define ll long long int

int data = 0x10000000;
int dataAddress = data;
int pc = 0;

// Forward declarations from lookup.cpp
extern unordered_map<string, vector<string>> codes_map;
extern map<string,int> datatype_map;
extern unordered_map<string, string> register_map;
extern unordered_map<string, char> type_map;

// -----------------------------------------------------------
// 1) Bit-String Helpers
//    Each function replicates the same parsing as Rformat/Iformat/etc.,
//    then converts fields to binary strings with fixed widths.
// -----------------------------------------------------------

// Convert an integer 'value' into a binary string of length 'width'.
static string toBinaryStr(unsigned value, int width) {
    // e.g. toBinaryStr(3, 5) -> "00011"
    // Use std::bitset for easy formatting.
    bitset<32> bs(value);
    // We only want the rightmost 'width' bits.
    string full = bs.to_string(); // 32 bits
    return full.substr(32 - width, width);
}

// For R-type: <opcode(7)>-<func3(3)>-<func7(7)>-<rd(5)>-<rs1(5)>-<rs2(5)>-NULL
string RbitString(const vector<string> &instr) {
    // Example: "add x1 x2 x3"
    // 1) Parse opcode/func3/func7 from codes_map
    string opcode_hex = codes_map[instr[0]][0]; // e.g. "0x33"
    string func3_hex  = codes_map[instr[0]][1]; // e.g. "0x0"
    string func7_hex  = codes_map[instr[0]][2]; // e.g. "0x00"

    int opcode = stoi(opcode_hex, nullptr, 16); // 0x33 => 51 decimal => 110011 in binary (6 bits), but RISC-V is 7 bits for opcode
    int func3  = stoi(func3_hex, nullptr, 16); 
    int func7  = stoi(func7_hex, nullptr, 16);

    // 2) Parse rd, rs1, rs2 from xN
    int rd  = stoi(instr[1].substr(1));
    int rs1 = stoi(instr[2].substr(1));
    int rs2 = stoi(instr[3].substr(1));

    // 3) Convert each to binary string
    //    RISC-V 7-bit opcode, 3-bit func3, 7-bit func7, 5-bit rd/rs1/rs2
    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string func7_bin  = toBinaryStr(func7, 7);
    string rd_bin     = toBinaryStr(rd, 5);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string rs2_bin    = toBinaryStr(rs2, 5);

    // 4) Combine with dashes, end with "-NULL"
    //    e.g. "0110011-000-0000000-00001-00010-00011-NULL"
    ostringstream oss;
    oss << opcode_bin << "-"
        << func3_bin  << "-"
        << func7_bin  << "-"
        << rd_bin     << "-"
        << rs1_bin    << "-"
        << rs2_bin    << "-"
        << "NULL";
    return oss.str();
}

// For I-type: <opcode(7)>-<func3(3)>-NULL-<rd(5)>-<rs1(5)>-NULL-<imm(12)>
string IbitString(const vector<string> &instr) {
    // Example: "andi x5 x6 10"
    // 1) Parse opcode/func3 from codes_map
    string opcode_hex = codes_map[instr[0]][0]; 
    string func3_hex  = codes_map[instr[0]][1]; 

    int opcode = stoi(opcode_hex, nullptr, 16); 
    int func3  = stoi(func3_hex, nullptr, 16);

    // 2) Parse rd, rs1, imm
    //    You already do complex logic in Iformat to handle "lb x1, 4(x2)" style.
    //    We'll do a simpler approach here, assuming "instr.size()==4" for typical I-type.
    //    If you have loads with 3 tokens, you'll need extra checks, etc.
    int rd  = stoi(instr[1].substr(1));
    // If immediate is in instr[3], then rs1 is in instr[2].
    // If it's a load with "lw x1, 4(x2)", you need to parse that. This example is simplified.
    int rs1 = 0;
    long long imm_val = 0;

    // Check if we have "xN" or "4(xN)"
    // (This is a simplified approach. You may want to replicate the exact logic in Iformat.)
    size_t parenPos = instr[3].find('(');
    if (parenPos == string::npos) {
        // No parentheses => "andi x5 x6 10" style
        rs1 = stoi(instr[2].substr(1));
        imm_val = stoll(instr[3]); 
    } else {
        // "lb x1 4(x2)" style
        // parse imm from the part before '('
        string offsetStr = instr[3].substr(0, parenPos); // e.g. "4"
        imm_val = stoll(offsetStr);
        // parse rs1 from inside the parentheses
        size_t closePos = instr[3].find(')');
        string regStr = instr[3].substr(parenPos+1, closePos - (parenPos+1)); // e.g. "x2"
        rs1 = stoi(regStr.substr(1));
    }

    // 3) Convert each field to binary
    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string rd_bin     = toBinaryStr(rd, 5);
    string rs1_bin    = toBinaryStr(rs1, 5);

    // Immediate is 12 bits
    // If negative or large, you’d normally sign-extend. We'll do a simple mask here:
    unsigned long long imm_12 = static_cast<unsigned long long>(imm_val) & 0xFFF;
    string imm_bin = toBinaryStr((unsigned)imm_12, 12);

    // 4) Combine
    //    e.g. "0010011-111-NULL-00101-00110-NULL-000000001010"
    ostringstream oss;
    oss << opcode_bin << "-"
        << func3_bin  << "-"
        << "NULL"     << "-"
        << rd_bin     << "-"
        << rs1_bin    << "-"
        << "NULL"     << "-"
        << imm_bin;
    return oss.str();
}

// Similar approach for S, SB, U, UJ:

string SbitString(const vector<string> &instr) {
    // Example: "sw x1, 4(x2)"
    // opcode(7 bits) - func3(3 bits) - NULL - NULL - rs1(5 bits) - rs2(5 bits) - imm(12 bits)
    // (Because S-type doesn't truly have an rd; the "func7" bits are partly from imm.)
    string opcode_hex = codes_map[instr[0]][0];
    string func3_hex  = codes_map[instr[0]][1];
    int opcode = stoi(opcode_hex, nullptr, 16); 
    int func3  = stoi(func3_hex, nullptr, 16);

    // parse rs2 from, e.g., "x1"
    int rs2 = stoi(instr[1].substr(1));

    // parse the "4(x2)" portion
    size_t parenPos = instr[2].find('(');
    size_t closePos = instr[2].find(')');
    long long imm_val = stoll(instr[2].substr(0, parenPos));
    int rs1 = stoi(instr[2].substr(parenPos+2, closePos - (parenPos+2))); // skip 'x'

    // Convert to binary
    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string rs2_bin    = toBinaryStr(rs2, 5);

    // 12-bit immediate
    unsigned imm_12 = static_cast<unsigned>(imm_val) & 0xFFF;
    string imm_bin = toBinaryStr(imm_12, 12);

    // Combine: "opcode-func3-NULL-NULL-rs1-rs2-imm"
    // You can adapt the exact order if you want it to match your example precisely.
    ostringstream oss;
    oss << opcode_bin << "-"
        << func3_bin  << "-"
        << "NULL-NULL-"
        << rs1_bin    << "-"
        << rs2_bin    << "-"
        << imm_bin;
    return oss.str();
}

string SBbitString(const vector<string> &instr) {
    // Example: "beq x1,x2, offset"
    // For illustration, we'll do:
    // opcode(7 bits)-func3(3 bits)-NULL-rs1(5 bits)-rs2(5 bits)-NULL-imm(13 bits?)
    // RISC-V uses a 12-bit immediate, but it’s split in a special way. We'll keep it simple.
    string opcode_hex = codes_map[instr[0]][0];
    string func3_hex  = codes_map[instr[0]][1];
    int opcode = stoi(opcode_hex, nullptr, 16);
    int func3  = stoi(func3_hex, nullptr, 16);

    int rs1 = stoi(instr[1].substr(1));
    int rs2 = stoi(instr[2].substr(1));
    long long imm_val = stoll(instr[3]); // ignoring sign for brevity

    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string rs2_bin    = toBinaryStr(rs2, 5);

    // For branches, the immediate is 12 bits (though used differently).
    unsigned imm_12 = static_cast<unsigned>(imm_val) & 0xFFF;
    string imm_bin = toBinaryStr(imm_12, 12);

    ostringstream oss;
    oss << opcode_bin << "-"
        << func3_bin  << "-"
        << "NULL-"
        << rs1_bin    << "-"
        << rs2_bin    << "-"
        << "NULL-"
        << imm_bin;
    return oss.str();
}

string UbitString(const vector<string> &instr) {
    // e.g. "lui x10, 0xABCD"
    // opcode(7 bits)-NULL-NULL-rd(5 bits)-NULL-NULL-imm(20 bits)
    string opcode_hex = codes_map[instr[0]][0];
    int opcode = stoi(opcode_hex, nullptr, 16);

    int rd = stoi(instr[1].substr(1));
    long long imm_val = stoll(instr[2]); // ignoring sign for brevity

    string opcode_bin = toBinaryStr(opcode, 7);
    string rd_bin     = toBinaryStr(rd, 5);

    // 20-bit immediate
    unsigned imm_20 = static_cast<unsigned>(imm_val) & 0xFFFFF;
    string imm_bin = toBinaryStr(imm_20, 20);

    ostringstream oss;
    oss << opcode_bin << "-"
        << "NULL-NULL-"
        << rd_bin    << "-"
        << "NULL-NULL-"
        << imm_bin;
    return oss.str();
}

string UJbitString(const vector<string> &instr) {
    // e.g. "jal x1, 32"
    // opcode(7 bits)-NULL-NULL-rd(5 bits)-NULL-NULL-imm(20 bits)
    // (RISC-V J immediate is 20 bits, arranged in a special shuffle, but we'll keep it simple.)
    string opcode_hex = codes_map[instr[0]][0];
    int opcode = stoi(opcode_hex, nullptr, 16);

    int rd = stoi(instr[1].substr(1));
    long long imm_val = stoll(instr[2]);

    string opcode_bin = toBinaryStr(opcode, 7);
    string rd_bin     = toBinaryStr(rd, 5);

    unsigned imm_20 = static_cast<unsigned>(imm_val) & 0xFFFFF;
    string imm_bin = toBinaryStr(imm_20, 20);

    ostringstream oss;
    oss << opcode_bin << "-"
        << "NULL-NULL-"
        << rd_bin    << "-"
        << "NULL-NULL-"
        << imm_bin;
    return oss.str();
}
// -----------------------------------------------------------


// The rest of your main.cpp code follows,
// with the final printing changed to match the format in your screenshot.
int main() {
    map<string,int> varmap;
    map<int,int> dataSegment;
    map<string,int> label;

    ifstream inputFile("input.asm");
    ofstream dataOutputFile("output.mc");
    string line;
    int flag = 0, comment = 0;

    // Pass 1: parse .data
    while (getline(inputFile, line))
    {
        if (line.empty()) continue;

        stringstream ss(line);
        vector<string> tokens;
        string token, temp;
        while (ss >> token) {
            for(char c: token) {
                if(c == '#') {
                    if(!temp.empty() && !comment) {
                        tokens.push_back(temp);
                    }
                    temp="";
                    comment=1;
                    break;
                }
                else if(c == ',') {
                    tokens.push_back(temp);
                    temp="";
                }
                else {
                    temp+=c;
                }     
            }
            if(!temp.empty() && !comment) {
                tokens.push_back(temp);
            }
            temp="";
            if(comment) {
                break;
            }
        }
        comment = 0;
        if(tokens.empty()) continue;

        if (tokens[0] == ".text") {
            flag=0;
            break;
        } else if (tokens[0] == ".data") {
            flag=1;
            continue;
        }

        // If we are in .data
        if(flag) {
            tokens[0].pop_back(); // remove ':'
            if (tokens.size() > 1 && tokens[1][0] == '.') {
                // e.g. var1: .word 0xFF
                if(datatype_map.find(tokens[1])!=datatype_map.end()) {
                    varmap[tokens[0]] = dataAddress;
                    int i=2;
                    while(i<(int)tokens.size()) {
                        string s = tokens[i];
                        long long n;
                        if(s.size()>=2 && s[0]=='0' && (s[1]=='x'||s[1]=='X')) {
                            n = stoll(s, nullptr, 16);
                        } else {
                            n = stoll(s);
                        }
                        int sizeBytes = datatype_map[tokens[1]];
                        while(sizeBytes--) {
                            dataSegment[dataAddress] = (n & 0xFF);
                            n >>= 8;
                            dataAddress++;
                        }
                        i++;
                    }
                }
                else if(tokens[1] == ".asciiz") {
                    if(tokens.size()!=3) {
                        dataOutputFile<<"Error for "<<tokens[0]<<endl;
                        break;
                    }
                    varmap[tokens[0]] = dataAddress;
                    // store string
                    string s = tokens[2];
                    for(char c : s) {
                        if(c=='"') continue;
                        dataSegment[dataAddress++] = c;
                    }
                    dataSegment[dataAddress++] = 0; // null terminator
                }
            }
        }
    }

    // Pass 2: find labels in .text
inputFile.clear();
inputFile.seekg(0, ios::beg);
flag = 0;
comment = 0;
while (getline(inputFile, line)) {
    if (line.empty()) continue;
    stringstream ss(line);
    vector<string> tokens;
    string token;
    while (ss >> token) {
        if (token[0] == '#') break;  // skip comments
        tokens.push_back(token);
    }
    if (tokens.empty()) continue;
    
    // Handle section directives
    if (tokens[0] == ".text") {
        flag = 0;
        continue;
    } else if (tokens[0] == ".data") {
        flag = 1;
        continue;
    }
    
    if (!flag) {  // Only process lines in the .text section
        // Check if a label is defined
        if (tokens[0].back() == ':') {
            // Label defined as "label:"
            string lab = tokens[0].substr(0, tokens[0].size() - 1);
            label[lab] = pc;
            continue;
        } else if (tokens.size() > 1 && tokens[1] == ":") {
            // Label defined as "label :"
            label[tokens[0]] = pc;
            continue;
        }
        // Otherwise, treat it as an instruction.
        pc += 4;
        // Check for a pseudo-instruction (e.g., la)
        if (tokens[0][0] == 'l' && type_map.find(tokens[0]) != type_map.end()) {
            // e.g. la x5 var1 => expecting at least 3 tokens
            if (tokens.size() >= 3 && varmap.find(tokens[2]) != varmap.end()) {
                pc += 4;
            }
        }
    }
}

    // Pass 3: Generate machine code
    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    pc=0; flag=0; comment=0;

    while(getline(inputFile, line)) {
        if(line.empty()) continue;
        stringstream ss(line);
        vector<string> tokens;
        string token, temp;
        while(ss >> token) {
            for(char c: token) {
                if(c=='#') {
                    if(!temp.empty() && !comment) {
                        tokens.push_back(temp);
                    }
                    temp="";
                    comment=1;
                    break;
                }
                else if(c==',') {
                    tokens.push_back(temp);
                    temp="";
                }
                else {
                    temp+=c;
                }
            }
            if(!temp.empty() && !comment) {
                tokens.push_back(temp);
            }
            temp="";
            if(comment) break;
        }
        comment=0;
        if(tokens.empty()) continue;

        if(tokens[0] == ".text") {
            flag=0; 
            continue;
        } else if(tokens[0] == ".data") {
            flag=1;
            continue;
        }

        if(!flag) {
            // We are in .text
            // Rebuild a user-friendly instruction string: "add x1,x2,x3"
            // (Insert commas for registers/immediates.)
            // Example: tokens = ["add","x1","x2","x3"]
            // => "add x1,x2,x3"
            // You can adapt how you want to re-insert commas/spaces.
            ostringstream instrStr;
            instrStr << tokens[0];
            for(int i=1; i<(int)tokens.size(); i++) {
                if(i==1) instrStr << " " << tokens[i]; 
                else instrStr << "," << tokens[i];
            }
            string originalInstr = instrStr.str();

            // If tokens[0] is a known instruction:
            if(type_map.find(tokens[0]) != type_map.end()) {
                char type = type_map[tokens[0]];
                // We’ll store the machine code & bit breakdown in separate strings:
                string mc, bits;
                switch(type) {
                    case 'r': {
                        mc   = Rformat(tokens);     // from format.cpp
                        bits = RbitString(tokens);  // new function above
                        dataOutputFile << "0x" << std::hex << pc << " " 
                                       << mc << " , " << originalInstr 
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 'i': {
                        // Pseudo-instruction check (e.g. "la")
                        if(tokens[0][0] == 'l' && tokens.size()>=3 && varmap.find(tokens[2])!=varmap.end()) {
                            // first line: auipc
                            // second line: actual I instruction
                            // This logic is from your original code, adapt as needed
                            int num = data>>12;
                            vector<string> tempInstr = {"auipc", tokens[1], to_string(num)};
                            string mc2   = Uformat(tempInstr);
                            string bits2 = UbitString(tempInstr);
                            // Print the "auipc" line
                            dataOutputFile << "0x" << std::hex << pc << " "
                                           << mc2 << " , "
                                           << ("auipc "+tokens[1]+","+to_string(num))
                                           << " # " << bits2 << "\n";
                            pc += 4;

                            num <<= 12;
                            int offset = (varmap[tokens[2]] - num - pc);
                            tokens[2] = to_string(offset) + "(" + tokens[1] + ")";
                        }
                        mc   = Iformat(tokens);
                        bits = IbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " "
                                       << mc << " , " << originalInstr
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 's': {
                        mc   = Sformat(tokens);
                        bits = SbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " "
                                       << mc << " , " << originalInstr
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 'b': {
                        // label offset
                        if(tokens.size()>=4 && label.find(tokens[3])!=label.end()) {
                            tokens[3] = to_string(label[tokens[3]] - pc);
                        }
                        mc   = SBformat(tokens);
                        bits = SBbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " "
                                       << mc << " , " << originalInstr
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 'u': {
                        mc   = Uformat(tokens);
                        bits = UbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " "
                                       << mc << " , " << originalInstr
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 'j': {
                        // label offset
                        if(tokens.size()>=3 && label.find(tokens[2])!=label.end()) {
                            tokens[2] = to_string(label[tokens[2]] - pc);
                        }
                        mc   = UJformat(tokens);
                        bits = UJbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " "
                                       << mc << " , " << originalInstr
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    default:
                        dataOutputFile << "Error" << "\n";
                }
            }
            else {
                // Possibly a label definition
                // e.g. "main:"
                int sz = tokens[0].size();
                if(tokens[0][sz-1] == ':') {
                    tokens[0].pop_back();
                    // if it’s not in label map, might be an error or just a label
                    if(label.find(tokens[0]) == label.end()) {
                        dataOutputFile<<"Command not found"<<endl;
                        break;
                    }
                }
            }
        }
    }

    // Finally, print data segment
    dataOutputFile << "\nData Segment\n";
    for (auto &it : dataSegment) {
        dataOutputFile << "0x" << std::hex << it.first << " 0x" << std::hex << it.second << "\n";
    }

    inputFile.close();
    dataOutputFile.close();
    return 0;
}
