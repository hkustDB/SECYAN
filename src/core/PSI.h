#pragma once
#include <vector>
#include <cstdint>

namespace SECYAN
{
	class PSI
	{
	public:
		const int EMPTY_BUCKET = -1;
		int bucketSize;

		enum Role
		{
			Alice,
			Bob
		};
		PSI(const std::vector<uint64_t> &data, uint32_t AliceSetSize, uint32_t BobSetSize, Role role);
		// Without payload, return indicator: indicator[i](Alice) + indicator[i](Bob) = 1 iff A[i]\in B
		std::vector<uint32_t> Intersect();
		// (Called by Alice) With payload, result[i](Alice) + result[i](Bob) = payload[j] if A[i]=B[j]
		std::vector<uint32_t> IntersectWithPayload();
		// Called by Bob. If called by Alice, then payload will be ignored
		std::vector<uint32_t> IntersectWithPayload(std::vector<uint32_t> &payload);
		std::vector<uint32_t> CombineSharedPayload(std::vector<uint32_t> &payload, std::vector<uint32_t> &indicator);
		std::vector<uint32_t> CuckooToAliceArray();
		std::vector<uint32_t> GetIndicators(std::vector<uint64_t> &mask);

	private:
		int AliceSetSize, BobSetSize, numMegabins, megaBinLoad, gamma;
		Role role;
		std::vector<uint64_t> cuckooTable, encCuckooTable;
		std::vector<std::vector<uint64_t>> simpleTable, encSimpleTable;
		std::vector<int> AliceIndicesHashed;
		std::vector<std::vector<int>> BobIndexVectorsHashed;
		void AliceCuckooHash(uint32_t **AliceHashArrs, int threshold = 3);
		void BobSimpleHash(uint32_t **BobHashArrs);
		void AlicePrepare(const std::vector<uint64_t> &AliceSet);
		void BobPrepare(const std::vector<uint64_t> &BobSet);
		std::vector<uint64_t> AliceIntersect();
		std::vector<uint64_t> BobIntersect(std::vector<uint32_t> &payload, bool arith);
		std::vector<uint32_t> BobPermutePayloadAlicePart(std::vector<uint32_t> &indicator);
		std::vector<uint32_t> AlicePermutePayloadAlicePart(std::vector<uint32_t> &payload, std::vector<uint32_t> &indicator);
	};
} // namespace SECYAN