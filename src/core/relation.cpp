#include "relation.h"
#include "OEP.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "PSI.h"
#include "MurmurHash3.h"
#include "circuit/share.h"
#include "circuit/booleancircuits.h"
#include <numeric>
#include "RNG.h"
#include <unordered_set>

namespace SECYAN
{
	// Take a subsequence of an array (override!)
	template <typename T>
	void SubSequence(std::vector<T> &a, std::vector<uint32_t> &indices)
	{
		std::vector<T> b = a;
		a.resize(indices.size());
		for (size_t i = 0; i < indices.size(); i++)
			a[i] = b[indices[i]];
	}

	void Relation::LoadData(const char *filePath, std::string annotAttrName)
	{
		if (IsDummy())
			return;
		m_Tuples.resize(m_RI.numRows);
		std::unordered_map<std::string, int> inverseAttrMap;
		for (uint32_t i = 0; i < m_RI.attrNames.size(); i++)
			inverseAttrMap[m_RI.attrNames[i]] = i;

		// open file filePath
		std::ifstream fin(filePath, std::ios::in);
		if(!fin.is_open())
		{
			std::cerr << "Open file error!" << std::endl;
			std::exit(1);
		}
		std::string element;
		int numColumns = -1;
		fin >> numColumns;
		fin >> std::ws; // remove the newline character

		std::vector<int> indexMap(numColumns);
		for (uint32_t i = 0; i < numColumns; i++)
		{
			getline(fin, element, '|');
			auto it = inverseAttrMap.find(element);
			if (it != inverseAttrMap.end())
				indexMap[i] = it->second;
			else if (element == annotAttrName)
				indexMap[i] = -2;
			else
				indexMap[i] = -1;
		}
		fin >> std::ws;

		for (uint32_t i = 0; i < m_RI.numRows; i++)
		{
			uint32_t year, month, day, pos;
			float f_value;
			const int arrlen = sizeof(uint64_t) / sizeof(char);
			char padded_str[arrlen];
			m_Tuples[i].resize(m_RI.attrNames.size());
			for (uint32_t j = 0; j < numColumns; j++)
			{
				getline(fin, element, '|');
				int index = indexMap[j];
				if (index >= 0)
				{
					switch (m_RI.attrTypes[index])
					{
					case DataType::INT:
						m_Tuples[i][index] = (uint64_t)stoi(element);
						break;
					case DataType::DECIMAL:
						f_value = stof(element);
						m_Tuples[i][index] = (uint64_t)(int)(100 * f_value);
						break;
					case DataType::DATE:
						if(sscanf(element.c_str(), "%4d-%2d-%2d", &year, &month, &day) != 3)
						{
							std::cerr << "Read date error!" << std::endl;
							std::exit(1);
						}
						m_Tuples[i][index] = year * 10000 + month * 100 + day;
						break;
					case DataType::STRING:
						std::memset(padded_str, 0, arrlen);
						pos = std::max(0, (int)element.length() - arrlen);
						element.copy(padded_str, arrlen, pos);
						m_Tuples[i][index] = *(uint64_t *)padded_str;
						break;
					}
				}
				else if (index == -2)
				{
					m_Annot[i] = stoul(element);
					if (m_AI.isBoolean)
						assert(m_Annot[i] <= 1);
				}
			}
			fin >> std::ws;
		}
		fin.close();
	}

	void Relation::RevealAnnotToOwner()
	{
		if (m_AI.knownByOwner || m_RI.numRows == 0)
			return;

		if (m_RI.owner == gParty.GetRole())
		{
			std::vector<uint32_t> annot;
			gParty.Recv(annot);
			for (uint32_t i = 0; i < m_RI.numRows; i++)
				m_Annot[i] = m_AI.isBoolean ? m_Annot[i] ^ annot[i] : m_Annot[i] + annot[i];
		}
		else
		{
			gParty.Send(m_Annot);
			std::fill(m_Annot.begin(), m_Annot.end(), 0);
		}

		m_AI.knownByOwner = true;
	}

	void Relation::Print(size_t limit_size, bool showZeroAnnotedTuple)
	{
		bool dummy = IsDummy();
		if (m_RI.owner != gParty.GetRole() && m_AI.knownByOwner)
		{
			std::cout << "Dummy Relation!" << std::endl;
			std::cout << std::endl;
			return;
		}

		std::cout << "row_num" << '\t';
		if (!dummy)
			for (auto attrName : m_RI.attrNames)
				std::cout << attrName << '\t';
		std::cout << "annotation" << std::endl;
		uint64_t i_value, year, month, day;
		uint32_t out, printed = 0;
		float f_value;
		const int arrlen = sizeof(uint64_t) / sizeof(char);
		char padded_str[arrlen + 1] = "";
		for (uint32_t i = 0; i < m_RI.numRows && printed < limit_size; i++)
		{
			if (m_AI.knownByOwner && (m_Annot[i] == 0 && !showZeroAnnotedTuple || m_Tuples[i].IsDummy()))
				continue;
			printed++;
			std::cout << i + 1 << '\t';
			if (!dummy)
			{
				for (uint32_t j = 0; j < m_RI.attrNames.size(); ++j)
				{
					switch (m_RI.attrTypes[j])
					{
					case DataType::INT:
						std::cout << (int)m_Tuples[i][j];
						break;
					case DataType::STRING:
						*(uint64_t *)padded_str = m_Tuples[i][j];
						std::cout << padded_str; // for simplicity
						break;
					case DataType::DATE:
						i_value = m_Tuples[i][j];
						day = i_value % 100;
						i_value /= 100;
						month = i_value % 100;
						i_value /= 100;
						year = i_value;
						std::cout << year << '-' << std::setfill('0') << std::setw(2) << month << '-' << std::setfill('0') << std::setw(2) << day;
						break;
					case DataType::DECIMAL:
						f_value = (float)(int)m_Tuples[i][j] / 100;
						std::cout << std::setprecision(2) << std::fixed << f_value;
						break;
					}
					std::cout << '\t';
				}
			}
			std::cout << (int)m_Annot[i] << std::endl;
		}
		if (printed == 0)
			std::cout << "Empty Relation!" << std::endl;
		std::cout << std::endl;
	}

	void Relation::PrintTableWithoutRevealing(const char *msg, int limit_size)
	{
		auto annot = m_Annot;
		auto ai = m_AI;
		if (msg)
			std::cout << msg << std::endl;
		RevealAnnotToOwner();
		Print(limit_size);
		m_Annot = annot;
		m_AI = ai;
	}

	void Relation::PermuteAnnotByOwner(std::vector<uint32_t> &permutedIndices)
	{
		if (IsDummy())
		{
			if (!m_AI.knownByOwner)
				m_Annot = SenderPermute(m_Annot);
			return;
		}
		if (m_AI.knownByOwner)
			SubSequence(m_Annot, permutedIndices);
		else
			m_Annot = PermutorPermute(permutedIndices, m_Annot);
	}

	void Relation::Sort()
	{
		if (m_RI.sorted)
			return;
		m_RI.sorted = true;
		if (m_RI.numRows == 0)
			return;
		std::vector<uint32_t> indices(m_RI.numRows);
		if (!IsDummy())
		{
			std::iota(indices.begin(), indices.end(), 0);
			std::sort(indices.begin(), indices.end(), [&](uint32_t i, uint32_t j) { return m_Tuples[i] < m_Tuples[j]; });
			SubSequence(m_Tuples, indices);
		}
		if (m_RI.isPublic)
			SubSequence(m_Annot, indices);
		else
			PermuteAnnotByOwner(indices);
	}

	void Relation::Project(const char *projectAttrName)
	{
		std::vector<std::string> projectAttrNames(1);
		projectAttrNames[0] = projectAttrName;
		Project(projectAttrNames);
	}

	void Relation::Project(std::vector<std::string> &projectAttrNames)
	{
		auto numProjectAttrs = projectAttrNames.size();
		m_RI.sorted = false;
		std::unordered_map<std::string, int> invAttrMap;
		std::vector<uint32_t> indexMap(numProjectAttrs);
		for (uint32_t i = 0; i < m_RI.attrNames.size(); i++)
			invAttrMap[m_RI.attrNames[i]] = i;
		for (uint32_t i = 0; i < numProjectAttrs; i++)
		{
			auto it = invAttrMap.find(projectAttrNames[i]);
			if(it == invAttrMap.end())
			{
				std::cerr << "Projection attribute error!" << std::endl;
				std::exit(1);
			}
			indexMap[i] = it->second;
		}
		if (!IsDummy())
		{
			for (uint32_t i = 0; i < m_RI.numRows; i++)
			{
				Tuple tuple = m_Tuples[i];
				m_Tuples[i].resize(numProjectAttrs);
				if (!tuple.IsDummy())
					for (uint32_t j = 0; j < numProjectAttrs; j++)
						m_Tuples[i][j] = tuple[indexMap[j]];
			}
		}
		SubSequence(m_RI.attrTypes, indexMap);
		m_RI.attrNames = projectAttrNames;
	}

	// The pi-1 projector (used after projection)
	void Relation::AnnotOrAgg()
	{
		Sort();

		if (!m_AI.knownByOwner)
		{
			OblivAnnotOrAgg();
			return;
		}
		m_AI.isBoolean = true;
		m_RI.sorted = false;
		if (IsDummy())
			return;
		int numRows = m_RI.numRows;
		for (uint32_t i = 0; i < numRows; i++)
			if (m_Annot[i] != 0)
				m_Annot[i] = 1;
		for (uint32_t i = 0; i < numRows - 1; i++)
		{
			if (m_Tuples[i] == m_Tuples[i + 1])
			{
				m_Annot[i + 1] |= m_Annot[i];
				m_Annot[i] = 0;
				m_Tuples[i].ToDummy();
			}
		}
	}

	void Relation::OblivAnnotOrAgg()
	{
		int numRows = m_RI.numRows;
		auto yc = (BooleanCircuit *)gParty.GetCircuit(S_YAO);
		auto bc = (BooleanCircuit *)gParty.GetCircuit(S_BOOL);
		share **bAnnot = new share *[numRows];
		for (uint32_t i = 0; i < numRows; i++)
		{
			if (m_AI.isBoolean)
				bAnnot[i] = yc->PutB2YGate(bc->PutSharedINGate(m_Annot[i], 1));
			else
			{
				auto s0 = bc->PutINGate(m_Annot[i], 32, SERVER);
				auto s1 = bc->PutINGate(-m_Annot[i], 32, CLIENT);
				bAnnot[i] = yc->PutB2YGate(bc->PutINVGate(bc->PutEQGate(s0, s1)));
			}
		}
		m_AI.isBoolean = true;
		for (uint32_t i = 0; i < numRows - 1; i++)
		{
			bool sameTuple = (m_RI.owner == gParty.GetRole()) && (m_Tuples[i] == m_Tuples[i + 1]);
			auto s_rep = yc->PutINGate((uint8_t)sameTuple, 1, m_RI.owner);
			auto s_or = yc->PutORGate(bAnnot[i], bAnnot[i + 1]);
			bAnnot[i] = yc->PutANDGate(yc->PutINVGate(s_rep), bAnnot[i]);
			bAnnot[i + 1] = yc->PutMUXGate(s_or, bAnnot[i + 1], s_rep);
		}
		for (uint32_t i = 0; i < numRows; i++)
		{
			auto y2b = bc->PutY2BGate(bAnnot[i]);
			bAnnot[i] = bc->PutSharedOUTGate(y2b);
		}
		gParty.ExecCircuit();
		for (uint32_t i = 0; i < numRows; i++)
			m_Annot[i] = bAnnot[i]->get_clear_value<uint8_t>();
		gParty.Reset();
		delete[] bAnnot;
		m_RI.sorted = false;
		if (IsDummy())
			return;
		for (uint32_t i = 0; i < numRows - 1; i++)
			if (m_Tuples[i] == m_Tuples[i + 1])
				m_Tuples[i].ToDummy();
	}

	void Relation::OwnerAnnotAddAgg()
	{
		auto size = m_RI.numRows;
		std::vector<uint32_t> aggBits(size - 1);
		for (uint32_t i = 0; i < size - 1; i++)
		{
			aggBits[i] = m_Tuples[i] == m_Tuples[i + 1];
			if (aggBits[i])
				m_Tuples[i].ToDummy();
		}
		if (m_RI.isPublic || m_AI.knownByOwner)
		{
			for (uint32_t i = 0; i < size - 1; i++)
			{
				if (aggBits[i])
				{
					m_Annot[i + 1] += m_Annot[i];
					m_Annot[i] = 0;
				}
			}
		}
		else
			m_Annot = PermutorAggregate(aggBits, m_Annot);
	}

	void Relation::Aggregate()
	{
		if (m_RI.numRows == 0)
			return;
		Sort();
		assert(!m_AI.isBoolean);
		m_RI.sorted = false;
		if (IsDummy())
		{
			if (!m_AI.knownByOwner)
				m_Annot = SenderAggregate(m_Annot);
			return;
		}
		OwnerAnnotAddAgg();
	}

	void Relation::Aggregate(const char *aggAttrName)
	{
		Project(aggAttrName);
		Aggregate();
	}

	void Relation::Aggregate(std::vector<std::string> &aggAttrNames)
	{
		Project(aggAttrNames);
		Aggregate();
	}

	uint64_t Relation::HashTuple(int i)
	{
		if (i >= m_RI.numRows || i < 0 || m_Tuples[i].IsDummy())
			return (uint64_t)i << 32 | gRNG.NextUInt32();
		int numColumns = this->m_RI.attrNames.size();
		if (numColumns == 1)
			return m_Tuples[i][0];
		uint64_t out[2];
		MurmurHash3_x64_128(m_Tuples[i].data(), numColumns * 8, 0, out);
		return out[0];
	}

	void Relation::SemiJoin(Relation &child, const char *parentAttrName, const char *childAttrName)
	{
		std::vector<std::string> parentAttrNames(1);
		std::vector<std::string> childAttrNames(1);
		parentAttrNames[0] = parentAttrName;
		childAttrNames[0] = childAttrName;
		SemiJoin(child, parentAttrNames, childAttrNames);
	}

	void Relation::SemiJoin(Relation &child, std::vector<std::string> &parentAttrNames, std::vector<std::string> &childAttrNames)
	{
		assert(parentAttrNames.size() == childAttrNames.size());
		if (m_RI.owner != child.m_RI.owner)
			OblivSemiJoin(child, parentAttrNames, childAttrNames);
		else
		{
			std::cerr << "Semi join by the same owner not implemented yet!" << std::endl;
			std::exit(1);
		}
	}

	void Relation::AnnotMul(uint32_t *indicator, uint32_t *childAnnotPermuted, bool isChildAnnotBool)
	{
		auto size = m_RI.numRows;
		auto ac = gParty.GetCircuit(S_ARITH);
		auto bc = gParty.GetCircuit(S_BOOL);
		auto s_indicator = bc->PutSharedSIMDINGate(size, indicator, 1);
		auto s_payload2 = isChildAnnotBool ? bc->PutSharedSIMDINGate(size, childAnnotPermuted, 1) : ac->PutSharedSIMDINGate(size, childAnnotPermuted, 32);
		share *s_payload1;
		if (m_AI.isBoolean)
		{
			if (m_AI.knownByOwner)
				s_payload1 = bc->PutSIMDINGate(m_RI.numRows, m_Annot.data(), 1, m_RI.owner);
			else
				s_payload1 = bc->PutSharedSIMDINGate(m_RI.numRows, m_Annot.data(), 1);
		}
		else
		{
			if (m_AI.knownByOwner)
				s_payload1 = ac->PutSIMDINGate(m_RI.numRows, m_Annot.data(), 32, m_RI.owner);
			else
				s_payload1 = ac->PutSharedSIMDINGate(m_RI.numRows, m_Annot.data(), 32);
		}

		//if(size < 1000)
		// {
		// 	bc->PutPrintValueGate(s_indicator,"indicator");
		// 	(m_AI.isBoolean ? bc : ac)->PutPrintValueGate(s_payload1,"payload1");
		// 	(isChildAnnotBool ? bc : ac)->PutPrintValueGate(s_payload2,"payload2");
		// }

		if (!m_AI.isBoolean) // so that s_payload1 is boolean if s_payload2 is boolean
			std::swap(s_payload1, s_payload2);
		share *s_out;
		if (m_AI.isBoolean && isChildAnnotBool)
		{
			auto s_and = bc->PutANDGate(s_indicator, bc->PutANDGate(s_payload1, s_payload2));
			s_out = bc->PutSharedOUTGate(s_and);
		}
		else if (!m_AI.isBoolean && !isChildAnnotBool)
		{
			auto sa_indicator = ac->PutB2AGate(s_indicator);
			auto s_mul = ac->PutMULGate(sa_indicator, ac->PutMULGate(s_payload1, s_payload2));
			// ac->PutPrintValueGate(s_mul, "MUL");
			s_out = ac->PutSharedOUTGate(s_mul);
		}
		else // (m_AI.isBoolean && !isChildAnnotBool)
		{
			auto s_and = bc->PutANDGate(s_indicator, s_payload1);
			auto sa_and = ac->PutB2AGate(s_and);
			//ac->PutPrintValueGate(sa_and, "sa_and");
			auto s_mul = ac->PutMULGate(sa_and, s_payload2);
			//ac->PutPrintValueGate(s_mul, "s_mul");
			s_out = ac->PutSharedOUTGate(s_mul);
		}
		gParty.ExecCircuit();
		uint32_t *newAnnot;
		uint32_t bitlen, nvals;
		s_out->get_clear_value_vec(&newAnnot, &bitlen, &nvals);
		assert(nvals == size);
		gParty.Reset();

		// Note: In ABY, B2AGate and MULGate not always return corret values
		// According to my test, they return random values with a small probability (between 1% and 10%)
		// This is reproduced by re-running a test program several times
		// To ensure the correctness, here we simply reveal the indicator and payloads
		// Although this violates the secure model, the running time and communication cost are still correctly simulated (with a small overhead)
		if (!m_AI.isBoolean || !isChildAnnotBool)
		{
			// indicator, payload1, payload2, mask
			std::vector<uint32_t> all_data;
			if (gParty.GetRole() == SERVER)
			{
				gParty.Recv(all_data);
				assert(all_data.size() == size * 4);
				for (uint32_t i = 0; i < size; i++)
				{
					all_data[i] = (all_data[i] ^ indicator[i]) & 1;
					all_data[i + size] += m_Annot[i];
					if (m_AI.isBoolean)
						all_data[i + size] &= 1;
					all_data[i + 2 * size] += childAnnotPermuted[i];
					if (isChildAnnotBool)
						all_data[i + 2 * size] &= 1;
					m_Annot[i] = all_data[i] * all_data[i + size] * all_data[i + 2 * size] - all_data[i + 3 * size];
				}
			}
			else
			{
				all_data.insert(all_data.end(), indicator, indicator + size);
				all_data.insert(all_data.end(), m_Annot.begin(), m_Annot.end());
				all_data.insert(all_data.end(), childAnnotPermuted, childAnnotPermuted + size);
				for (uint32_t i = 0; i < size; i++)
					m_Annot[i] = gRNG.NextUInt32();
				all_data.insert(all_data.end(), m_Annot.begin(), m_Annot.end());
				gParty.Send(all_data);
			}
		}
		else
			m_Annot.assign(newAnnot, newAnnot + size);

		m_AI.knownByOwner = false;
		if (!isChildAnnotBool)
			m_AI.isBoolean = false;
		delete[] newAnnot;
	}

	void Relation::AliceSemiJoin(Relation &BobRelation)
	{
		assert(m_RI.owner == gParty.GetRole());
		auto aliceRowNum = m_RI.numRows;
		auto bobRowNum = BobRelation.m_RI.numRows;
		std::unordered_map<uint64_t, std::vector<uint32_t>> rowIndexMap;
		for (uint32_t i = 0; i < aliceRowNum; i++)
			rowIndexMap[HashTuple(i)].push_back(i);
		std::vector<uint64_t> myHashValues(aliceRowNum);
		std::vector<uint32_t> firstPermutedIndices(aliceRowNum);
		uint32_t i = 0;
		for (auto mapPair : rowIndexMap)
		{
			for (auto j : mapPair.second)
				firstPermutedIndices[j] = i;
			myHashValues[i++] = mapPair.first;
		}
		assert(i <= aliceRowNum);
		for (; i < aliceRowNum; i++)
			myHashValues[i] = HashTuple(-i);
		PSI psi(myHashValues, aliceRowNum, bobRowNum, PSI::Alice);

		auto indicator = psi.Intersect();
		std::vector<uint32_t> bobpayload_mask;
		if (BobRelation.m_AI.knownByOwner)
			bobpayload_mask = psi.IntersectWithPayload();
		else
			bobpayload_mask = psi.CombineSharedPayload(BobRelation.m_Annot, indicator);

		auto secondPermutedIndices = psi.CuckooToAliceArray();
		std::vector<uint32_t> indices(aliceRowNum);
		for (uint32_t i = 0; i < aliceRowNum; i++)
			indices[i] = secondPermutedIndices[firstPermutedIndices[i]];

		// auto ac = gParty.GetCircuit(S_ARITH);
		// auto in = ac->PutSharedSIMDINGate(bobpayload_mask.size(), bobpayload_mask.data(), 32);
		// ac->PutPrintValueGate(in, "before OEP");

		bobpayload_mask = PermutorExtendedPermute(indices, bobpayload_mask);
		indicator = PermutorExtendedPermute(indices, indicator);

		// in = ac->PutSharedSIMDINGate(aliceRowNum, bobpayload_mask.data(), 32);
		// ac->PutPrintValueGate(in, "after OEP");
		// gParty.ExecCircuit();
		// gParty.Reset();

		AnnotMul(indicator.data(), bobpayload_mask.data(), BobRelation.m_AI.isBoolean);
	}

	void Relation::BobSemiJoin(Relation &BobRelation)
	{
		assert(BobRelation.m_RI.owner == gParty.GetRole());
		auto aliceRowNum = m_RI.numRows;
		auto bobRowNum = BobRelation.m_RI.numRows;
		std::vector<uint64_t> myHashValues(bobRowNum);
		for (uint32_t i = 0; i < bobRowNum; i++)
			myHashValues[i] = BobRelation.HashTuple(i);

		PSI psi(myHashValues, aliceRowNum, bobRowNum, PSI::Bob);
		auto indicator = psi.Intersect();
		std::vector<uint32_t> bobpayload_mask;
		if (BobRelation.m_AI.knownByOwner)
			bobpayload_mask = psi.IntersectWithPayload(BobRelation.m_Annot);
		else
			bobpayload_mask = psi.CombineSharedPayload(BobRelation.m_Annot, indicator);

		//auto ac = gParty.GetCircuit(S_ARITH);
		//auto in = ac->PutSharedSIMDINGate(bobpayload_mask.size(), bobpayload_mask.data(), 32);
		//ac->PutPrintValueGate(in, "before OEP");

		bobpayload_mask = SenderExtendedPermute(bobpayload_mask, aliceRowNum);
		indicator = SenderExtendedPermute(indicator, aliceRowNum);
		//in = ac->PutSharedSIMDINGate(aliceRowNum, bobpayload_mask.data(), 32);
		//ac->PutPrintValueGate(in, "after OEP");
		//gParty.ExecCircuit();
		//gParty.Reset();

		AnnotMul(indicator.data(), bobpayload_mask.data(), BobRelation.m_AI.isBoolean);
	}

	void Relation::OblivSemiJoin(Relation &child, std::vector<std::string> &parentAttrNames, std::vector<std::string> &childAttrNames)
	{
		Relation parentRelation = *this;
		Relation childRelation = child;
		parentRelation.Project(parentAttrNames);
		childRelation.Project(childAttrNames);
		if (gParty.GetRole() == m_RI.owner)
			parentRelation.AliceSemiJoin(childRelation);
		else
			parentRelation.BobSemiJoin(childRelation);
		m_AI = parentRelation.m_AI;
		m_Annot = parentRelation.m_Annot;
	}

	void Relation::RemoveZeroAnnotatedTuples()
	{
		uint32_t *out, bitlen, nvals;
		auto numRows = m_RI.numRows;
		if (m_RI.isPublic)
		{
			out = new uint32_t[numRows];
			for (uint32_t i = 0; i < numRows; i++)
				out[i] = m_Annot[i] == 0;
		}
		else if (m_AI.knownByOwner)
		{
			out = new uint32_t[numRows];
			if (IsDummy())
				gParty.Recv(out, numRows);
			else
			{
				for (uint32_t i = 0; i < numRows; i++)
					out[i] = m_Annot[i] == 0;
				gParty.Send(out, numRows);
			}
		}
		else
		{
			share *zeroAnnot;
			auto bc = (BooleanCircuit *)gParty.GetCircuit(S_BOOL);
			if (m_AI.isBoolean)
				zeroAnnot = bc->PutINVGate(bc->PutSharedSIMDINGate(numRows, m_Annot.data(), 1));
			else
			{
				auto annot = m_Annot;
				if (gParty.GetRole() == CLIENT)
					for (uint32_t i = 0; i < numRows; i++)
						annot[i] = -m_Annot[i];
				zeroAnnot = bc->PutEQGate(bc->PutSIMDINGate(numRows, m_Annot.data(), 32, SERVER), bc->PutSIMDINGate(numRows, annot.data(), 32, CLIENT));
			}
			auto s_out = bc->PutOUTGate(zeroAnnot, ALL);
			gParty.ExecCircuit();
			s_out->get_clear_value_vec(&out, &bitlen, &nvals);
			gParty.Reset();
		}
		std::vector<uint32_t> nonZeroIndices;
		for (uint32_t i = 0; i < numRows; i++)
			if (!out[i])
				nonZeroIndices.push_back(i);
		SubSequence(m_Annot, nonZeroIndices);
		if (!IsDummy())
			SubSequence(m_Tuples, nonZeroIndices);
		m_RI.numRows = nonZeroIndices.size();
		delete[] out;
	}

	// Every tuple must not be dummy!!!
	std::vector<uint64_t> Relation::PackTuples()
	{
		assert(!IsDummy() && "Trying to pack tuples in dummy relation!");
		auto numColumns = m_RI.attrNames.size();
		auto numRows = m_RI.numRows;
		std::vector<uint64_t> packedTuples(numColumns * numRows);
		for (uint32_t i = 0; i < numRows; i++)
			for (uint32_t j = 0; j < numColumns; j++)
				packedTuples[i * numColumns + j] = m_Tuples[i][j];
		return packedTuples;
	}

	std::vector<Tuple> UnpackTuples(std::vector<uint64_t> &packedTuples, uint32_t numRows)
	{
		auto numColumns = packedTuples.size() / numRows;
		std::vector<Tuple> tuples(numRows);
		for (uint32_t i = 0; i < numRows; i++)
		{
			tuples[i].resize(numColumns);
			for (uint32_t j = 0; j < numColumns; j++)
				tuples[i][j] = packedTuples[i * numColumns + j];
		}
		return tuples;
	}

	void Relation::RevealTuples()
	{
		if (m_RI.isPublic || m_RI.numRows == 0)
		{
			m_RI.isPublic = true;
			return;
		}
		std::vector<uint64_t> packedTuples;
		if (IsDummy())
		{
			gParty.Recv(packedTuples);
			m_Tuples = UnpackTuples(packedTuples, m_RI.numRows);
		}
		else
		{
			packedTuples = PackTuples();
			gParty.Send(packedTuples);
		}
		m_RI.isPublic = true;
	}

	void Relation::Join(Relation &child, const char *parentAttrName, const char *childAttrName)
	{
		std::vector<std::string> parentAttrNames(1);
		std::vector<std::string> childAttrNames(1);
		parentAttrNames[0] = parentAttrName;
		childAttrNames[0] = childAttrName;
		Join(child, parentAttrNames, childAttrNames);
	}

	// Every tuple must not be zero annotated in join !!!
	void Relation::Join(Relation &child, std::vector<std::string> &parentAttrNames, std::vector<std::string> &childAttrNames)
	{
		assert(parentAttrNames.size() == childAttrNames.size());
		assert(m_RI.isPublic && child.m_RI.isPublic); // Only support public join yet
		auto parentCopy = *this;
		parentCopy.Project(parentAttrNames);
		auto childCopy = child;
		childCopy.Project(childAttrNames);
		//auto aliceRowNum = parentCopy.m_RI.numRows;
		//auto bobRowNum = childCopy.m_RI.numRows;
		std::unordered_map<uint64_t, std::pair<std::vector<int>, std::vector<int>>> rowIndexMap;
		for (uint32_t i = 0; i < parentCopy.m_RI.numRows; i++)
			rowIndexMap[parentCopy.HashTuple(i)].first.push_back(i);
		for (uint32_t i = 0; i < childCopy.m_RI.numRows; i++)
			rowIndexMap[childCopy.HashTuple(i)].second.push_back(i);
		std::unordered_set<std::string> childJoinAttrNames(childAttrNames.begin(), childAttrNames.end());
		std::vector<std::string> newChildAttrNames;
		for (auto attr : child.m_RI.attrNames)
			if (childJoinAttrNames.find(attr) == childJoinAttrNames.end())
				newChildAttrNames.push_back(attr);
		childCopy = child;
		childCopy.Project(newChildAttrNames);
		std::vector<Tuple> newTableTuples;
		std::vector<uint32_t> newAnnot1, newAnnot2;
		auto newTableNumColumns = m_RI.attrNames.size() + childCopy.m_RI.attrNames.size();
		for (auto mapPair : rowIndexMap)
		{
			for (auto i : mapPair.second.first)
			{
				for (auto j : mapPair.second.second)
				{
					newTableTuples.push_back(m_Tuples[i] + childCopy.m_Tuples[j]);
					newAnnot1.push_back(m_Annot[i]);
					newAnnot2.push_back(child.m_Annot[j]);
				}
			}
		}
		m_Tuples = newTableTuples;
		m_RI.numRows = m_Tuples.size();
		m_RI.attrNames.insert(m_RI.attrNames.end(), childCopy.m_RI.attrNames.begin(), childCopy.m_RI.attrNames.end());
		m_RI.attrTypes.insert(m_RI.attrTypes.end(), childCopy.m_RI.attrTypes.begin(), childCopy.m_RI.attrTypes.end());
		// Because tuples are not zero annotated, when an annotation is boolean, it must be 1.
		if (m_AI.isBoolean)
			m_Annot = newAnnot2;
		else if (child.m_AI.isBoolean)
			m_Annot = newAnnot1;
		else
		{
			auto ac = gParty.GetCircuit(S_ARITH);
			auto s1 = ac->PutSharedSIMDINGate(newAnnot1.size(), newAnnot1.data(), 32);
			auto s2 = ac->PutSharedSIMDINGate(newAnnot2.size(), newAnnot2.data(), 32);
			auto s_mul = ac->PutMULGate(s1, s2);
			auto s_out = ac->PutSharedOUTGate(s_mul);
			gParty.ExecCircuit();
			uint32_t *out, bitlen, nvals;
			s_out->get_clear_value_vec(&out, &bitlen, &nvals);
			gParty.Reset();
			m_Annot.resize(nvals);
			m_Annot.assign(out, out + nvals);
			delete[] out;
		}
	}

	void Relation::AnnotAdd(Relation &child)
	{
		assert(m_RI.numRows == child.m_RI.numRows && !m_AI.isBoolean && !child.m_AI.isBoolean);
		for (uint32_t i = 0; i < m_RI.numRows; i++)
		{
			m_Annot[i] += child.m_Annot[i];
			if (!IsDummy())
				assert(m_Tuples[i].IsDummy() == child.m_Tuples[i].IsDummy());
		}
	}

	void Relation::AnnotSub(Relation &child)
	{
		assert(m_RI.numRows == child.m_RI.numRows && !m_AI.isBoolean && !child.m_AI.isBoolean);
		for (uint32_t i = 0; i < m_RI.numRows; i++)
		{
			m_Annot[i] -= child.m_Annot[i];
			if (!IsDummy())
				assert(m_Tuples[i].IsDummy() == child.m_Tuples[i].IsDummy());
		}
	}

	void Relation::Union(Relation &child)
	{
		assert(m_RI.owner == child.m_RI.owner && m_RI.attrNames == child.m_RI.attrNames && m_RI.attrTypes == child.m_RI.attrTypes);
		m_Tuples.insert(m_Tuples.end(), child.m_Tuples.begin(), child.m_Tuples.end());
		m_Annot.insert(m_Annot.end(), child.m_Annot.begin(), child.m_Annot.end());
		m_RI.numRows += child.m_RI.numRows;
		m_AI.knownByOwner &= child.m_AI.knownByOwner;
		m_AI.isBoolean &= child.m_AI.isBoolean;
	}

	void Relation::AddAttr(const char *attrName, DataType attrType, uint64_t value)
	{
		//std::string sattrName(attrName);
		m_RI.attrNames.push_back(attrName);
		m_RI.attrTypes.push_back(attrType);
		if (!IsDummy())
			for (uint32_t i = 0; i < m_RI.numRows; i++)
				m_Tuples[i].push_back(value);
	}

} // namespace SECYAN