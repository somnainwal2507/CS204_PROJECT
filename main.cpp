#include <bits/stdc++.h>
#include "format.cpp"
using namespace std;
#define ll long long int

// Starting addresses for segments
int dataAddress = 0x10000000;
int pc = 0;

int main() {
    // Maps for variable addresses, data segment contents, and labels.
    map<string,int> varmap;
    map<int,int> dataSegment;
    map<string,int> label;

    ifstream inputFile("input.asm");
    ofstream dataOutputFile("output.mc");
    string line;
    bool inData = false, inText = false;

    // === PASS 1: Process data segment directives ===
    while (getline(inputFile, line)) {
        if(line.empty()) continue;
        stringstream ss(line);
        vector<string> tokens;
        string token;
        // Tokenize the line (stop at comment)
        while(ss >> token) {
            if(token[0] == '#') break;
            tokens.push_back(token);
        }
        if(tokens.empty()) continue;
        if(tokens[0] == ".data") {
            inData = true;
            inText = false;
            continue;
        }
        else if(tokens[0] == ".text") {
            inText = true;
            inData = false;
            break; // End of data segment pass.
        }
        if(inData) {
            // Remove ':' from variable label if present.
            if(tokens[0].back() == ':') {
                tokens[0].pop_back();
            }
            // Process directives: .byte, .half, .word, .dword, .asciz
            if(datatype_map.find(tokens[1]) != datatype_map.end()) {
                varmap[tokens[0]] = dataAddress;
                int typeSize = datatype_map[tokens[1]];
                // Process all numbers following the directive.
                for (size_t i = 2; i < tokens.size(); i++) {
                    int n;
                    if(tokens[i].substr(0,2) == "0x")
                        n = stoi(tokens[i].substr(2), nullptr, 16);
                    else
                        n = stoi(tokens[i]);
                    // Store each byte (little-endian)
                    for(int j = 0; j < typeSize; j++) {
                        dataSegment[dataAddress++] = n & 0xFF;
                        n >>= 8;
                    }
                }
            }
            else if(tokens[1] == ".asciz") {
                // Expect exactly one string literal.
                if(tokens.size() == 3) {
                    varmap[tokens[0]] = dataAddress;
                    string s = tokens[2];
                    // Remove surrounding quotes.
                    s.erase(remove(s.begin(), s.end(), '\"'), s.end());
                    for(char c : s) {
                        dataSegment[dataAddress++] = c;
                    }
                    // Null termination.
                    dataSegment[dataAddress++] = 0;
                }
            }
        }
    }

    // === PASS 2: Process labels in the text segment ===
    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    inText = false;
    while(getline(inputFile, line)) {
        if(line.empty()) continue;
        stringstream ss(line);
        vector<string> tokens;
        string token;
        ss >> token;
        if(token == ".text") {
            inText = true;
            continue;
        }
        if(token == ".data") {
            inText = false;
            continue;
        }
        if(inText) {
            // If a token ends with a colon, it's a label.
            if(token.back() == ':') {
                token.pop_back();
                label[token] = pc;
            }
            else {
                // Assume one instruction per line (ignoring extra tokens after the instruction)
                pc += 4;
            }
        }
    }

    // === PASS 3: Assemble text segment instructions ===
    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    pc = 0;
    inText = false;
    while(getline(inputFile, line)) {
        if(line.empty()) continue;
        stringstream ss(line);
        vector<string> tokens;
        string token;
        // Simple tokenization: split on spaces and commas (stop at comment)
        while(ss >> token) {
            if(token[0] == '#') break;
            // Remove commas if present.
            if(token.back() == ',') {
                token.pop_back();
            }
            tokens.push_back(token);
        }
        if(tokens.empty()) continue;
        if(tokens[0] == ".text") {
            inText = true;
            continue;
        }
        else if(tokens[0] == ".data") {
            inText = false;
            continue;
        }
        if(inText) {
            // Handle label definition lines
            if(tokens[0].back() == ':') {
                // Label definition; skip to next line.
                continue;
            }
            // Process branch or jump labels by replacing them with relative offsets.
            if(tokens.size() > 3 && (type_map[tokens[0]] == 'b' || type_map[tokens[0]] == 'j')) {
                if(label.find(tokens.back()) != label.end()) {
                    int offset = label[tokens.back()] - pc;
                    tokens.back() = to_string(offset);
                }
            }
            // Special handling for load pseudo-instructions referring to variables.
            if(tokens[0][0] == 'l' && type_map.find(tokens[0]) != type_map.end()) {
                // If the second token is a register and third is a variable.
                if(varmap.find(tokens[2]) != varmap.end()) {
                    int upper = dataAddress >> 12;
                    vector<string> auipcInst = {"auipc", tokens[1], to_string(upper)};
                    dataOutputFile << "0x" << hex << pc << " " << Uformat(auipcInst) << " , auipc " << tokens[1] << ", " << upper << " # opcode details" << endl;
                    int base = upper << 12;
                    int offset = varmap[tokens[2]] - base - pc;
                    pc += 4;
                    tokens[2] = to_string(offset) + "(" + tokens[1] + ")";
                }
            }
            // Select and call the appropriate formatting function.
            char type = type_map[tokens[0]];
            string machineCode;
            switch(type) {
                case 'r':
                    machineCode = Rformat(tokens);
                    break;
                case 'i':
                    machineCode = Iformat(tokens);
                    break;
                case 's':
                    machineCode = Sformat(tokens);
                    break;
                case 'b':
                    machineCode = SBformat(tokens);
                    break;
                case 'u':
                    machineCode = Uformat(tokens);
                    break;
                case 'j':
                    machineCode = UJformat(tokens);
                    break;
                default:
                    machineCode = "Error";
            }
            dataOutputFile << "0x" << hex << pc << " " << machineCode << " , ";
            // Reconstruct the assembly instruction for output.
            for (size_t i = 0; i < tokens.size(); i++) {
                dataOutputFile << tokens[i];
                if(i < tokens.size()-1)
                    dataOutputFile << ", ";
            }
            dataOutputFile << " # opcode details" << endl;
            pc += 4;
        }
    }

    // Write termination code for text segment.
    dataOutputFile << "0x" << hex << pc << " " << "0x00000000" << " , termination" << endl;

    // Write the data segment.
    dataOutputFile << "\nData Segment" << endl;
    for(auto it: dataSegment) {
        dataOutputFile << "0x" << hex << it.first << " 0x" << hex << it.second << endl;
    }

    inputFile.close();
    dataOutputFile.close();
    return 0;
}
