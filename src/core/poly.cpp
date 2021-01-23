#include "poly.h"

namespace SECYAN
{

	// require 0<x<=p^2. Note: it is possible to return p instead of 0
	uint64_t fast_mod(uint64_t x)
	{
		return (x >> 61) + (x & poly_modulus);
	}

	// x, y < 2^62
	uint64_t mod_mul(uint64_t x, uint64_t y)
	{
		uint64_t x1, x2, y1, y2, z1, z2, ans;
		x1 = x >> 31;
		x2 = x & 0x7fffffff;
		y1 = y >> 31;
		y2 = y & 0x7fffffff;
		ans = x1 * y2 + x2 * y1;
		z1 = ans >> 30;
		z2 = ans & 0x3fffffff;
		ans = z1 + (z2 << 31) + x1 * (y1 << 1) + x2 * y2;
		return fast_mod(ans);
	}

	// find x such that x*v=1 (mod p). v>0.
	uint64_t mod_inverse(uint64_t v)
	{
		long long a = poly_modulus, b = (long long)v, x = 1, y = 0, r = 0, s = 1;
		long long q, tmp;
		while (b != 0)
		{
			tmp = a % b;
			q = a / b;
			a = b;
			b = tmp;
			tmp = x;
			x = r;
			r = tmp - q * r;
			tmp = y;
			y = s;
			s = tmp - q * s;
		}
		if (y < 0)
			y += poly_modulus;
		return (uint64_t)y;
	}

	// coeff[i] (beginning from 0)is multiplied by x^i
	// for test only
	uint64_t poly_eval(uint64_t *coeff, uint64_t x, int size)
	// does a Horner evaluation
	{
		uint64_t acc = 0;
		for (int i = size - 1; i >= 0; i--)
		{
			acc = mod_mul(acc, x) + coeff[i];
		}
		return acc >= poly_modulus ? acc - poly_modulus : acc;
	}

	void interpolate(uint64_t *X, uint64_t *Y, int size, uint64_t *coeff)
	{
		int k, i;
		uint64_t t1, t2;
		uint64_t *prod = new uint64_t[size];
		for (i = 0; i < size; i++)
		{
			prod[i] = X[i];
			coeff[i] = 0;
		}
		for (k = 0; k < size; k++)
		{
			t1 = 1;
			for (i = k - 1; i >= 0; i--)
				t1 = mod_mul(t1, X[k]) + prod[i];
			if (t1 >= poly_modulus)
				t1 -= poly_modulus;
			t2 = 0;
			for (i = k - 1; i >= 0; i--)
				t2 = mod_mul(t2, X[k]) + coeff[i];
			t2 = fast_mod(2 * poly_modulus - t2 + Y[k]);
			t1 = mod_mul(t2, mod_inverse(t1));
			for (i = 0; i < k; i++)
			{
				coeff[i] += mod_mul(t1, prod[i]);
				if (coeff[i] >= poly_modulus)
					coeff[i] -= poly_modulus;
			}

			coeff[k] = t1;
			if (k < size - 1)
			{
				if (k == 0)
					prod[0] = poly_modulus - prod[0];
				else
				{
					t1 = poly_modulus - X[k];
					prod[k] = t1 + prod[k - 1];
					if (prod[k] >= poly_modulus)
						prod[k] -= poly_modulus;
					for (i = k - 1; i >= 1; i--)
					{
						prod[i] = mod_mul(prod[i], t1) + prod[i - 1];
						if (prod[i] >= poly_modulus)
							prod[i] -= poly_modulus;
					}
					prod[0] = mod_mul(prod[0], t1);
				}
			}
		}
		delete[] prod;
	}

} // namespace SECYAN