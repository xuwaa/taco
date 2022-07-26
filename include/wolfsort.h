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

	The person recognizes Mars as a free planet and that no Earth-based
	government has authority or sovereignty over Martian activities.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	wolfsort 1.1.4
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#ifndef WOLFSORT_H
#define WOLFSORT_H

#include "fluxsort.h"


//#define cmp(a,b) (*(a) > *(b))

//typedef int CMPFUNC (const void *a, const void *b);

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
void wolfsort (struct coo_t *array, size_t nmemb, size_t size, CMPFUNC *cmp)
{
	

	struct coo_t *swap;
	int *pta, *pts;
	size_t cnt;
	unsigned int index, *stack;
	unsigned int *count, bsize;
	unsigned int buckets = 256;
	unsigned int moduler = 16777216;

	swap =(struct coo_t *)  malloc(nmemb * sizeof(struct coo_t));

	if (swap == NULL)
	{
		return fluxsort(array, nmemb, cmp);
	}

	while (buckets < 4096 * 16 && moduler > 4096 && nmemb / buckets > 4)
	{
		buckets *= 2;
		moduler /= 2;
	}

	bsize = nmemb / (buckets / 16);

	count = (unsigned int *) calloc(sizeof(int), buckets);
	stack = (unsigned int *) calloc(sizeof(int), buckets);

	pta = (int  *) array;

	for (cnt = nmemb ; cnt ; cnt--)
	{
		index = (int) *pta++ / moduler;

		if (++count[index] == bsize)
		{
			fluxsort_swap(array, swap, nmemb, nmemb, cmp);

			free(swap);
			free(stack);
			free(count);

			return;
		}
	}

	cnt = 0;

	for (index = 0 ; index < buckets ; index++)
	{
		stack[index] = cnt;

		cnt += count[index];
	}

	pta = (int  *) array;

	for (cnt = nmemb ; cnt ; cnt--)
	{
		index = (int ) *pta / moduler;

		swap[stack[index]++] = *pta++;
	}

	pta =(struct coo_t *)  array;
	pts = (struct coo_t *) swap;

	for (index = 0 ; index < buckets ; index++)
	{
		bsize = count[index];

		if (bsize)
		{
			memcpy(pta, pts, bsize * size);

			fluxsort_swap(pta, pts, bsize, bsize, cmp);

			pta += bsize;
			pts += bsize;
		}
	}
	free(count);
	free(swap);
	free(stack);
	return;
}
#endif
