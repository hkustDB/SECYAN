#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include "ENCRYPTO_utils/parse_options.h"
#include "../core/PSI.h"
#include "../core/party.h"

using namespace std;
using namespace SECYAN;

void read_options(int32_t *argcp, char ***argvp, e_role *role, string *address, uint16_t *port, string *infilepath, string *outfilepath)
{

    uint32_t int_role = 0, int_port = 0;

    parsing_ctx options[] = {
        {(void *)&int_role, T_NUM, "r", "Role: 0/1, default: 0 (SERVER)", true, false},
        {(void *)address, T_STR, "a", "IP-address, default: 127.0.0.1", false, false},
        {(void *)&int_port, T_NUM, "p", "Port (will use port & port+1), default: 7766", false, false},
        {(void *)infilepath, T_STR, "i", "Input data file path, default: input.txt", false, false},
        {(void *)outfilepath, T_STR, "o", "Output data file path, default: output.txt", false, false}};

    if (!parse_options(argcp, argvp, options, sizeof(options) / sizeof(parsing_ctx)))
    {
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(0);
    }

    if (int_role != 0 && int_role != 1)
    {
        cerr << "Role error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(0);
    }
    *role = (e_role)int_role;

    if(int_port == 0)
        int_port = 7766;
    if (int_port > INT16_MAX)
    {
        cerr << "Port error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(0);
    }
    *port = (uint16_t)int_port;

    ifstream in(infilepath->c_str());
    if(!in.good())
    {
        cerr << "Input file path error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(0);
    }
    in.close();
    ofstream out(outfilepath->c_str());
    if(!out.good())
    {
        cerr << "Output file path error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(0);
    }
    out.close();
}

unordered_set<uint32_t> read_ids_from_file(string filepath)
{
    int line_num = 0;
    unordered_set<uint32_t> out;
    uint32_t value;
    ifstream fin(filepath.c_str(), ios::in);
    if(!fin.is_open())
    {
        cerr << "Open input file error!" << endl;
        exit(1);
    }
    string line;
    while (getline(fin, line))
    {
        line_num++;
        try
        {
            value = stoi(line);
            out.insert(value);
        }
        catch(...)
        {
            cerr << "Read line " << line_num << "error!" << endl;
        }
    }
    fin.close();
    return out;
}

void write_ids_to_file(vector<uint32_t> &ids, string filepath)
{
    ofstream fout(filepath.c_str(), ios::out);
    if(!fout.is_open())
    {
        cerr << "Open output file error!" << endl;
        exit(1);
    }
    for(auto it = ids.begin(); it != ids.end(); it++)
        fout << (*it) << endl;
    fout.close();
}

vector<uint32_t> serverPSI(vector<uint64_t> &data)
{
    
    uint32_t AliceSetSize = data.size();
    uint32_t BobSetSize;
    gParty.Send(&AliceSetSize, 1);
    gParty.Recv(&BobSetSize, 1);
    PSI psi(data, AliceSetSize, BobSetSize, PSI::Alice);
    vector<uint32_t> indicator = psi.Intersect();
    vector<uint32_t> indicator2;
    gParty.Recv(indicator2);

    vector<uint32_t> intersectSet;
    auto perm = psi.CuckooToAliceArray();
    for(uint32_t i = 0; i < AliceSetSize; i++)
    {
        uint32_t index = perm[i];
        if(indicator[index] ^ indicator2[index])
            intersectSet.push_back(data[i]);
    }
    gParty.Send(intersectSet);
    return intersectSet;
}

vector<uint32_t> clientPSI(vector<uint64_t> &data)
{
    uint32_t AliceSetSize;
    uint32_t BobSetSize = data.size();
    gParty.Recv(&AliceSetSize, 1);
    gParty.Send(&BobSetSize, 1);
    PSI psi(data, AliceSetSize, BobSetSize, PSI::Bob);
    vector<uint32_t> indicator = psi.Intersect();
    gParty.Send(indicator);

    vector<uint32_t> intersectSet;
    gParty.Recv(intersectSet);
    return intersectSet;
}

int main(int argc, char **argv)
{
    e_role role = SERVER;
    uint16_t port = 7766;
    string address = "127.0.0.1";
    string infilepath = "input.txt";
    string outfilepath = "output.txt";
    read_options(&argc, &argv, &role, &address, &port, &infilepath, &outfilepath);

    cout << "Reading data from file..." << flush;
    unordered_set<uint32_t> ids = read_ids_from_file(infilepath);
    cout << " Finished!" << endl;
    vector<uint64_t> data;
    data.assign(ids.begin(), ids.end());

    cout << "Exchanging data securely..." << flush;
    gParty.Tick("Exchanging data");
    gParty.Init(address, port, role);
    vector<uint32_t> intersectSet = (role == SERVER)? serverPSI(data): clientPSI(data);
    cout << " Finished!" << endl;
    gParty.Tick("Exchanging data");

    cout << "Writing data to file..." << flush;
    write_ids_to_file(intersectSet, outfilepath);
    cout << " Finished!" << endl;

    cout << "Press enter to exit...";
    cin.get();
    return 0;
}
