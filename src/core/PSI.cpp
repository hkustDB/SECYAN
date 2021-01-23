#include "PSI.h"
#include "poly.h"
#include "MurmurHash3.h"
#include "OEP.h"
#include <algorithm>
#include <cstdint>
#include "sharing/sharing.h"
#include "circuit/share.h"
#include "circuit/arithmeticcircuits.h"
#include <iostream>
#include "RNG.h"
#include "party.h"
#include <cassert>

namespace SECYAN
{
	using namespace std;

	// In the hash arrays, we use the first 3 elements as values of 3 hash functions that assgin elements to different bins
	// We use the last element as the value of second level hashing that maps elements to domain 2^32, which is only expected to have no collision for each bin
	inline void SingleHash(uint64_t ele, uint32_t *outarr)
	{
		MurmurHash3_x64_128((char *)&ele, 8, 14131, outarr);
		// Must use the same seed for both SERVER and CLIENT!
	}

	void PSI::AliceCuckooHash(uint32_t **AliceHashArrs, int threshold)
	{
		int totalThreshold = threshold * AliceSetSize;
		AliceIndicesHashed.resize(bucketSize, EMPTY_BUCKET);
		for (int i = 0; i < AliceSetSize; i++)
		{
			int index = i;
			int bin_id;
			uint8_t hash_id;
			bool finished = false;
			while (1)
			{
				for (hash_id = 0; hash_id < 3; hash_id++)
				{
					bin_id = AliceHashArrs[index][hash_id] % bucketSize;
					if (AliceIndicesHashed[bin_id] == EMPTY_BUCKET)
					{
						finished = true;
						AliceIndicesHashed[bin_id] = index;
						break;
					}
				}
				if (finished)
					break;
				if(totalThreshold-- < 0)
				{
					std::cerr << "Threshold exceeded in Cuckoo Hashing!" << std::endl;
					std::exit(1);
				}
				hash_id = gRNG.NextUInt16() % 3;
				bin_id = AliceHashArrs[index][hash_id] % bucketSize;
				swap(index, AliceIndicesHashed[bin_id]);
			}
		}
	}

	void PSI::BobSimpleHash(uint32_t **BobHashArrs)
	{
		int locations[3];
		BobIndexVectorsHashed.resize(bucketSize);
		for (int i = 0; i < BobSetSize; i++)
		{
			for (uint8_t hash_id = 0; hash_id < 3; hash_id++)
			{
				int loc = BobHashArrs[i][hash_id] % bucketSize;
				locations[hash_id] = loc;
				bool inserted = false;
				for (int j = 0; j < hash_id; j++)
					if (locations[j] == loc)
						inserted = true;
				if (!inserted)
					BobIndexVectorsHashed[loc].push_back(i);
			}
		}
	}

	PSI::PSI(const vector<uint64_t> &data, uint32_t AliceSetSize, uint32_t BobSetSize, Role role)
	{
		this->AliceSetSize = AliceSetSize;
		this->BobSetSize = BobSetSize;
		this->role = role;
		if(AliceSetSize < 30 && BobSetSize < 30)
		{
			std::cerr << "Error: Intersect two sets with both sizes less than 30!" << std::endl;
			std::exit(1);
		}
		bucketSize = max((uint32_t)(1.27 * AliceSetSize), 1 + BobSetSize / 256);
		int logBucketSize = floor_log2(bucketSize);
		if(logBucketSize >= 61 - 40)
		{
			std::cerr << "Error: Bucket size exceeded in PSI!" << std::endl;
			std::exit(1);
		}
		int numBinsInMegabin = max((int)(bucketSize * log2(bucketSize) / BobSetSize), 1);
		numMegabins = (bucketSize + numBinsInMegabin - 1) / numBinsInMegabin;
		int m = 3 * BobSetSize;
		int n = numMegabins;
		double logn = 0;
		if (n > 1)
			logn = log2(n);
		if (m > n * logn * 4) // Throw m balls into n bins, see  "Balls into Bins" A Simple and Tight Analysis
			megaBinLoad = std::ceil((double)m / n + sqrt(2 * logn * m / n));
		else
			megaBinLoad = std::ceil(1.41 * m / n + 1.04 * logn);
		if (n > 1)
			megaBinLoad += std::ceil(40 / logn * (1 - 1.0 / n));
		else
			megaBinLoad = m;
		// In any case, the load is at most 3*256+20=778
		gamma = 40 + logBucketSize;
		if (role == Alice)
			AlicePrepare(data);
		else
			BobPrepare(data);
	}

	void PSI::AlicePrepare(const vector<uint64_t> &AliceSet)
	{
		//gParty.Tick("OPRF");
		// Alice builds hash table
		uint32_t **AliceHashArrs = new uint32_t *[AliceSetSize];
		for (int i = 0; i < AliceSetSize; i++)
		{
			AliceHashArrs[i] = new uint32_t[4];
			SingleHash(AliceSet[i], AliceHashArrs[i]);
		}

		AliceCuckooHash(AliceHashArrs);
		cuckooTable.resize(bucketSize);
		for (int i = 0; i < bucketSize; i++)
		{
			int index = AliceIndicesHashed[i];
			cuckooTable[i] = index == EMPTY_BUCKET ? EMPTY_BUCKET : AliceSet[index];
		}
		encCuckooTable = gParty.OPRFRecv(cuckooTable);
		for (int i = 0; i < AliceSetSize; i++)
			delete[] AliceHashArrs[i];
		delete[] AliceHashArrs;
		//gParty.Tick("OPRF");
	}

	void PSI::BobPrepare(const vector<uint64_t> &BobSet)
	{
		//gParty.Tick("OPRF");
		// Bob builds hash table
		uint32_t **BobHashArrs = new uint32_t *[BobSetSize];
		for (int i = 0; i < BobSetSize; i++)
		{
			BobHashArrs[i] = new uint32_t[4];
			SingleHash(BobSet[i], BobHashArrs[i]);
		}
		BobSimpleHash(BobHashArrs);
		simpleTable.resize(bucketSize);
		for (int i = 0; i < bucketSize; i++)
		{
			simpleTable[i].reserve(BobIndexVectorsHashed[i].size());
			for (int index : BobIndexVectorsHashed[i])
				simpleTable[i].push_back(BobSet[index]);
		}
		encSimpleTable = gParty.OPRFSend(simpleTable);
		for (int i = 0; i < BobSetSize; i++)
			delete[] BobHashArrs[i];
		delete[] BobHashArrs;
		//gParty.Tick("OPRF");
	}

	inline uint64_t PSI_combine(uint64_t v, uint64_t j)
	{
		return (v & 0x1ffffffffff) | (j << 40);
	}

	vector<uint64_t> PSI::AliceIntersect()
	{
		auto circ = gParty.GetCircuit(S_BOOL);
		vector<uint64_t> AliceT(bucketSize);
		uint64_t *coeff = new uint64_t[numMegabins * megaBinLoad];
		uint32_t bitlen, nvals;
		gParty.Recv(coeff, numMegabins * megaBinLoad);
		int remainder = bucketSize % numMegabins;
		int division = bucketSize / numMegabins;
		int startBinId = 0;
		for (int i = 0; i < numMegabins; i++)
		{
			int endBinId = startBinId + division + (i < remainder);
			for (int j = startBinId; j < endBinId; j++)
				if (AliceIndicesHashed[j] != EMPTY_BUCKET)
					AliceT[j] = poly_eval(coeff + i * megaBinLoad, PSI_combine(cuckooTable[j], j), megaBinLoad) ^ encCuckooTable[j];
			startBinId = endBinId;
		}
		delete[] coeff;
		return AliceT;
	}

	vector<uint64_t> PSI::BobIntersect(vector<uint32_t> &payload, bool arith)
	{
		vector<uint64_t> BobT(bucketSize);
		// polynomial communication
		uint64_t *pointX = new uint64_t[megaBinLoad];
		uint64_t *pointY = new uint64_t[megaBinLoad];
		uint64_t *coeff = new uint64_t[megaBinLoad * numMegabins];
		for (int i = 0; i < bucketSize; i++)
			BobT[i] = gRNG.NextUInt64();

		int remainder = bucketSize % numMegabins;
		int division = bucketSize / numMegabins;
		int startBinId = 0;
		for (int i = 0; i < numMegabins; i++)
		{
			int endBinId = startBinId + division + (i < remainder);
			int pointId = 0;
			for (int j = startBinId; j < endBinId; j++)
			{
				for (int k = 0; k < simpleTable[j].size(); k++)
				{
					pointX[pointId] = PSI_combine(simpleTable[j][k], j);
					auto tmp = arith ? payload[BobIndexVectorsHashed[j][k]] - BobT[j] : payload[BobIndexVectorsHashed[j][k]] ^ BobT[j];
					pointY[pointId] = (encSimpleTable[j][k] ^ tmp) & poly_modulus;
					pointId++;
				}
			}
			if(pointId > megaBinLoad)
			{
				std::cerr << "Error: Mega-bin load not enough!" << std::endl;
				std::exit(1);
			}
			while (pointId < megaBinLoad)
			{
				pointX[pointId] = poly_modulus - pointId;
				pointY[pointId] = gRNG.NextUInt32();
				pointId++;
			}
			interpolate(pointX, pointY, megaBinLoad, coeff + i * megaBinLoad);
			startBinId = endBinId;
		}
		gParty.Send(coeff, megaBinLoad * numMegabins);
		delete[] pointX;
		delete[] pointY;
		delete[] coeff;
		return BobT;
	}

	// return the indicator
	vector<uint32_t> PSI::Intersect()
	{
		//gParty.Tick("Intersection");
		auto circ = gParty.GetCircuit(S_BOOL);
		vector<uint32_t> payload(BobSetSize, 0);
		vector<uint64_t> mask;
		if (role == Alice)
			mask = AliceIntersect();
		else
			mask = BobIntersect(payload, false);
		auto s1 = circ->PutSIMDINGate(bucketSize, mask.data(), gamma, SERVER);
		auto s2 = circ->PutSIMDINGate(bucketSize, mask.data(), gamma, CLIENT);
		auto eq = circ->PutEQGate(s1, s2);
		auto s_out = circ->PutSharedOUTGate(eq);
		gParty.ExecCircuit();
		uint32_t *indicator;
		uint32_t nvals, bitlen;
		s_out->get_clear_value_vec(&indicator, &bitlen, &nvals);
		vector<uint32_t> v_indicator(indicator, indicator + bucketSize);
		gParty.Reset();
		delete[] indicator;
		//gParty.Tick("Intersection");
		return v_indicator;
	}

	// return the payload_mask
	vector<uint32_t> PSI::IntersectWithPayload()
	{
		//gParty.Tick("IntersectWithPayload");
		assert(role == Alice);
		vector<uint64_t> mask = AliceIntersect();
		vector<uint32_t> result(mask.begin(), mask.end());
		//gParty.Tick("IntersectWithPayload");
		return result;
	}

	vector<uint32_t> PSI::IntersectWithPayload(vector<uint32_t> &payload)
	{
		if (role == Alice)
			return IntersectWithPayload();
		//gParty.Tick("IntersectWithPayload");
		vector<uint64_t> mask = BobIntersect(payload, true);
		vector<uint32_t> result(mask.begin(), mask.end());
		//gParty.Tick("IntersectWithPayload");
		return result;
	}

	vector<uint32_t> PSI::BobPermutePayloadAlicePart(vector<uint32_t> &indicator)
	{
		vector<uint32_t> tmp(bucketSize);
		int extendShareSize = BobSetSize + bucketSize;

		vector<uint32_t> rp1(extendShareSize), invrp1(extendShareSize);
		for (int i = 0; i < extendShareSize; ++i)
			rp1[i] = i;

		shuffle(rp1.begin(), rp1.end(), gRNG.stdrng);
		for (int i = 0; i < extendShareSize; ++i)
			invrp1[rp1[i]] = i;

		std::vector<uint32_t> zero(extendShareSize, 0);
		auto out = PermutorPermute(rp1, zero);

		vector<uint64_t> BobRev = BobIntersect(invrp1, false);

		// modify PSI, if indicator1[i] ^ indicator2[i] = true, remain (AliceRev, BobRev); else change AliceRev + BobRev = pi^-1(B+i)
		auto circ = gParty.GetCircuit(S_BOOL);
		auto s_m0 = circ->PutSIMDINGate(bucketSize, invrp1.data() + BobSetSize, 32, gParty.GetRole());
		auto s_m1 = circ->PutSharedSIMDINGate(bucketSize, BobRev.data(), 32);
		auto s_b = circ->PutSharedSIMDINGate(bucketSize, indicator.data(), 1);
		auto s_mux = circ->PutMUXGate(s_m1, s_m0, s_b);
		circ->PutOUTGate(s_mux, gParty.GetRevRole());
		gParty.ExecCircuit();
		gParty.Reset();
		return SenderExtendedPermute(out, bucketSize);
	}

	vector<uint32_t> PSI::AlicePermutePayloadAlicePart(vector<uint32_t> &payload, vector<uint32_t> &indicator)
	{
		int extendShareSize = BobSetSize + bucketSize;

		// each party extend the shares
		vector<uint32_t> extendValueShare(payload);
		extendValueShare.resize(extendShareSize, 0);
		auto out = SenderPermute(extendValueShare);
		vector<uint64_t> AliceRev = AliceIntersect();
		auto circ = gParty.GetCircuit(S_BOOL);
		auto s_m0 = circ->PutDummySIMDINGate(bucketSize, 32);
		auto s_m1 = circ->PutSharedSIMDINGate(bucketSize, AliceRev.data(), 32);
		auto s_b = circ->PutSharedSIMDINGate(bucketSize, indicator.data(), 1);
		auto s_mux = circ->PutMUXGate(s_m1, s_m0, s_b);
		auto s_k = circ->PutOUTGate(s_mux, gParty.GetRole());
		gParty.ExecCircuit();
		uint32_t *ki, bitlen, nvals;
		s_k->get_clear_value_vec(&ki, &bitlen, &nvals);
		gParty.Reset();
		std::vector<uint32_t> vki(ki, ki + bucketSize);
		return PermutorExtendedPermute(vki, out);
	}

	// if indicator[i]=1, then AliceSet[i] is in BobSet, otherwise not.
	// output[i]_1+output[i]_2=BobPayload(x_i) if x_i is in BobSet, otherwise random
	vector<uint32_t> PSI::CombineSharedPayload(vector<uint32_t> &payload, vector<uint32_t> &indicator)
	{
		//gParty.Tick("CombineSharedPayload");
		vector<uint64_t> payload1;
		vector<uint32_t> payload2;
		if (role == Alice)
		{
			payload1 = AliceIntersect();
			payload2 = AlicePermutePayloadAlicePart(payload, indicator);
		}
		else
		{
			payload1 = BobIntersect(payload, true);
			payload2 = BobPermutePayloadAlicePart(indicator);
		}
		// auto bc = gParty.GetCircuit(S_BOOL);
		// auto ac = gParty.GetCircuit(S_ARITH);
		// bc->PutPrintValueGate(bc->PutSharedSIMDINGate(indicator.size(), indicator.data(), 1), "indicator");
		// ac->PutPrintValueGate(ac->PutSharedSIMDINGate(payload1.size(), payload1.data(), 32), "payload1");
		// ac->PutPrintValueGate(ac->PutSharedSIMDINGate(payload2.size(), payload2.data(), 32), "payload2");
		// gParty.ExecCircuit();
		// gParty.Reset();
		for (int i = 0; i < bucketSize; i++)
			payload2[i] += payload1[i];
		//gParty.Tick("CombineSharedPayload");
		return payload2;
	}

	vector<uint32_t> PSI::CuckooToAliceArray()
	{
		assert(role == Alice);
		vector<uint32_t> permutedIndices(AliceSetSize);
		for (int i = 0; i < bucketSize; i++)
		{
			int index = AliceIndicesHashed[i];
			if (index != EMPTY_BUCKET)
				permutedIndices[index] = i;
		}
		return permutedIndices;
	}

} // namespace SECYAN
