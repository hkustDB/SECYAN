#include <cstdint>
#include <string>
#include <functional>
#include "TPCH.h"

using namespace std;
function<run_query> query_funcs[QTOTAL] = {run_Q3, run_Q10, run_Q18, run_Q8, run_Q9};

int main(int argc, char **)
{
    int irole, iqn, ids;
    string address = "127.0.0.1";
    uint16_t port = 7766;
    e_role role = SERVER;

    cout << "Who are you? [0. Server, 1. Client]: ";
    cin >> irole;
    if (irole != 0 && irole != 1)
    {
        cerr << "Role error!" << endl;
        exit(1);
    }
    role = (e_role)irole;

    cout << "Establishing connection... ";
    gParty.Init(address, port, role);
    cout << "Finished!" << endl;

    QueryName qn;
    DataSize ds;
    cout << "Which query to run? [0. Q3, 1. Q10, 2. Q18, 3. Q8, 4. Q9]: ";
    cin >> iqn;
    if (iqn < 0 || iqn >= 5)
    {
        cerr << "Query selection error!" << endl;
        exit(1);
    }
    qn = (QueryName)iqn;
    cout << "Which TPCH data size to use? [0. 1MB, 1. 3MB, 2. 10MB, 3. 33MB, 4. 100MB]: ";
    cin >> ids;
    if (ids < 0 || ids >= 5)
    {
        cerr << "Data size selection error!" << endl;
        exit(1);
    }
    ds = (DataSize)ids;
    cout << "Start running query..." << endl;
    gParty.Tick("Running time");
    query_funcs[qn](ds, true);
    gParty.Tick("Running time");
    auto cost = gParty.GetCommCostAndResetStats();
    cout << "Communication cost: " << cost / 1024 / 1024.0 << " MB" << endl;
    cout << "Finished!" << endl;
    return 0;
}