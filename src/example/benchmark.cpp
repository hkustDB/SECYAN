
#include <cstdint>
#include <string>
#include <functional>
#include "ENCRYPTO_utils/parse_options.h"
#include "TPCH.h"

using namespace std;
function<run_query> query_funcs[QTOTAL] = {run_Q3, run_Q10, run_Q18, run_Q8, run_Q9};
uint32_t QueryID[DTOTAL] = {3, 10, 18, 8, 9};

struct Stat
{
    uint64_t time;
    uint64_t cost;
};

// Get the average running time and cost of a single query
Stat SingleQuery(QueryName qn, DataSize ds, uint32_t numRepeat)
{
    Stat st;
    gParty.Tick("SingleQuery");
    for (uint32_t i = 0; i < numRepeat; i++)
        query_funcs[qn](ds, false);
    st.time = gParty.Tick("SingleQuery") / numRepeat;
    st.cost = gParty.GetCommCostAndResetStats() / numRepeat;
    return st;
}

void read_options(int32_t *argcp, char ***argvp, e_role *role, string *address, uint16_t *port, uint32_t *num_reps, uint32_t *qid)
{

    uint32_t int_role = 0, int_port = 0;

    parsing_ctx options[] = {
        {(void *)&int_role, T_NUM, "r", "Role: 0/1, default: 0 (SERVER)", true, false},
        {(void *)address, T_STR, "a", "IP-address, default: 127.0.0.1", false, false},
        {(void *)&int_port, T_NUM, "p", "Port (will use port & port+1), default: 7766", false, false},
        {(void *)num_reps, T_NUM, "n", "Number of test runs, default: 3", false, false},
        {(void *)qid, T_NUM, "q", "Query ID (3,10,18,8,9,0), default: 0, i.e. test all queries. ", false, false}};

    if (!parse_options(argcp, argvp, options, sizeof(options) / sizeof(parsing_ctx)))
    {
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(EXIT_SUCCESS);
    }

    if (int_role != 0 && int_role != 1)
    {
        cerr << "Role error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(EXIT_SUCCESS);
    }
    *role = (e_role)int_role;

    if (*qid != 3 && *qid != 10 && *qid != 18 && *qid != 8 && *qid != 9 && *qid != 0)
    {
        cerr << "Query id error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(EXIT_SUCCESS);
    }

    if (int_port == 0 || int_port > INT16_MAX)
    {
        cerr << "Port error!" << endl;
        print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
        exit(EXIT_SUCCESS);
    }
    *port = (uint16_t)int_port;
}

template <typename T>
void print_array(T *arr, uint32_t size)
{
    cout << "[";
    for (uint32_t i = 0; i < size - 1; i++)
        cout << arr[i] << ", ";
    cout << arr[size - 1] << "]" << endl;
}

int main(int argc, char **argv)
{
    e_role role = SERVER;
    uint16_t port = 7766;
    string address = "127.0.0.1";
    uint32_t qid = 0;
    uint32_t numreps = 3;
    read_options(&argc, &argv, &role, &address, &port, &numreps, &qid);
    uint32_t startid = 0, endid = QTOTAL;
    for (uint32_t i = 0; i < QTOTAL; i++)
    {
        if (QueryID[i] == qid)
        {
            startid = i;
            endid = i + 1;
            break;
        }
    }

    // role = CLIENT;
    // if(argc > 1)
    //     role = (e_role)(1-role);
    gParty.printTickTime = false;
    gParty.Init(address, port, role);
    double times[DTOTAL];
    double costs[DTOTAL];
    for (uint32_t i = startid; i < endid; i++)
    {
        auto qn = (QueryName)i;
        cout << "-------------- Query " << QueryID[i] << " --------------" << endl;
        for (uint32_t j = 0; j < DTOTAL; j++)
        {
            auto ds = (DataSize)j;
            auto st = SingleQuery(qn, ds, numreps);
            times[j] = st.time / 1000.0;
            costs[j] = st.cost / 1024 / 1024.0;
        }
        cout << "Running time (s): ";
        print_array(times, DTOTAL);
        cout << "Communication cost (MB): ";
        print_array(costs, DTOTAL);
        cout << endl;
    }

    return EXIT_SUCCESS;
}
