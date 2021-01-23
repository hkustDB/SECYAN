#pragma once

#include "../core/relation.h"
#include <string>
#include <cstdint>
#include <vector>

using namespace SECYAN;

enum RelationName
{
    CUSTOMER,
    ORDERS,
    LINEITEM,
    PART,
    SUPPLIER,
    PARTSUPP,
    RTOTAL
};

enum QueryName
{
    Q3,
    Q10,
    Q18,
    Q8,
    Q9,
    QTOTAL
};

enum DataSize
{
    _1MB,
    _3MB,
    _10MB,
    _33MB,
    _100MB,
    DTOTAL
};

using run_query = void(DataSize, bool);
run_query run_Q3, run_Q10, run_Q18, run_Q8, run_Q9;