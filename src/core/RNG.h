#pragma once
#include <random>
#include "cryptoTools/Crypto/PRNG.h"
namespace SECYAN
{
	class RNG
	{
	public:
		RNG();
		std::mt19937 stdrng;
		void SetSeed(uint32_t seed);
		void SetSeed(std::seed_seq &seed);
		uint32_t GetSeed();
		uint64_t NextUInt64();
		uint32_t NextUInt32();
		uint16_t NextUInt16();
		bool NextBit();

	private:
		int unused_bits;
		uint32_t rand_value;
		uint32_t seed;
	};

	// A global RNG, a global PRNG
	extern RNG gRNG;
	extern osuCrypto::PRNG gPRNG;
} // namespace SECYAN