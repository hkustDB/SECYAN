# Requirements
## For Debian Linux
 - build-essential (gcc >= 8)
 - cmake >= 3.12
 - libssl-dev
 - libgmp-dev
 - libboost-all-dev (Boost >= 1.66)

# Configure and Compile
``` bash
git clone --recurse-submodules https://github.com/Aqua-Dream/SECYAN
cd SECYAN
mkdir Release
cd Release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 8
```

# Run Demo
Switch to the output folder `Release/src/example`.
``` bash
# Server
> ./secyandemo
Who are you? [0. Server, 1. Client]: 0
Establishing connection... Finished!
Which query to run? [0. Q3, 1. Q10, 2. Q18, 3. Q8, 4. Q9]: 2
Which TPCH data size to use? [0. 1MB, 1. 3MB, 2. 10MB, 3. 33MB, 4. 100MB]: 2
Start running query...
Dummy Relation!

Running time: 5277ms
Communication cost: 266.873 MB
Finished!
```
``` bash
# Client
> ./secyandemo
Who are you? [0. Server, 1. Client]: 1
Establishing connection... Finished!
Which query to run? [0. Q3, 1. Q10, 2. Q18, 3. Q8, 4. Q9]: 2
Which TPCH data size to use? [0. 1MB, 1. 3MB, 2. 10MB, 3. 33MB, 4. 100MB]: 2
Start running query...
row_num o_custkey       o_orderkey      o_orderdate     o_totalprice    c_name annotation
1       667     29158   1995-10-21      439687.19       00000667        305
2       178     6882    1997-04-09      422359.62       00000178        303

Running time: 3714ms
Communication cost: 266.41 MB
Finished!
```

# Run Benchmark
Switch to the output folder `Release/src/example`.
``` bash
> ./benchmark
Usage: ./benchmark
 -r [Role: 0/1, default: 0 (SERVER), required]
 -a [IP-address, default: 127.0.0.1, optional]
 -p [Port (will use port & port+1), default: 7766, optional]
 -n [Number of test runs, default: 3, optional]
 -q [Query ID (3,10,18,8,9,0), default: 0, i.e. test all queries. , optional]

Program exiting
> ./benchmark -r 0 > result_server.txt &
> ./benchmark -r 1 > result_client.txt &
```

# Remark
 - SECYAN only read the last 8 digits of string from data file. For example, the the first row of column `c_name` in `customer.tbl` is `Customer#000000001`, but it will be outputted as `00000001`.
 - To use your own data, please refer to the format of files under `data` folder. SECYAN currently cannot generate annotations automatically. You need to write the annotation columns on your own.
 - To run your own query, please refer to the file `data/TPCH.cpp`. SECYAN currently cannot generate codes from SQL automatically. You need to rewrite your query into combinations of operators (`Aggregation`,`SemiJoin`,`Join`,etc.).
