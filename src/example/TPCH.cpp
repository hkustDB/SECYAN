#include <vector>
#include <string>
#include "../core/relation.h"
#include <iostream>
#include <chrono>
#include "TPCH.h"

using namespace std;
using namespace SECYAN;

size_t NumRows[RTOTAL][DTOTAL] = {
	{150, 450, 1500, 4950, 15000},
	{1500, 4500, 15000, 49500, 150000},
	{6005, 17973, 60175, 198344, 595215},
	{200, 600, 2000, 6600, 20000},
	{10, 30, 100, 330, 1000},
	{800, 2400, 8000, 26400, 80000}};

std::vector<std::string> AttrNames[RTOTAL][QTOTAL] = {
	{{"c_custkey"}, {"c_custkey", "c_name", "c_nationkey"}, {"c_custkey", "c_name"}, {"c_custkey"}, {}},
	{{"o_custkey", "o_orderkey", "o_orderdate", "o_shippriority"}, {"o_custkey", "o_orderkey"}, {"o_custkey", "o_orderkey", "o_orderdate", "o_totalprice"}, {"o_orderkey", "o_custkey", "o_year"}, {"o_orderkey", "o_year"}},
	{{"l_orderkey"}, {"l_orderkey"}, {"l_orderkey"}, {"l_orderkey", "l_suppkey", "l_partkey"}, {"l_orderkey", "l_suppkey", "l_partkey"}},
	{{}, {}, {}, {"p_partkey"}, {"p_partkey"}},
	{{}, {}, {}, {"s_suppkey"}, {"s_suppkey"}},
	{{}, {}, {}, {}, {"ps_partkey", "ps_suppkey"}}};

std::vector<Relation::DataType> AttrTypes[RTOTAL][QTOTAL] = {
	{{Relation::INT}, {Relation::INT, Relation::STRING, Relation::INT}, {Relation::INT, Relation::STRING}, {Relation::INT}, {}},
	{{Relation::INT, Relation::INT, Relation::DATE, Relation::STRING}, {Relation::INT, Relation::INT}, {Relation::INT, Relation::INT, Relation::DATE, Relation::DECIMAL}, {Relation::INT, Relation::INT, Relation::INT}, {Relation::INT, Relation::INT}},
	{{Relation::INT}, {Relation::INT}, {Relation::INT}, {Relation::INT, Relation::INT, Relation::INT}, {Relation::INT, Relation::INT, Relation::INT}},
	{{}, {}, {}, {Relation::INT}, {Relation::INT}},
	{{}, {}, {}, {Relation::INT}, {Relation::INT}},
	{{}, {}, {}, {}, {Relation::INT, Relation::INT}}};

std::string filename[] = {
	"customer.tbl",
	"orders.tbl",
	"lineitem.tbl",
	"part.tbl",
	"supplier.tbl",
	"partsupp.tbl"};

std::string datapath[] = {
	"../../../data/1MB/",
	"../../../data/3MB/",
	"../../../data/10MB/",
	"../../../data/33MB/",
	"../../../data/100MB/"};

inline Relation::RelationInfo GetRI(RelationName rn, QueryName qn, DataSize ds, e_role owner)
{
	Relation::RelationInfo ri = {
		owner,
		false,
		AttrNames[rn][qn],
		AttrTypes[rn][qn],
		NumRows[rn][ds],
		false};
	return ri;
}

inline std::string GetFilePath(RelationName rn, DataSize ds)
{
	return datapath[ds] + filename[rn];
}

void run_Q3(DataSize ds, bool printResult)
{
	vector<string> o_groupBy = {"o_orderkey", "o_orderdate", "o_shippriority"};
	auto cust_ri = GetRI(CUSTOMER, Q3, ds, SERVER);
	Relation::AnnotInfo cust_ai = {true, true};
	Relation customer(cust_ri, cust_ai);
	auto filePath = GetFilePath(CUSTOMER, ds);
	customer.LoadData(filePath.c_str(), "q3_annot");

	auto orders_ri = GetRI(ORDERS, Q3, ds, CLIENT);
	Relation::AnnotInfo orders_ai = {true, true};
	Relation orders(orders_ri, orders_ai);
	filePath = GetFilePath(ORDERS, ds);
	orders.LoadData(filePath.c_str(), "q3_annot");
	//orders.Print();

	auto lineitem_ri = GetRI(LINEITEM, Q3, ds, SERVER);
	Relation::AnnotInfo lineitem_ai = {false, true};
	Relation lineitem(lineitem_ri, lineitem_ai);
	filePath = GetFilePath(LINEITEM, ds);
	lineitem.LoadData(filePath.c_str(), "q3_annot");
	lineitem.Aggregate();

	orders.SemiJoin(customer, "o_custkey", "c_custkey");
	//orders.PrintTableWithoutRevealing("orders semijoin customer");
	// PSI: 95.4267ms
	// Two OEPs: 53.8459ms
	// AnnotMul: 4.4265ms

	orders.SemiJoin(lineitem, "o_orderkey", "l_orderkey");
	//orders.PrintTableWithoutRevealing("orders semijoin lineitem");
	// PSI: 162.877ms
	// Two OEPs: 25.8395ms
	// AnnotMul: 51.1201ms

	orders.Aggregate(o_groupBy);
	orders.RevealAnnotToOwner();
	if (printResult)
		orders.Print();
}

void run_Q10(DataSize ds, bool printResult)
{
	auto cust_ri = GetRI(CUSTOMER, Q10, ds, SERVER);
	Relation::AnnotInfo cust_ai = {true, true};
	Relation customer(cust_ri, cust_ai);
	auto filePath = GetFilePath(CUSTOMER, ds);
	customer.LoadData(filePath.c_str(), "q10_annot");

	auto orders_ri = GetRI(ORDERS, Q10, ds, CLIENT);
	Relation::AnnotInfo orders_ai = {true, true};
	Relation orders(orders_ri, orders_ai);
	filePath = GetFilePath(ORDERS, ds);
	orders.LoadData(filePath.c_str(), "q10_annot");

	auto lineitem_ri = GetRI(LINEITEM, Q10, ds, SERVER);
	Relation::AnnotInfo lineitem_ai = {false, true};
	Relation lineitem(lineitem_ri, lineitem_ai);
	filePath = GetFilePath(LINEITEM, ds);
	lineitem.LoadData(filePath.c_str(), "q10_annot");
	lineitem.Aggregate();

	orders.SemiJoin(lineitem, "o_orderkey", "l_orderkey");
	orders.Aggregate("o_custkey");
	customer.SemiJoin(orders, "c_custkey", "o_custkey");
	customer.RevealAnnotToOwner();
	if (printResult)
		customer.Print();
}

void run_Q18(DataSize ds, bool printResult)
{
	auto cust_ri = GetRI(CUSTOMER, Q18, ds, SERVER);
	Relation::AnnotInfo cust_ai = {true, true};
	Relation customer(cust_ri, cust_ai);
	auto filePath = GetFilePath(CUSTOMER, ds);
	customer.LoadData(filePath.c_str(), "q18_annot");
	//customer.PrintTableWithoutRevealing("customer");

	auto orders_ri = GetRI(ORDERS, Q18, ds, CLIENT);
	Relation::AnnotInfo orders_ai = {true, true};
	Relation orders(orders_ri, orders_ai);
	filePath = GetFilePath(ORDERS, ds);
	orders.LoadData(filePath.c_str(), "q18_annot");
	//orders.PrintTableWithoutRevealing("orders");

	auto lineitem_ri = GetRI(LINEITEM, Q18, ds, SERVER);
	Relation::AnnotInfo lineitem_ai = {false, true};
	Relation lineitem(lineitem_ri, lineitem_ai);
	filePath = GetFilePath(LINEITEM, ds);
	lineitem.LoadData(filePath.c_str(), "q18_annot");
	lineitem.Aggregate();
	//lineitem.PrintTableWithoutRevealing("lineitem");

	//gParty.Tick("orders semijoin lineitem");
	orders.SemiJoin(lineitem, "o_orderkey", "l_orderkey");
	//orders.PrintTableWithoutRevealing("orders");
	//gParty.Tick("orders semijoin lineitem");

	Relation customer_bool_annot = customer;
	customer_bool_annot.Project("c_custkey");
	customer_bool_annot.AnnotOrAgg();
	//customer.PrintTableWithoutRevealing("customer bool");

	//gParty.Tick("orders annot bool");
	Relation orders_bool_annot = orders;
	orders_bool_annot.Project("o_custkey");
	orders_bool_annot.AnnotOrAgg();
	//orders_bool_annot.PrintTableWithoutRevealing("orders bool");
	//gParty.Tick("orders annot bool");

	//gParty.Tick("orders <== semijoin ==> customer");
	orders.SemiJoin(customer_bool_annot, "o_custkey", "c_custkey");
	//orders.PrintTableWithoutRevealing("orders");
	customer.SemiJoin(orders_bool_annot, "c_custkey", "o_custkey");
	//customer.PrintTableWithoutRevealing("customer");
	//gParty.Tick("orders <== semijoin ==> customer");

	//gParty.Tick("Reveal customer and orders");
	customer.RemoveZeroAnnotatedTuples();
	customer.RevealTuples();
	orders.RemoveZeroAnnotatedTuples();
	orders.RevealTuples();
	//orders.PrintTableWithoutRevealing("orders");
	//customer.PrintTableWithoutRevealing("customer");
	//gParty.Tick("Reveal customer and orders");

	orders.Join(customer, "o_custkey", "c_custkey");
	orders.RevealAnnotToOwner();
	if (printResult)
		orders.Print();
}

void run_Q8(DataSize ds, bool printResult)
{
	auto cust_ri = GetRI(CUSTOMER, Q8, ds, SERVER);
	Relation::AnnotInfo cust_ai = {true, true};
	Relation customer(cust_ri, cust_ai);
	auto filePath = GetFilePath(CUSTOMER, ds);
	customer.LoadData(filePath.c_str(), "q8_annot");

	auto orders_ri = GetRI(ORDERS, Q8, ds, CLIENT);
	Relation::AnnotInfo orders_ai = {true, true};
	Relation orders(orders_ri, orders_ai);
	filePath = GetFilePath(ORDERS, ds);
	orders.LoadData(filePath.c_str(), "q8_annot");

	auto lineitem_ri = GetRI(LINEITEM, Q8, ds, SERVER);
	Relation::AnnotInfo lineitem_ai = {false, true};
	Relation lineitem(lineitem_ri, lineitem_ai);
	filePath = GetFilePath(LINEITEM, ds);
	lineitem.LoadData(filePath.c_str(), "q8_annot");
	lineitem.Aggregate();

	auto part_ri = GetRI(PART, Q8, ds, CLIENT);
	Relation::AnnotInfo part_ai = {true, true};
	Relation part(part_ri, part_ai);
	filePath = GetFilePath(PART, ds);
	part.LoadData(filePath.c_str(), "q8_annot");

	auto supp_ri = GetRI(SUPPLIER, Q8, ds, CLIENT);
	Relation::AnnotInfo supp_ai = {true, true};
	Relation supplier(supp_ri, supp_ai);
	auto supplier_copy = supplier;
	filePath = GetFilePath(SUPPLIER, ds);
	supplier.LoadData(filePath.c_str(), "q8_annot1");
	supplier_copy.LoadData(filePath.c_str(), "q8_annot2");

	orders.SemiJoin(customer, "o_custkey", "c_custkey");
	//orders.PrintTableWithoutRevealing("orders join customer");
	lineitem.SemiJoin(part, "l_partkey", "p_partkey");
	//lineitem.PrintTableWithoutRevealing("lineitem join part");

	auto lineitem_copy = lineitem;
	lineitem.SemiJoin(supplier, "l_suppkey", "s_suppkey");
	lineitem.Aggregate("l_orderkey");
	lineitem_copy.SemiJoin(supplier_copy, "l_suppkey", "s_suppkey");
	//lineitem_copy.PrintTableWithoutRevealing("lineitem semijoin supplier");
	lineitem_copy.Aggregate("l_orderkey");
	//lineitem_copy.PrintTableWithoutRevealing("lineitem semijoin supplier and then aggregate");

	auto orders_copy = orders;
	orders.SemiJoin(lineitem, "o_orderkey", "l_orderkey");
	orders.Aggregate("o_year");
	orders_copy.SemiJoin(lineitem_copy, "o_orderkey", "l_orderkey");
	//orders_copy.PrintTableWithoutRevealing("orders semijoin lineitem_copy");
	orders_copy.Aggregate("o_year");
	//orders_copy.PrintTableWithoutRevealing("orders semijoin lineitem_copy and then aggregate");

	orders.RevealAnnotToOwner();
	orders_copy.RevealAnnotToOwner();
	if (printResult)
	{
		orders.Print();
		orders_copy.Print();
	}
}

void run_Q9(DataSize ds, bool printResult)
{
	const int numCopies = 25;

	auto orders_ri = GetRI(ORDERS, Q9, ds, CLIENT);
	Relation::AnnotInfo orders_ai = {true, true};
	Relation orders(orders_ri, orders_ai);
	auto filePath = GetFilePath(ORDERS, ds);
	orders.LoadData(filePath.c_str(), "q9_annot");

	auto lineitem_ri = GetRI(LINEITEM, Q9, ds, SERVER);
	Relation::AnnotInfo lineitem_ai = {false, true};
	Relation lineitem(lineitem_ri, lineitem_ai);
	auto lineitem_copy = lineitem;
	filePath = GetFilePath(LINEITEM, ds);
	lineitem.LoadData(filePath.c_str(), "q9_annot1");
	lineitem_copy.LoadData(filePath.c_str(), "q9_annot2");

	auto part_ri = GetRI(PART, Q9, ds, CLIENT);
	Relation::AnnotInfo part_ai = {true, true};
	Relation part(part_ri, part_ai);
	filePath = GetFilePath(PART, ds);
	part.LoadData(filePath.c_str(), "q9_annot");
	//part.PrintTableWithoutRevealing("part");

	auto partsupp_ri = GetRI(PARTSUPP, Q9, ds, CLIENT);
	Relation::AnnotInfo partsupp_ai = {false, true};
	Relation partsupp(partsupp_ri, partsupp_ai);
	Relation::AnnotInfo partsupp_copy_ai = {false, true};
	Relation partsupp_copy(partsupp_ri, partsupp_copy_ai);
	filePath = GetFilePath(PARTSUPP, ds);
	partsupp.LoadData(filePath.c_str(), "q9_annot1");
	partsupp.Aggregate();
	partsupp_copy.LoadData(filePath.c_str(), "q9_annot2");
	partsupp_copy.Aggregate();

	vector<string> ps_joinAttrs = {"ps_partkey", "ps_suppkey"};
	vector<string> line_joinAttrs = {"l_partkey", "l_suppkey"};
	lineitem.SemiJoin(partsupp, line_joinAttrs, ps_joinAttrs);
	//lineitem.PrintTableWithoutRevealing("lineitem1", 4000);
	lineitem.SemiJoin(part, "l_partkey", "p_partkey");
	//lineitem.PrintTableWithoutRevealing("lineitem join partsupp");
	lineitem_copy.SemiJoin(partsupp_copy, line_joinAttrs, ps_joinAttrs);
	//lineitem_copy.PrintTableWithoutRevealing("lineitem_copy join partsupp_copy");
	lineitem.AnnotSub(lineitem_copy);
	lineitem.SemiJoin(part, "l_partkey", "p_partkey");
	//lineitem.Sort();
	//lineitem.PrintTableWithoutRevealing("lineitem");

	Relation::RelationInfo out_ri = orders_ri;
	out_ri.numRows = 0;
	out_ri.attrNames = {"o_year", "s_nationkey"};
	out_ri.attrTypes = {Relation::INT, Relation::INT};
	Relation::AnnotInfo out_ai = {false, true};
	Relation out(out_ri, out_ai);

	auto supp_ri = GetRI(SUPPLIER, Q9, ds, CLIENT);
	Relation::AnnotInfo supp_ai = {true, true};
	string annotPrefix = "q9_annot";
	Relation supplier(supp_ri, supp_ai);
	filePath = GetFilePath(SUPPLIER, ds);

	for (int i = 0; i < numCopies; i++)
	{
		//cout << "i=" << i << endl;
		auto supplier_copy = supplier;
		string annotName = annotPrefix + to_string(i);
		supplier_copy.LoadData(filePath.c_str(), annotName.c_str());
		//supplier_copy.Print();
		lineitem_copy = lineitem;
		lineitem_copy.SemiJoin(supplier_copy, "l_suppkey", "s_suppkey");
		//lineitem_copy.PrintTableWithoutRevealing("lineitem semijoin supplier");
		lineitem_copy.Aggregate("l_orderkey");
		//lineitem_copy.PrintTableWithoutRevealing("lineitem agg");
		auto orders_copy = orders;
		orders_copy.SemiJoin(lineitem_copy, "o_orderkey", "l_orderkey");
		//orders_copy.PrintTableWithoutRevealing("orders_copy semijoin lineitem_copy");
		orders_copy.Aggregate("o_year");
		orders_copy.AddAttr("s_nationkey", Relation::INT, i);
		//orders_copy.PrintTableWithoutRevealing("aggregated orders_copy");
		out.Union(orders_copy);
	}
	out.RevealAnnotToOwner();
	if (printResult)
		out.Print();
}