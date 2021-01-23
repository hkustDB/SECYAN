#pragma once
#include <cstdint>

namespace SECYAN{
static const uint64_t poly_modulus = 0x1fffffffffffffffull;
uint64_t poly_eval(uint64_t* coeff, uint64_t x, int size);
void interpolate(uint64_t* X, uint64_t* Y, int size, uint64_t* coeff);
}