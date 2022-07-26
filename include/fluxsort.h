/*
	Copyright (C) 2014-2021 Igor van den Hoven ivdhoven@gmail.com
*/

/*
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	fluxsort 1.1.5.2

*/
#define FLUX_OUT 24
#define _FLUXSORT_H_
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include "quadsort.h"

// Determine whether to use mergesort or quicksort
typedef int CMPFUNC(const void *a, const void *b);
/*
struct coo_t
{
	int32_t idx0;
	int32_t idx1;
	int32_t idx2;
	int32_t idx3;
	double  val;
};
*/
size_t flux_analyze(struct  coo_t *array, size_t nmemb, CMPFUNC *cmp)
{
	char loop, dist;
	size_t cnt, balance = 0, streaks = 0;
	struct  coo_t *pta, *ptb, swap;

	pta = array;

	for (cnt = nmemb; cnt > 16; cnt -= 16)
	{
		for (dist = 0, loop = 16; loop; loop--)
		{
			dist += cmp(pta, pta + 1) > 0; pta++;
		}
		streaks += (dist == 0) | (dist == 16);
		balance += dist;
	}

	while (--cnt)
	{
		balance += cmp(pta, pta + 1) > 0;
		pta++;
	}

	if (balance == 0)
	{
		return 1;
	}

	if (balance == nmemb - 1)
	{
		ptb = pta + 1;
		pta = array;

		cnt = nmemb / 2;

		do
		{
			swap = *pta; *pta++ = *--ptb; *ptb = swap;
		} while (--cnt);

		return 1;
	}

	if (streaks >= nmemb / 40)
	{
		quadsort(array, nmemb, cmp);

		return 1;
	}
	return 0;
}

// The next 5 functions are used for pivot selection

struct  coo_t median_of_sqrt(struct  coo_t *array, struct  coo_t *swap, struct  coo_t *ptx, size_t nmemb, CMPFUNC *cmp)
{
	struct  coo_t *pta, *pts;
	size_t cnt, sqrt, div;

	sqrt = nmemb > 262144 ? 256 : 128;

	div = nmemb / sqrt;

	pta = ptx + rand() % sqrt;

	pts = ptx == array ? swap : array;

	for (cnt = 0; cnt < sqrt; cnt++)
	{
		pts[cnt] = pta[0];

		pta += div;
	}
	quadsort_swap(pts, pts + sqrt, sqrt, sqrt, cmp);

	return pts[sqrt / 2];
}

struct  coo_t median_of_five(struct  coo_t *array, size_t v0, size_t v1, size_t v2, size_t v3, size_t v4, CMPFUNC *cmp)
{
	struct  coo_t swap[6], *pta;
	size_t x, y, z;

	swap[2] = array[v0];
	swap[3] = array[v1];
	swap[4] = array[v2];
	swap[5] = array[v3];

	pta = swap + 2;

	x = cmp(pta, pta + 1) > 0; y = !x; swap[0] = pta[y]; pta[0] = pta[x]; pta[1] = swap[0]; pta += 2;
	x = cmp(pta, pta + 1) > 0; y = !x; swap[0] = pta[y]; pta[0] = pta[x]; pta[1] = swap[0]; pta -= 2;
	x = cmp(pta, pta + 2) > 0; y = !x; swap[0] = pta[0]; swap[1] = pta[2]; pta[0] = swap[x]; pta[2] = swap[y]; pta++;
	x = cmp(pta, pta + 2) > 0; y = !x; swap[0] = pta[0]; swap[1] = pta[2]; pta[0] = swap[x]; pta[2] = swap[y];

	pta[2] = array[v4];

	x = cmp(pta, pta + 1) > 0;
	y = cmp(pta, pta + 2) > 0;
	z = cmp(pta + 1, pta + 2) > 0;

	return pta[(x == y) + (y ^ z)];
}

struct  coo_t median_of_twentyfive(struct  coo_t *array, size_t nmemb, CMPFUNC *cmp)
{
	struct  coo_t swap[5];
	size_t div = nmemb / 64;

	swap[0] = median_of_five(array, div * 4, div * 1, div * 2, div * 8, div * 10, cmp);
	swap[1] = median_of_five(array, div * 16, div * 12, div * 14, div * 18, div * 20, cmp);
	swap[2] = median_of_five(array, div * 32, div * 24, div * 30, div * 34, div * 38, cmp);
	swap[3] = median_of_five(array, div * 48, div * 42, div * 44, div * 50, div * 52, cmp);
	swap[4] = median_of_five(array, div * 60, div * 54, div * 56, div * 62, div * 63, cmp);

	return median_of_five(swap, 0, 1, 2, 3, 4, cmp);
}

size_t median_of_three(struct  coo_t *array, size_t v0, size_t v1, size_t v2, CMPFUNC *cmp)
{
	size_t v[3] = { v0, v1, v2 };
	char x, y, z;

	x = cmp(array + v0, array + v1) > 0;
	y = cmp(array + v0, array + v2) > 0;
	z = cmp(array + v1, array + v2) > 0;

	return v[(x == y) + (y ^ z)];
}

struct  coo_t median_of_nine(struct  coo_t *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t x, y, z, div = nmemb / 16;

	x = median_of_three(array, div * 2, div * 1, div * 4, cmp);
	y = median_of_three(array, div * 8, div * 6, div * 10, cmp);
	z = median_of_three(array, div * 14, div * 12, div * 15, cmp);

	return array[median_of_three(array, x, y, z, cmp)];
}

void flux_partition(struct  coo_t *array, struct  coo_t *swap, struct  coo_t *ptx, struct  coo_t *ptp, size_t nmemb, CMPFUNC *cmp);

// As per suggestion by Marshall Lochbaum to improve generic data handling

void flux_reverse_partition(struct  coo_t *array, struct  coo_t *swap, struct  coo_t *ptx, struct  coo_t *piv, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size, s_size;

	{
		size_t cnt, val, m;
		struct  coo_t *pts = swap;

		for (m = 0, cnt = nmemb / 8; cnt; cnt--)
		{
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
		}

		for (cnt = nmemb % 8; cnt; cnt--)
		{
			val = cmp(piv, ptx) > 0; pts[-m] = array[m] = *ptx++; m += val; pts++;
		}
		a_size = m;
		s_size = nmemb - a_size;
	}
	memcpy(array + a_size, swap, s_size * sizeof(struct  coo_t));

	if (s_size <= a_size / 16 || a_size <= FLUX_OUT)
	{
		return quadsort_swap(array, swap, a_size, a_size, cmp);
	}
	flux_partition(array, swap, array, piv, a_size, cmp);
}

size_t flux_default_partition(struct  coo_t *array, struct  coo_t *swap, struct  coo_t *ptx, struct  coo_t *piv, size_t nmemb, CMPFUNC *cmp)
{
	size_t cnt, val, m = 0;

	for (cnt = nmemb / 8; cnt; cnt--)
	{
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
	}

	for (cnt = nmemb % 8; cnt; cnt--)
	{
		val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
	}
	return m;
}

void flux_partition(struct  coo_t *array, struct  coo_t *swap, struct  coo_t *ptx, struct  coo_t *piv, size_t nmemb, CMPFUNC *cmp)
{
	size_t a_size = 0, s_size;

	while (1)
	{
		--piv;

		if (nmemb <= 2048)
		{
			*piv = median_of_nine(ptx, nmemb, cmp);
		}
		else if (nmemb <= 65536)
		{
			*piv = median_of_twentyfive(ptx, nmemb, cmp);
		}
		else
		{
			*piv = median_of_sqrt(array, swap, ptx, nmemb, cmp);
		}

		if (a_size && cmp(piv + 1, piv) <= 0)
		{
			return flux_reverse_partition(array, swap, array, piv, nmemb, cmp);
		}
		a_size = flux_default_partition(array, swap, ptx, piv, nmemb, cmp);
		s_size = nmemb - a_size;

		if (a_size <= s_size / 16 || s_size <= FLUX_OUT)
		{
			if (s_size == 0)
			{
				return flux_reverse_partition(array, swap, array, piv, a_size, cmp);
			}
			memcpy(array + a_size, swap, s_size * sizeof(struct  coo_t));
			quadsort_swap(array + a_size, swap, s_size, s_size, cmp);
		}
		else
		{
			flux_partition(array + a_size, swap, swap, piv, s_size, cmp);
		}

		if (s_size <= a_size / 16 || a_size <= FLUX_OUT)
		{
			return quadsort_swap(array, swap, a_size, a_size, cmp);
		}
		nmemb = a_size;
		ptx = array;
	}
}

void fluxsort(struct  coo_t *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		tail_swap(array, nmemb, cmp);
	}
	else if (flux_analyze(array, nmemb, cmp) == 0)
	{
		struct  coo_t *swap =(struct  coo_t *) malloc(nmemb * sizeof(struct  coo_t));

		if (swap == NULL)
		{
			return quadsort(array, nmemb, cmp);
		}
		flux_partition(array, swap, array, swap + nmemb, nmemb, cmp);

		free(swap);
	}
}

void fluxsort_swap(struct  coo_t *array, struct  coo_t *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		tail_swap(array, nmemb, cmp);
	}
	else if (swap_size < nmemb)
	{
		quadsort_swap(array, swap, swap_size, nmemb, cmp);
	}
	else if (flux_analyze(array, nmemb, cmp) == 0)
	{
		flux_partition(array, swap, array, swap + nmemb, nmemb, cmp);
	}
}
