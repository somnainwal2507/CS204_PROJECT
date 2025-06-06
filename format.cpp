
#include <bits/stdc++.h>
#include "lookup.cpp"
using namespace std;
#define ll long long int

//Helper function for immediate values
ll parseImmediate(const string &immStr) {
    bool negative = false;
    string token = immStr;
    if(!token.empty() && token[0] == '-') {
        negative = true;
        token = token.substr(1);
    }
    if(token.size() >= 3 && token.front() == '\'' && token.back() == '\'') {
        ll val = static_cast<ll>(token[1]);
        return negative ? -val : val;
    }
    if(token.size() >= 2 && token[0] == '0' && (token[1]=='x' || token[1]=='X')) {
        ll val = stoll(token.substr(2), nullptr, 16);
        return negative ? -val : val;
    }
    ll val = stoll(token);
    return negative ? -val : val;
}



string Rformat (vector <string> &instruction)
{
    // Validate instruction format
    if (instruction.size() != 4) {
        return "Invalid instruction format";
    }

    string opcode_temp = codes_map[instruction[0]][0];
    string func3_temp = codes_map[instruction[0]][1];
    string func7_temp = codes_map[instruction[0]][2];

    int opcode = stoi(opcode_temp, nullptr, 16);
    int func3 = stoi(func3_temp, nullptr, 16);
    int func7 = stoi(func7_temp, nullptr, 16);

    int rd = stoi(instruction[1].substr(1)); 
    int rs1 = stoi(instruction[2].substr(1)); 
    int rs2 = stoi(instruction[3].substr(1)); 

    ll machineCode = (func7 << 25) | (rs2 << 20) | (rs1 << 15) | (func3 << 12) | (rd << 7) | opcode;
    stringstream ss;
    ss << hex << "0x" << uppercase << std::setw(8) << std::setfill('0') << machineCode;
    return ss.str(); 
}



string Iformat(vector<string> &instruction)
{
    // Determine if we are dealing with a load instruction (which has a different token count)
    int flag = 0;
    if (instruction.size() != 4) {
        vector<string> temp = {"lb", "ld", "lh", "lw"};
        if (instruction.size() == 3 && find(temp.begin(), temp.end(), instruction[0]) != temp.end())
            flag = 1;
        else
            return "Invalid instruction format";
    }

    string opcode_temp = codes_map[instruction[0]][0];
    string func3_temp  = codes_map[instruction[0]][1];

    int opcode = stoi(opcode_temp, nullptr, 16);
    int func3  = stoi(func3_temp, nullptr, 16);
    int rd     = stoi(instruction[1].substr(1));
    int rs1;
    ll rawImm; // the immediate value as provided

    if (!flag)
    {   
        rs1 = stoi(instruction[2].substr(1));
        // Use the helper to parse the immediate (works for numbers and char literals)
        rawImm = parseImmediate(instruction[3]);
    }
    else
    {
        // For load instructions 
        string str = instruction[2];
        size_t start_pos = str.find('(');
        size_t end_pos = str.find(')');
        string imm_str = str.substr(0, start_pos);
        rawImm = parseImmediate(imm_str);
        rs1 = stoi((str.substr(start_pos + 1, end_pos - start_pos - 1)).substr(1));
    }

    if (rawImm > 2047 || rawImm < -2048) {
        return "Immediate value out of bounds";
    }

    ll imm;
    if (rawImm < 0)
        imm = (1 << 12) + rawImm;  
    else
        imm = rawImm;

    ll machineCode = (imm << 20) | (rs1 << 15) | (func3 << 12) | (rd << 7) | opcode;
    stringstream ss;
    ss << hex << "0x" << uppercase << setw(8) << setfill('0') << machineCode;
    return ss.str();
}



string Sformat (vector <string> &instruction)
{
    if (instruction.size() != 3)
        return "Invalid instruction format";

    string opcode_temp = codes_map[instruction[0]][0];
    string func3_temp = codes_map[instruction[0]][1];


    int opcode = stoi(opcode_temp, nullptr, 16);
    int func3 = stoi(func3_temp, nullptr, 16);
    int rs2 = stoi(instruction[1].substr(1));
    int rs1;
    ll imm;


    string str = instruction[2];
    size_t start_pos = str.find('(');
    size_t end_pos = str.find(')');


    string imm_str = str.substr(0,start_pos);
    int flag2 = 0;
    if (imm_str[0] == '-'){
        imm_str = imm_str.substr(1);
        flag2 = 1;
    }

    if (imm_str[0] == '0' && (imm_str[1] == 'x' || imm_str[1] == 'X')){
        imm = stol(imm_str.substr(2), nullptr, 16);
    }
    else{
        imm = stol(imm_str);

    }

    if (flag2) imm *= -1;
    if (imm<0){
        imm = (1 << 12) + imm;
        imm = imm & 0xFFF;
    }

    

    ll imm1,imm2;
    imm1 = (imm>>5) & 127;
    imm2 = (imm) & 31;

    rs1 = stoi((str.substr(start_pos+1,end_pos - start_pos -1)).substr(1));


    ll machineCode = (imm1 << 25) | (rs2 << 20) | (rs1 << 15) | (func3 << 12) | (imm2 << 7) | opcode;
    stringstream ss;
    ss << hex << "0x" << uppercase << std::setw(8) << std::setfill('0') << machineCode;
    return ss.str();
}



string SBformat (vector <string> &instruction)
{
    if (instruction.size() != 4)
        return "Invalid instruction format";

    string opcode_temp = codes_map[instruction[0]][0];
    string func3_temp = codes_map[instruction[0]][1];


    int opcode = stoi(opcode_temp, nullptr, 16);
    int func3 = stoi(func3_temp, nullptr, 16);
    int rs1 = stoi(instruction[1].substr(1));
    int rs2 = stoi(instruction[2].substr(1));
    ll imm;

    string imm_str = instruction[3];
    int flag2 = 0;
    if (imm_str[0] == '-'){
        imm_str = imm_str.substr(1);
        flag2 = 1;
    }

    if (imm_str[0] == '0' && (imm_str[1] == 'x' || imm_str[1] == 'X')){
        imm = stol(imm_str.substr(2), nullptr, 16);
    }
    else{
        imm = stol(imm_str);

    }

    if (flag2) imm *= -1;
    if (imm<0){
        imm = (1 << 20) + imm;
        imm = imm & 0xFFFFF;
    }

    //shuffling part
    ll imm1=0,imm2=0;
    imm1 |= ((imm>>12) & 1)<<6;
    imm1 |= ((imm>>5) & 0x3f);
    imm2 |= ((imm>>1) & 0xf)<<1;
    imm2 |= ((imm>>11) & 1);

    ll machineCode = (imm1 << 25) | (rs2 << 20) | (rs1 << 15) | (func3 << 12) | (imm2 << 7) | opcode;
    stringstream ss;
    ss << hex << "0x" << uppercase << std::setw(8) << std::setfill('0') << machineCode;
    return ss.str();
}



string Uformat (vector <string> &instruction)
{
    if (instruction.size() != 3)
        return "Invalid instruction format";

    string opcode_temp = codes_map[instruction[0]][0];
    int opcode = stoi(opcode_temp, nullptr, 16);
    int rd = stoi(instruction[1].substr(1));
    ll imm;

    string imm_str = instruction[2];
    int flag2 = 0;
    if (imm_str[0] == '-'){
        imm_str = imm_str.substr(1);
        flag2 = 1;
    }

    if (imm_str[0] == '0' && (imm_str[1] == 'x' || imm_str[1] == 'X')){
        imm = stol(imm_str.substr(2), nullptr, 16);
    }
    else{
        imm = stol(imm_str);

    }

    if (flag2) imm *= -1;
    if (imm<0){
        imm = (1 << 20) + imm;
        imm = imm & 0xFFFFF;
    }


    ll machineCode = (imm << 12) | (rd << 7) | opcode;
    stringstream ss;
    ss << hex << "0x" << uppercase << std::setw(8) << std::setfill('0') << machineCode;
    return ss.str();

}



string UJformat (vector <string> &instruction)
{
    if (instruction.size() != 3)
        return "Invalid instruction format";

    string opcode_temp = codes_map[instruction[0]][0];
    int opcode = stoi(opcode_temp, nullptr, 16);
    int rd = stoi(instruction[1].substr(1));
    ll imm;

    string imm_str = instruction[2];
    int flag2 = 0;
    if (imm_str[0] == '-'){
        imm_str = imm_str.substr(1);
        flag2 = 1;
    }

    if (imm_str[0] == '0' && (imm_str[1] == 'x' || imm_str[1] == 'X')){
        imm = stol(imm_str.substr(2), nullptr, 16);
    }
    else{
        imm = stol(imm_str);

    }

    if (flag2) imm *= -1;
    if (imm<0){
        imm = (1 << 20) + imm;
        imm = imm & 0xFFFFF;
    }

    //shuffling part
    ll shuffled_imm = 0;
    shuffled_imm |= ((imm>>20) & 1)<<20; //20th bit
    shuffled_imm |= ((imm>>1) & 0x3ff)<<10; //19th - 10th bit
    shuffled_imm |= ((imm>>11) & 1)<<9; //9th bit
    shuffled_imm |= ((imm>>12) & 0xff)<<1; //8th - 1th bit

    shuffled_imm >>=1;//discarding 0th bit
    if (flag2) shuffled_imm |= 1<<19; //for negative values it should be sign extended


    ll machineCode = (shuffled_imm << 12) | (rd << 7) | opcode;
    stringstream ss;
    ss << hex << "0x" << uppercase << std::setw(8) << std::setfill('0') << machineCode;
    return ss.str();

}
