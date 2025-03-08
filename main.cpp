#include <bits/stdc++.h>
#include "format.cpp"
using namespace std;
#define ll long long int

int data = 0x10000000;
int dataAddress = data;
int pc = 0;



int main() {

    map<string,int> varmap;
    map<int,int> dataSegment;
    map<string,int> label;

    ifstream inputFile("input.asm");
    ofstream dataOutputFile("output.mc");
    //ofstream dataTokenFile("output_token.mc");
    string line;
    int flag=0,comment=0;

    while (getline(inputFile, line))
    {
        if (line.empty()) 
        {
            continue;
        }

        stringstream ss(line);
        vector<string> tokens;
        string token,temp;
        while (ss >> token) 
        {
            for(char c: token)
            {
                if(c == '#')
                {
                    if(!temp.empty() && !comment)
                    {
                        tokens.push_back(temp);
                        //dataTokenFile<<temp<<endl;
                    }
                    temp="";
                    comment=1;
                    break;
                }
                else if(c == ',')
                {
                    tokens.push_back(temp);
                    //dataTokenFile<<temp<<endl;
                    temp="";
                }
                else
                {
                    temp+=c;
                }     
            }
            if(!temp.empty() && !comment)
            {
                tokens.push_back(temp);
                //dataTokenFile<<temp<<endl;
            }
            temp="";
            if(comment)
            {
                break;
            }
        }

        comment = 0;

        if(tokens.empty())
        {
            continue;
        }
        //dataTokenFile<<token<<endl;
        if (tokens[0] == ".text") 
        {
            flag=0;
            break;
        } 
        else if (tokens[0] == ".data") 
        {
            flag=1;
            continue;
        }
        if(flag)
        {
            tokens[0].pop_back();
            if (tokens[1][0] == '.') 
            {    
                if(datatype_map.find(tokens[1])!=datatype_map.end())
                {
                    varmap[tokens[0]] = dataAddress;
                    int i=2;
                    string s = tokens[i];
                    while(i!=tokens.size())
                    {
                        int n;
                        if(s[0]=='0'&&(s[1]=='x'||s[1]=='X'))
                        {
                            n = stoi(s.substr(2), nullptr, 16);
                        }
                        else
                        {
                            n = stoi(s);
                        }
                        int j=datatype_map[tokens[1]];
                        while(j--)
                        {
                            dataSegment[dataAddress] = n&511;
                            n=n>>8;
                            dataAddress += 1;
                        }
                        i++;
                        if(i!=tokens.size())
                        {
                            s=tokens[i];
                        }
                        
                    }   
                }
                else if(tokens[1] == ".asciiz")
                {
                    if(tokens.size()!=3)
                    {
                        dataOutputFile<<"Error for "<<tokens[0]<<endl;
                        break;
                    }
                    varmap[tokens[0]] = dataAddress;
                    string s = tokens[2];
                    for (char c : s) 
                    {
                        if(c=='"')
                        {
                            continue;
                        }
                        dataSegment[dataAddress] = c;
                        dataAddress += 1;
                    }
                    dataSegment[dataAddress] = 0;
                    dataAddress += 1;
                }
            }  
        }
    }



    inputFile.clear();             
    inputFile.seekg(0, ios::beg);
    flag=0;
    comment=0;


    while(getline(inputFile, line))
    {
        if (line.empty()) 
        {
            continue;
        }
        else
        {
            stringstream ss(line);
            vector<string> tokens;
            string token;
            while (ss >> token) 
            {
                //dataTokenFile<<token<<endl;
                if(token[0] == '#')
                {
                    break;
                }
                if (token == ".text") 
                {
                    flag=0;
                    break;
                } 
                else if (token == ".data") 
                {
                    flag=1;
                    break;
                }
                int a = token.size();
                if(token[a-1]==':' && !flag){
                    token.pop_back();
                    label[token]=pc;
                    //cout<<"Stored"<<endl;
                }
                else if(!flag){
                    pc+=4;
                    if(token[0]=='l' && type_map.find(token)!=type_map.end()){
                        ss>>token;
                        ss>>token;
                        if(varmap.find(token)!=varmap.end()){
                            pc+=4;
                        }
                    }
                }
                break;
            }
        }
    }

    

    inputFile.clear();             
    inputFile.seekg(0, ios::beg);

    pc=0;
    flag=0;
    comment=0;

    while (getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        }

        stringstream ss(line);
        vector<string> tokens;
        string token,temp;
        while (ss >> token) {
            for(char c: token){
                if(c == '#'){
                    if(!temp.empty() && !comment){
                        tokens.push_back(temp);
                        //dataTokenFile<<temp<<endl;
                    }
                    temp="";
                    comment=1;
                    break;
                }
                else if(c == ','){
                    tokens.push_back(temp);
                    //dataTokenFile<<temp<<endl;
                    temp="";
                }else{
                    temp+=c;
                }     
            }
            if(!temp.empty() && !comment){
                tokens.push_back(temp);
                //dataTokenFile<<temp<<endl;
            }
            temp="";
            if(comment){
                break;
            }
        }

        comment = 0;

        if(tokens.empty()){
            continue;
        }


        if (tokens[0] == ".text") 
        {
            flag=0;
            continue;
        } 
        else if (tokens[0] == ".data") 
        {
            flag=1;
            continue;
        }

        if(!flag){
            if(type_map.find(tokens[0])!=type_map.end())
            {
                char type = type_map[tokens[0]];
                switch(type){
                case 'r':
                    {
                        dataOutputFile<<"0x"<<std::hex<<pc<<" "<<Rformat(tokens)<<endl;
                        pc+=4;
                        break;
                    }
                case 'i':
                    {
                        if(varmap.find(tokens[2])!=varmap.end() && tokens[0][0]=='l'){
                            int num = data>>12;
                            vector<string> temp = {"auipc",tokens[1],to_string(num)};
                            dataOutputFile<<"0x"<<std::hex<<pc<<" "<<Uformat(temp)<<endl;             
                            num = num<<12;
                            int n = (varmap[tokens[2]]-num-pc);
                            pc+=4;
                            tokens[2]=to_string(n)+"("+tokens[1]+")";
                        }
                        dataOutputFile<<"0x"<<std::hex<<pc<<" "<<Iformat(tokens)<<endl;
                        pc+=4;
                        break;
                    }
                case 's':
                    {
                        dataOutputFile<<"0x"<<std::hex<<pc<<" "<<Sformat(tokens)<<endl;
                        pc+=4;
                        break;
                    }
                case 'b':
                    {
                        if(label.find(tokens[3])!=label.end()){
                            tokens[3]=to_string(label[tokens[3]]-pc);
                        }
                        dataOutputFile<<"0x"<<std::hex<<pc<<" "<<SBformat(tokens)<<endl;
                        pc+=4;
                        break;
                    }
                case 'u':
                    {
                        dataOutputFile<<"0x"<<std::hex<<pc<<" "<<Uformat(tokens)<<endl;
                        pc+=4;
                        break;
                    }
                case 'j':
                    {
                        if(label.find(tokens[2])!=label.end()){
                            tokens[2]=to_string(label[tokens[2]]-pc);
                        }
                        dataOutputFile<<"0x"<<std::hex<<pc<<" "<<UJformat(tokens)<<endl;
                        pc+=4;
                        break;
                    }    
                default:
                {
                    dataOutputFile<<"Error"<<endl;
                }
                }
            }else {
                tokens[0].pop_back();
                if(label.find(tokens[0])==label.end()){
                    dataOutputFile<<"Command not found"<<endl;
                    break;
                }
            }
        }
    }

    dataOutputFile<<endl<<"Data Segment"<<endl;

    for(auto it: dataSegment)
    {
        dataOutputFile<<"0x"<<std::hex<<it.first<<" 0x"<<std::hex<<it.second<<endl;
    }

    // for(auto it: label)
    // {
    //     dataOutputFile<<it.first<<" "<<it.second<<endl;
    // }

    // for(auto it: varmap)
    // {
    //     dataOutputFile<<it.first<<" "<<std::hex<<it.second<<endl;
    // }

    inputFile.close();
    dataOutputFile.close();
    //dataTokenFile.close();

    return 0;
}
