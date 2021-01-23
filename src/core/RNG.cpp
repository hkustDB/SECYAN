#include "RNG.h"
#include <ctime>
using namespace std;

namespace SECYAN
{
	RNG gRNG;
	osuCrypto::PRNG gPRNG;

	RNG::RNG()
	{
		seed = 14131;
#ifdef NDEBUG // In release mode, we don't use fixed seed every time
		seed = time(0);
#endif
		stdrng.seed(seed);
	}

	void RNG::SetSeed(uint32_t seed)
	{
		this->seed = seed;
		stdrng.seed(seed);
	}

	void RNG::SetSeed(seed_seq &seed)
	{
		stdrng.seed(seed);
	}

	uint32_t RNG::GetSeed()
	{
		return seed;
	}

	uint64_t RNG::NextUInt64()
	{
		return (((uint64_t)stdrng()) << 32) | stdrng();
	}

	uint32_t RNG::NextUInt32()
	{
		return stdrng();
	}

	uint16_t RNG::NextUInt16()
	{
		uint16_t ret;
		if (unused_bits < 16)
		{
			uint32_t r = stdrng();
			ret = r & 0xffff;
			rand_value = (rand_value << 16) | (r >> 16);
			unused_bits += 16;
		}
		else
		{
			ret = rand_value & 0xffff;
			rand_value >>= 16;
			unused_bits -= 16;
		}
		return ret;
	}

	bool RNG::NextBit()
	{
		if (unused_bits == 0)
		{
			rand_value = stdrng();
			unused_bits = 32;
		}
		bool ret = rand_value & 1;
		rand_value >>= 1;
		unused_bits--;
		return ret;
	}
} // namespace SECYAN