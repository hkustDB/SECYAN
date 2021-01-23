#include "circuit/circuit.h"
#include "circuit/share.h"
#include <vector>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "../core/OEP.h"
#include "../core/relation.h"
#include "../core/PSI.h"
#include "../core/party.h"
#include "../core/RNG.h"

using namespace std;
using namespace SECYAN;

/* 	test_op(10);
	test_op(200);
	test_op(3000);
*/
void test_op(int size)
{
	auto role = gParty.GetRole();
	uint32_t temp, index;
	vector<uint32_t> zero(size, 0);
	vector<uint32_t> source(size);
	vector<uint32_t> dest(size);
	vector<uint32_t> out;
	for (int i = 0; i < size; i++)
		source[i] = dest[i] = i;
	for (int i = 1; i < size; i++)
	{
		index = rand() % (i + 1);
		temp = dest[index];
		dest[index] = dest[0];
		dest[0] = temp;
	}

	if (role == SERVER)
		out = PermutorPermute(dest, zero);
	else
		out = SenderPermute(source);
	auto circ = gParty.GetCircuit(S_ARITH);
	auto s_out = circ->PutOUTGate(circ->PutSharedSIMDINGate(size, out.data(), 32), ALL);
	gParty.ExecCircuit();
	uint32_t *real_out, nvals, bitlen;
	s_out->get_clear_value_vec(&real_out, &bitlen, &nvals);
	gParty.Reset();
	for (int i = 0; i < size; i++)
	{
		if (real_out[i] != dest[i])
		{
			cerr << "OP test fail when size=" << size << endl;
			exit(EXIT_FAILURE);
		}
	}
	delete[] real_out;
}

void test_oep(int M, int N)
{
	auto role = gParty.GetRole();
	vector<uint32_t> one(M, 1);
	vector<uint32_t> source(M);
	vector<uint32_t> dest(N);
	vector<uint32_t> out;
	for (int i = 0; i < M; i++)
		source[i] = i;
	for (int i = 0; i < N; i++)
		dest[i] = rand() % M;

	if (role == SERVER)
		out = PermutorExtendedPermute(dest, one);
	else
		out = SenderExtendedPermute(source, N);
	auto circ = gParty.GetCircuit(S_ARITH);
	auto s_out = circ->PutOUTGate(circ->PutSharedSIMDINGate(N, out.data(), 32), ALL);
	gParty.ExecCircuit();
	uint32_t *real_out, nvals, bitlen;
	s_out->get_clear_value_vec(&real_out, &bitlen, &nvals);
	gParty.Reset();
	for (int i = 0; i < N; i++)
	{
		if (real_out[i] != dest[i] + 1)
		{
			cerr << "OEP test fail when M=" << M << "and N=" << N << endl;
			exit(EXIT_FAILURE);
		}
	}
	delete[] real_out;
}

void test_oeps()
{
	test_op(10);
	test_op(200);
	test_op(3000);
	test_oep(240, 30);
	test_oep(240, 200);
	test_oep(240, 280);
	cout << "All OP and OEP tests passed!" << endl;
}

void test_one_relation(Relation &r, vector<string> &projectAttrNames)
{
	cout << "Original relation: " << endl;
	r.Print();

	Relation rr = r;

	r.Project(projectAttrNames);
	cout << "After projection: " << endl;
	r.Print();

	r.Sort();
	cout << "After sorting: " << endl;
	r.Print();

	r.RevealAnnotToOwner();
	cout << "After revealing: " << endl;
	r.Print();

	cout << "Copied relation:" << endl;
	rr.Print();

	rr.RevealAnnotToOwner();
	cout << "Copied relation after revealing: " << endl;
	rr.Print();
}

void test_semi_join(Relation &p, Relation &c, vector<string> pAttrs, vector<string> cAttrs)
{
	cout << "Parent relation: " << endl;
	p.Print();
	cout << "Child relation: " << endl;
	c.Print();
	p.SemiJoin(c, pAttrs, cAttrs);
	p.Print();
	p.RevealAnnotToOwner();
	p.Print();
}

void test_join(Relation &p, Relation &c, vector<string> pAttrs, vector<string> cAttrs)
{
	cout << "Parent relation: " << endl;
	p.Print();
	p.RemoveZeroAnnotatedTuples();
	cout << "Zero annotated tuples removed: " << endl;
	p.Print();
	p.RevealTuples();
	cout << "Tuples revealed: " << endl;
	p.Print();
	cout << "Child relation: " << endl;
	c.Print();
	c.RemoveZeroAnnotatedTuples();
	c.RevealTuples();
	p.Join(c, pAttrs, cAttrs);
	cout << "Parent relation join child relation: " << endl;
	p.Print();
	p.RevealAnnotToOwner();
	cout << "Annot revealed: " << endl;
	p.Print();
}

//test_relations();
void test_relations()
{
	vector<string> customer_attrs = {"c_custkey", "c_name", "c_acctbal", "c_mktsegment"};
	vector<Relation::DataType> customer_attrtypes = {Relation::INT, Relation::STRING, Relation::DECIMAL, Relation::STRING};
	Relation::RelationInfo customer_ri = {
		SERVER,
		false,
		customer_attrs,
		customer_attrtypes,
		18,
		true};
	Relation::AnnotInfo customer_ai = {false, false};
	Relation customer(customer_ri, customer_ai);
	customer.LoadData("../../../data/small/customer.tbl", "q3_annot");
	cout << "Relation Customer" << endl;
	vector<string> customer_proj_attrs = {"c_mktsegment"};
	Relation customer_copy = customer;
	//test_one_relation(customer_copy, customer_proj_attrs);
	vector<string> orders_attrs = {"o_orderkey", "o_custkey"};
	vector<Relation::DataType> orders_attrtypes = {Relation::INT, Relation::INT};
	Relation::RelationInfo orders_ri = {
		CLIENT,
		false,
		orders_attrs,
		orders_attrtypes,
		48,
		true};
	Relation::AnnotInfo orders_ai = {false, false};
	Relation orders(orders_ri, orders_ai);
	orders.LoadData("../../../data/small/orders.tbl", "q3_annot");
	vector<string> ato = {"o_custkey"};
	vector<string> atc = {"c_custkey"};
	customer_copy = customer;
	Relation orders_copy = orders;
	//test_semi_join(orders_copy, customer_copy, ato, atc);
	customer_copy = customer;
	orders_copy = orders;
	test_join(orders_copy, customer_copy, ato, atc);
	customer_copy = customer;
	orders_copy = orders;
	test_join(customer_copy, orders_copy, atc, ato);
}

void test_one_psi(int M, int N)
{
	auto role = gParty.GetRole();
	auto bc = gParty.GetCircuit(S_BOOL);
	vector<uint64_t> AliceSet(M);
	vector<uint64_t> BobSet(N);
	for (int i = 0; i < M; i++)
		AliceSet[i] = i;
	for (int i = 0; i < N; i++)
		BobSet[i] = N - i;
	PSI *psi;
	if (role == SERVER)
		psi = new PSI(AliceSet, M, N, PSI::Alice);
	else
		psi = new PSI(BobSet, M, N, PSI::Bob);
	auto out = psi->Intersect();
	auto s_in = bc->PutSharedSIMDINGate(out.size(), out.data(), 1);
	auto s_out = bc->PutOUTGate(s_in, ALL);
	uint32_t *a;
	uint32_t b, c;
	gParty.ExecCircuit();
	s_out->get_clear_value_vec(&a, &b, &c);
	gParty.Reset();
	int num = 0;
	for (int i = 0; i < (int)out.size(); i++)
		num += a[i];
	if (num != min(M - 1, N))
	{
		cout << "test psi fail when M=" << M << " and N=" << N << endl;
		exit(1);
	}
	delete[] a;
	delete psi;
}

void test_psi_payload(int M, int N)
{
	auto role = gParty.GetRole();
	auto ac = gParty.GetCircuit(S_ARITH);
	//auto bc = gParty.GetCircuit(S_BOOL);
	vector<uint64_t> AliceSet(M);
	vector<uint64_t> BobSet(N);
	vector<uint32_t> BobPayload1(N);
	vector<uint32_t> BobPayload2(N);
	vector<uint32_t> BobPayload(N);
	for (int i = 0; i < M; i++)
		AliceSet[i] = i;
	for (int i = 0; i < N; i++)
	{
		BobSet[i] = i + 1;
		BobPayload[i] = rand() % 4209;
		BobPayload1[i] = rand();
		BobPayload2[i] = BobPayload[i] - BobPayload1[i];
	}
	PSI *psi;
	if (role == SERVER)
		psi = new PSI(AliceSet, M, N, PSI::Alice);
	else
		psi = new PSI(BobSet, M, N, PSI::Bob);
	auto indicator = psi->Intersect();
	auto out_payload = psi->IntersectWithPayload(BobPayload);
	auto bucketSize = indicator.size();
	auto s_payload_in = ac->PutSharedSIMDINGate(bucketSize, out_payload.data(), 32);
	auto s_out_payload = ac->PutOUTGate(s_payload_in, ALL);
	gParty.ExecCircuit();
	uint64_t *recvpayload;
	uint32_t b, c;
	s_out_payload->get_clear_value_vec(&recvpayload, &b, &c);
	gParty.Reset();

	if (role == SERVER)
	{
		auto permutedIndices = psi->CuckooToAliceArray();
		for (int i = 1; i < min(M, N); i++)
		{
			int index = permutedIndices[i];
			if (recvpayload[index] != BobPayload[i - 1])
			{
				cout << "test psi with payload fail when M=" << M << " and N=" << N << endl;
				exit(1);
			}
		}
	}

	vector<uint32_t> newpayload;
	if (role == SERVER)
		newpayload = psi->CombineSharedPayload(BobPayload1, indicator);
	else
		newpayload = psi->CombineSharedPayload(BobPayload2, indicator);

	auto s_newpayload = ac->PutSharedSIMDINGate(bucketSize, newpayload.data(), 32);
	auto s_newpayloadout = ac->PutOUTGate(s_newpayload, ALL);
	gParty.ExecCircuit();
	uint32_t *newpayloadout;
	s_newpayloadout->get_clear_value_vec(&newpayloadout, &b, &c);
	gParty.Reset();

	if (role == SERVER)
	{
		auto permutedIndices = psi->CuckooToAliceArray();
		for (int i = 1; i < min(M, N); i++)
		{
			int index = permutedIndices[i];
			if (newpayloadout[index] != BobPayload[i - 1])
			{
				cout << "test psi with shared payload fail when M=" << M << " and N=" << N << endl;
				exit(1);
			}
		}
	}
	delete[] recvpayload;
	delete[] newpayloadout;
	delete psi;
}

void test_psis()
{
	test_one_psi(40, 40);
	test_one_psi(40, 4000);
	test_one_psi(4000, 400);
	test_psi_payload(40, 40);
	test_psi_payload(40, 400);
	test_psi_payload(400, 40);
	cout << "All PSI tests passed!" << endl;
}

void test_aby_func()
{
	uint32_t a[] = {1, 2, 3, 4};
	uint32_t b[] = {1, 0, 0, 1};
	auto ac = gParty.GetCircuit(S_ARITH);
	auto bc = gParty.GetCircuit(S_BOOL);
	auto yc = gParty.GetCircuit(S_YAO);
	auto in = ac->PutSharedSIMDINGate(4, a, 32);
	auto in2 = bc->PutA2BGate(in, yc);
	auto zero = bc->PutSIMDCONSGate(4, (uint64_t)0, 32);
	auto sb = bc->PutSIMDINGate(4, b, 1, SERVER);
	auto mux = bc->PutMUXGate(in2, zero, sb);
	bc->PutPrintValueGate(in2, "in");
	bc->PutPrintValueGate(mux, "mux");
	gParty.ExecCircuit();
	gParty.Reset();
}

/*
test_ot(300, 10);
test_ot(600, 10);
test_ot(300, 100);
libOT: 78.125 ms
MUX: 953.125 ms
libOT: 187.5 ms
MUX: 2031.25 ms
libOT: 156.25 ms
MUX: 1875 ms
*/
// m times, each time run n 1-out-of-2 OT
void test_ot(int m, int n)
{
	clock_t start, end;
	float duration;
	vector<uint64_t> msg[2];
	vector<uint32_t> selectBits(n);
	vector<uint64_t> out(n);
	msg[0].resize(n);
	msg[1].resize(n);

	start = clock();
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			msg[0][j] = rand() & 0xff;
			msg[1][j] = rand() & 0xff;
			selectBits[j] = rand() & 1;
		}
		if (gParty.GetRole() == SERVER)
			gParty.OTSend(msg[0], msg[1]);
		else
		{
			auto out = gParty.OTRecv(selectBits);
			for (int j = 0; j < n; j++)
			{
				if (msg[selectBits[j]][j] != out[j])
				{
					cerr << "OT test fail!" << endl;
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	end = clock();
	duration = 1000.0 * (end - start) / CLOCKS_PER_SEC;
	cout << "libOT: " << duration << " ms" << endl;

	auto circ = gParty.GetCircuit(S_BOOL);
	start = clock();
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			msg[0][j] = rand() & 0xff;
			msg[1][j] = rand() & 0xff;
			selectBits[j] = rand() & 1;
		}
		share *in0, *in1, *b;
		if (gParty.GetRole() == SERVER)
		{
			in0 = circ->PutSIMDINGate(n, msg[0].data(), 64, SERVER);
			in1 = circ->PutSIMDINGate(n, msg[1].data(), 64, SERVER);
			b = circ->PutDummySIMDINGate(n, 1);
		}
		else
		{
			in0 = circ->PutDummySIMDINGate(n, 64);
			in1 = circ->PutDummySIMDINGate(n, 64);
			b = circ->PutSIMDINGate(n, selectBits.data(), 1, CLIENT);
		}
		auto mux = circ->PutMUXGate(in1, in0, b);
		auto s_out = circ->PutOUTGate(mux, CLIENT);
		uint32_t bitlen, nvals;
		uint64_t *p_out;
		gParty.ExecCircuit();
		if (gParty.GetRole() == CLIENT)
		{
			s_out->get_clear_value_vec(&p_out, &bitlen, &nvals);
			for (int j = 0; j < n; j++)
			{
				if (msg[selectBits[j]][j] != p_out[j])
				{
					cerr << "OT test fail!" << endl;
					exit(EXIT_FAILURE);
				}
			}
		}
		gParty.Reset();
	}
	end = clock();
	duration = 1000.0 * (end - start) / CLOCKS_PER_SEC;
	cout << "MUX: " << duration << " ms" << endl;
}

/*
test_tranfer(1000,10000);
test_tranfer(5000,10000);
test_tranfer(1000,50000);
Simple connection: 484.375 ms
ABY: 4406.25 ms
Simple connection: 1953.12 ms
ABY: 22375 ms
Simple connection: 2062.5 ms
ABY: 20265.6 ms
*/

void test_tranfer(int m, int n)
{
	clock_t start, end;
	float duration;
	vector<uint64_t> msg(n);
	vector<uint64_t> out(n);

	start = clock();
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
			msg[j] = rand() & 0xff;
		if (gParty.GetRole() == SERVER)
			gParty.Send(msg);
		else
		{
			gParty.Recv(out);
			for (int j = 0; j < n; j++)
			{
				if (msg[j] != out[j])
				{
					cerr << "Transfer test fail!" << endl;
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	end = clock();
	duration = 1000.0 * (end - start) / CLOCKS_PER_SEC;
	cout << "Simple connection: " << duration << " ms" << endl;

	auto circ = gParty.GetCircuit(S_BOOL);
	start = clock();
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
			msg[j] = rand() & 0xff;
		auto s_in = circ->PutSIMDINGate(n, msg.data(), 64, SERVER);
		auto s_out = circ->PutOUTGate(s_in, CLIENT);
		uint32_t bitlen, nvals;
		uint64_t *p_out;
		gParty.ExecCircuit();
		if (gParty.GetRole() == CLIENT)
		{
			s_out->get_clear_value_vec(&p_out, &bitlen, &nvals);
			for (int j = 0; j < n; j++)
			{
				if (msg[j] != p_out[j])
				{
					cerr << "OT test fail!" << endl;
					exit(EXIT_FAILURE);
				}
			}
		}
		gParty.Reset();
	}
	end = clock();
	duration = 1000.0 * (end - start) / CLOCKS_PER_SEC;
	cout << "ABY: " << duration << " ms" << endl;
}

int main(int argc, char **)
{
	srand(14131);
	string address = "127.0.0.1";
	uint16_t port = 7766;
	e_role role = SERVER;
	//role = CLIENT;

	if (argc > 1)
		role = (e_role)(1 - role);

	gParty.Init(address, port, role);
	test_oeps();
	test_psis();
	test_relations();
	//test_aby_func();

	return 0;
}