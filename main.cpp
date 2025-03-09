#include <bits/stdc++.h>
#include "format.cpp"    // Your existing Rformat, Iformat, Sformat, etc.
using namespace std;
#define ll long long int

int data = 0x10000000;
int dataAddress = data;
int pc = 0;

static string toBinaryStr(unsigned value, int width) {
    bitset<32> bs(value);
    string full = bs.to_string(); // 32 bits
    return full.substr(32 - width, width);
}

// For R-type: <opcode(7)>-<func3(3)>-<func7(7)>-<rd(5)>-<rs1(5)>-<rs2(5)>-NULL
string RbitString(const vector<string> &instr) {
    string opcode_hex = codes_map[instr[0]][0];
    string func3_hex  = codes_map[instr[0]][1];
    string func7_hex  = codes_map[instr[0]][2];

    int opcode = stoi(opcode_hex, nullptr, 16);
    int func3  = stoi(func3_hex, nullptr, 16);
    int func7  = stoi(func7_hex, nullptr, 16);

    int rd  = stoi(instr[1].substr(1));
    int rs1 = stoi(instr[2].substr(1));
    int rs2 = stoi(instr[3].substr(1));

    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string func7_bin  = toBinaryStr(func7, 7);
    string rd_bin     = toBinaryStr(rd, 5);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string rs2_bin    = toBinaryStr(rs2, 5);

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

//Same for other types
string IbitString(const vector<string> &instr) {
    string opcode_hex = codes_map[instr[0]][0];
    string func3_hex  = codes_map[instr[0]][1];
    int opcode = stoi(opcode_hex, nullptr, 16);
    int func3  = stoi(func3_hex, nullptr, 16);
    int rd = stoi(instr[1].substr(1));
    int rs1 = 0;
    int imm_val = 0;  // immediate is 12-bit

    // Adjust based on the number of tokens:
    if (instr.size() == 3) {
        // Load instruction like "lw x10,0(x6)"
        size_t parenPos = instr[2].find('(');
        if (parenPos == string::npos) {
            throw std::invalid_argument("Invalid instruction format: missing '('");
        } else {
            string offsetStr = instr[2].substr(0, parenPos); // e.g. "0"
            // Use parseImmediate to support char literals etc.
            imm_val = static_cast<int>(parseImmediate(offsetStr));
            size_t closePos = instr[2].find(')');
            string regStr = instr[2].substr(parenPos+1, closePos - (parenPos+1)); // e.g. "x6"
            rs1 = stoi(regStr.substr(1));
        }
    } else if (instr.size() >= 4) {
        // For instructions like "addi x5 x10 -8" or "addi x5 x11 'a'"
        size_t parenPos = instr[3].find('(');
        if (parenPos == string::npos) {
            rs1 = stoi(instr[2].substr(1));
            // Here we use parseImmediate instead of stoi to handle char literals.
            imm_val = static_cast<int>(parseImmediate(instr[3]));
        } else {
            string offsetStr = instr[3].substr(0, parenPos);
            imm_val = static_cast<int>(parseImmediate(offsetStr));
            size_t closePos = instr[3].find(')');
            string regStr = instr[3].substr(parenPos+1, closePos - (parenPos+1));
            rs1 = stoi(regStr.substr(1));
        }
    }

    // Check if the immediate is within the valid 12-bit range (-2048 to 2047)
    if (imm_val < -2048 || imm_val > 2047) {
        throw std::out_of_range("Immediate out of bounds");
    }
    
    // Convert negative immediate to 2's complement 12-bit representation.
    unsigned int imm_12;
    if (imm_val < 0)
        imm_12 = (1 << 12) + imm_val;
    else
        imm_12 = imm_val;
    
    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string rd_bin     = toBinaryStr(rd, 5);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string imm_bin    = toBinaryStr(imm_12, 12);
    
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

string SbitString(const vector<string> &instr) {
    string opcode_hex = codes_map[instr[0]][0];
    string func3_hex  = codes_map[instr[0]][1];
    int opcode = stoi(opcode_hex, nullptr, 16); 
    int func3  = stoi(func3_hex, nullptr, 16);

    int rs2 = stoi(instr[1].substr(1));

    size_t parenPos = instr[2].find('(');
    size_t closePos = instr[2].find(')');
    int imm_val = stoi(instr[2].substr(0, parenPos));
    int rs1 = stoi(instr[2].substr(parenPos+2, closePos - (parenPos+2))); // skip 'x'

    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string rs2_bin    = toBinaryStr(rs2, 5);
    unsigned imm_12 = static_cast<unsigned>(imm_val) & 0xFFF;
    string imm_bin = toBinaryStr(imm_12, 12);

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
    string opcode_hex = codes_map[instr[0]][0];
    string func3_hex  = codes_map[instr[0]][1];
    int opcode = stoi(opcode_hex, nullptr, 16);
    int func3  = stoi(func3_hex, nullptr, 16);

    int rs1 = stoi(instr[1].substr(1));
    int rs2 = stoi(instr[2].substr(1));
    int imm_val = stoi(instr[3]); // simplified for illustration

    string opcode_bin = toBinaryStr(opcode, 7);
    string func3_bin  = toBinaryStr(func3, 3);
    string rs1_bin    = toBinaryStr(rs1, 5);
    string rs2_bin    = toBinaryStr(rs2, 5);
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
    string opcode_hex = codes_map[instr[0]][0];
    int opcode = stoi(opcode_hex, nullptr, 16);
    int rd = stoi(instr[1].substr(1));
    int imm_val = stoi(instr[2]); // simplified (hex conversion may be needed)

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

string UJbitString(const vector<string> &instr) {
    string opcode_hex = codes_map[instr[0]][0];
    int opcode = stoi(opcode_hex, nullptr, 16);
    int rd = stoi(instr[1].substr(1));
    int imm_val = stoi(instr[2]);

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
            int f=0;
            for(char c: token) {
                if(c == '#') {
                    if(!temp.empty() && !comment) {
                        tokens.push_back(temp);
                    }
                    temp = "";
                    comment = 1;
                    break;
                }
                else if(c == ',') {
                    if(f==1) temp+=c;
                    else{    
                        tokens.push_back(temp);
                        temp = "";
                    }
                }
                else {
                    if(c=='\"') f=1;
                    temp += c;
                }     
            }
            if(!temp.empty() && !comment) {
                tokens.push_back(temp);
            }
            temp = "";
            if(comment) break;
        }
        comment = 0;
        if(tokens.empty()) continue;

        if (tokens[0] == ".text") {
            flag = 0;
            break;
        } else if (tokens[0] == ".data") {
            flag = 1;
            continue;
        }

        // In .data section, process variables and directives.
        if(flag) {
            if(tokens[0][(tokens[0]).size()-1]==':'){
                tokens[0].pop_back(); // remove ':' from label
                if (tokens.size() > 1 && tokens[1][0] == '.') {
                    if(datatype_map.find(tokens[1]) != datatype_map.end()) {
                        varmap[tokens[0]] = dataAddress;
                        int i = 2;
                        while(i < (int)tokens.size()) {
                            long long n;
                            string s = tokens[i];
                            int sizeBytes = datatype_map[tokens[1]];
                            if(s.size() >= 2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
                                if((s.size()-2)>sizeBytes*2){
                                    cout<<"Data size is invalid"<<endl;
                                    break;
                                }
                                n = stoll(s, nullptr, 16);
                            } else {
                                if(s.size() == 3 && s[0]=='\'' && s[2]=='\'') {
                                    n = s[1];
                                } else {
                                    n = stoll(s);
                                }
                            }
                            while(sizeBytes--) {
                                dataSegment[dataAddress] = (n & 0xFF);
                                n >>= 8;
                                dataAddress++;
                            }
                            i++;
                        }
                    }
                    else if(tokens[1] == ".asciiz") {
                        if(tokens.size()>2){
                            if(tokens[2][0]!='\"' || tokens[tokens.size()-1][(tokens[tokens.size()-1]).size()-1]!='\"') {
                                dataOutputFile << "Error for " << tokens[0] << endl;
                                break;
                            }
                            if(tokens[0][tokens[0].size()-1] == ':') tokens[0].pop_back();
                            varmap[tokens[0]] = dataAddress;
                            int i=2;
                            while(i < (int)tokens.size()) {
                                string s = tokens[i];
                                for(char c : s) {    
                                    if(c == '"') continue;
                                    dataSegment[dataAddress++] = c;
                                }
                                if(i!=tokens.size()-1) dataSegment[dataAddress++] = ' ';
                                i++;
                            }  
                            dataSegment[dataAddress++] = 0;  
                        }
                    }
                }
            }else{
                if (tokens.size() >= 1 && tokens[0][0] == '.') {
                    if(datatype_map.find(tokens[0]) != datatype_map.end()) {
                        int i = 1;
                        while(i < (int)tokens.size()) {
                            long long n;
                            string s = tokens[i];
                            int sizeBytes = datatype_map[tokens[0]];
                            if(s.size() >= 2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
                                if((s.size()-2)>sizeBytes*2){
                                    cout<<"Data size is invalid"<<endl;
                                    break;
                                }
                                n = stoll(s, nullptr, 16);
                            } else {
                                if(s.size() == 3 && s[0]=='\'' && s[2]=='\'') {
                                    n = s[1];
                                } else {
                                    n = stoll(s);
                                }
                            }
                            while(sizeBytes--) {
                                dataSegment[dataAddress] = (n & 0xFF);
                                n >>= 8;
                                dataAddress++;
                            }
                            i++;
                        }
                    }
                    else if(tokens[0] == ".asciiz") {
                        if(tokens.size()>1){
                            if(tokens[1][0]!='\"' || tokens[tokens.size()-1][(tokens[tokens.size()-1]).size()-1]!='\"') {
                                dataOutputFile << "Error for " << tokens[0] << endl;
                                break;
                            }
                            int i=1;
                            while(i < (int)tokens.size()) {
                                string s = tokens[i];
                                for(char c : s) {    
                                    if(c == '"') continue;
                                    dataSegment[dataAddress++] = c;
                                }
                                if(i!=tokens.size()-1) dataSegment[dataAddress++] = ' ';
                                i++;
                            }  
                            dataSegment[dataAddress++] = 0;  
                        }
                    }
                }
            }
        }
    }

    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    flag = 0; comment = 0;
    while(getline(inputFile, line)) {
        if(line.empty()) continue;
        stringstream ss(line);
        vector<string> tokens;
        string token;
        while(ss >> token) {
            if(token[0] == '#') break;
            if(token == ".text") {
                flag = 0;
                break;
            } else if(token == ".data") {
                flag = 1;
                break;
            }
            int sz = token.size();
            if(token[sz-1] == ':' && !flag) {
                token.pop_back();
                label[token] = pc;
            }
            else if(!flag) {
                pc += 4;
                if(token[0]=='l' && type_map.find(token) != type_map.end()) {
                    ss >> token; 
                    ss >> token; \
                    if(varmap.find(token) != varmap.end()) {
                        pc += 4;
                    }
                }
            }
            break;
        }
    }

    // Pass 3: Generate machine code
    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    pc = 0; flag = 0; comment = 0;
    while(getline(inputFile, line)) {
        if(line.empty()) continue;
        stringstream ss(line);
        vector<string> tokens;
        string token, temp;
        while(ss >> token) {
            for(char c: token) {
                if(c == '#') {
                    if(!temp.empty() && !comment) {
                        tokens.push_back(temp);
                    }
                    temp = "";
                    comment = 1;
                    break;
                }
                else if(c == ',') {
                    tokens.push_back(temp);
                    temp = "";
                }
                else {
                    temp += c;
                }
            }
            if(!temp.empty() && !comment) {
                tokens.push_back(temp);
            }
            temp = "";
            if(comment) break;
        }
        comment = 0;
        if(tokens.empty()) continue;

        if(tokens[0] == ".text") {
            flag = 0;
            continue;
        } else if(tokens[0] == ".data") {
            flag = 1;
            continue;
        }

        if(!flag) {
            ostringstream instrStr;
            instrStr << tokens[0];
            for(int i = 1; i < (int)tokens.size(); i++) {
                if(i == 1)
                    instrStr << " " << tokens[i];
                else
                    instrStr << "," << tokens[i];
            }
            string originalInstr = instrStr.str();

            if(type_map.find(tokens[0]) != type_map.end()) {
                char type = type_map[tokens[0]];
                string mc, bits;
                switch(type) {
                    case 'r': {
                        mc   = Rformat(tokens);
                        bits = RbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " " 
                                       << mc << " , " << originalInstr 
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 'i': {
                        if(tokens[0][0]=='l'){
                            if(tokens.size()>3){
                                string mer="";
                                mer+=tokens[2];
                                mer+='(';
                                mer+=tokens[3];
                                mer+=')';
                                tokens[2]=mer;
                                while(tokens.size()>3) tokens.pop_back();
                            }
                            if(tokens.size()>=3 && varmap.find(tokens[2]) != varmap.end()) {
                                int num = data >> 12;
                                vector<string> tempInstr = {"auipc", tokens[1], to_string(num)};
                                string mc2   = Uformat(tempInstr);
                                string bits2 = UbitString(tempInstr);
                                dataOutputFile << "0x" << std::hex << pc << " "
                                               << mc2 << " , " 
                                               << ("auipc " + tokens[1] + "," + to_string(num))
                                               << " # " << bits2 << "\n";
                                pc += 4;
                                num <<= 12;
                                int offset = (varmap[tokens[2]] - num - pc);
                                tokens[2] = to_string(offset) + "(" + tokens[1] + ")";
                            }
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
                        if(tokens.size()>3){
                            string mer="";
                            mer+=tokens[2];
                            mer+='(';
                            mer+=tokens[3];
                            mer+=')';
                            tokens[2]=mer;
                            while(tokens.size()>3) tokens.pop_back();
                        }
                        mc   = Sformat(tokens);
                        bits = SbitString(tokens);
                        dataOutputFile << "0x" << std::hex << pc << " "
                                       << mc << " , " << originalInstr
                                       << " # " << bits << "\n";
                        pc += 4;
                        break;
                    }
                    case 'b': {
                        if(tokens.size()>=4 && label.find(tokens[3]) != label.end()) {
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
                        if(tokens.size()>=3 && label.find(tokens[2]) != label.end()) {
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
                int sz = tokens[0].size();
                if(tokens[0][sz-1] == ':') {
                    tokens[0].pop_back();
                    if(label.find(tokens[0]) == label.end()) {
                        dataOutputFile << "Command not found" << endl;
                        break;
                    }
                }
            }
        }
    }

    // Finally, print data segment
    dataOutputFile << "\nData Segment\n";
    for (auto &it : dataSegment) {
        dataOutputFile << "0x" << std::hex << it.first 
                       << " 0x" << std::hex << it.second << "\n";
    }

    inputFile.close();
    dataOutputFile.close();
    return 0;
}
