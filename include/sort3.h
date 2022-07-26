#pragma once

#define _SORT_H_
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include<string.h>
#include <errno.h>
/*/ home / xwb / taco / transpose / taco / apps / tensor_4_fusion
'4a037ec7b4c80987320379feba850bc31de8c0b9949ef1045551c4866fe3c1c6'
*/
int cmp(const void *p, const void *q, int *a, int length);

struct coo_t {
	int  idx[3];
	int  val;
};


/*
	blitsort 1.1.5.1
*/
/***
**下面前一段是基本的归并排序的实现部分
**sort为普通归并排序的实现
***/
/**
void Gqsort(struct coo_t *a, int first, int mid, int last, struct coo_t *temp,int* a, int length)
{
	int n = mid, m = last;
	int k = 0;
	int i = first, j = mid + 1;
	while (i <= n && j <= m)  // 两边同时进行
	{
		if (cmp(a + i, a + j))  //  i和j进行比较
		{
			temp[k++] = a[j++];  // 如果i <= j,则i的值移动到temp里面
		}
		else
		{
			temp[k++] = a[i++];   // 如果i > j,则j的值移动到temp里面
		}
	}
	while (i <= n)
	{
		temp[k++] = a[i++];
	}
	while (j <= m)
	{
		temp[k++] = a[j++];
	}
	for (i = 0; i < k; i++)
	{
		a[first + i] = temp[i];
	}
}

void G_sort(struct coo_t*a, int first, int last, struct coo_t*temp,int* a, int length)
{
	if (first < last)  //  当同时到达一个数时结束判断
	{
		int mid = (first + last) / 2;  // 找到中间值
		G_sort(a, first, mid, temp, cmp);  //  递归函数左边
		G_sort(a, mid + 1, last, temp, cmp);   // 右边
		G_qsort(a, first, mid, last, temp, cmp);  // 进行排序
	}
}

void sort(struct coo_t*a, int n,int* a, int length)
{
	struct coo_t* p = (struct coo_t *)malloc(sizeof(struct coo_t) * n);
	G_sort(a, 0, n - 1, p, cmp);
	free(p);

}

**/

/***
*下面的函数都是为了服务于函数blitsort，其表示优化后的归并排序
*
***/

// the next six functions are used for sorting 0 to 31 elements
void parity_merge_two(struct coo_t *array, struct coo_t *swap, int x, int y, struct coo_t * ptl, struct coo_t * ptr, struct coo_t *pts, int* a, int length)
{  //比较四个数，两个两个一组进行前后比较，将两组共四个数
	ptl = array + 0; ptr = array + 2; pts = swap + 0;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;
	*pts = cmp(ptl, ptr, a, length) <= 0 ? *ptl : *ptr;

	ptl = array + 1; ptr = array + 3; pts = swap + 3;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;
	*pts = cmp(ptl, ptr, a, length) > 0 ? *ptl : *ptr;
}

void parity_merge_four(struct coo_t *array, struct coo_t *swap, int x, int y, struct coo_t * ptl, struct coo_t * ptr, struct coo_t *pts, int* a, int length)
{  //比较八个数，在前后各四个两组间使用四个数的比较方法
	ptl = array + 0; ptr = array + 4; pts = swap;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;
	*pts = cmp(ptl, ptr, a, length) <= 0 ? *ptl : *ptr;

	ptl = array + 3; ptr = array + 7; pts = swap + 7;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;
	x = cmp(ptl, ptr, a, length) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;
	*pts = cmp(ptl, ptr, a, length) > 0 ? *ptl : *ptr;
}
void unguarded_insert(struct coo_t *array, size_t offset, size_t nmemb, int* a, int length)
{//给数组从小到大排序，--pta是end的前一项，排序截至nmemb的数

	struct coo_t key, *pta, *end;//sitruct coo_t char
	size_t i, top;

	for (i = offset; i < nmemb; i++)
	{
		pta = end = array + i;

		if (cmp(--pta, end, a, length) <= 0)
		{
			continue;
		}

		key = *end;

		if (cmp(array, &key, a, length) > 0)
		{
			top = i;

			do
			{
				*end-- = *pta--;
			} while (--top);

			*end = key;
		}
		else
		{
			do
			{
				*end-- = *pta--;
			} while (cmp(pta, &key, a, length) > 0);

			*end = key;
		}
	}
}

void parity_swap_four(struct coo_t *array, int* a, int length)
{//将四个数分成两组，前两个有序放到swap数组，后两个有序放到swap数组，在用归并排序前后两组数据
	struct coo_t swap[4];
	struct coo_t ptl[1];
	struct coo_t  ptr[1];
	struct coo_t pts[1];
	unsigned char x, y;

	x = cmp(array + 0, array + 1, a, length) <= 0; y = !x; swap[0 + y] = array[0]; swap[0 + x] = array[1];
	x = cmp(array + 2, array + 3, a, length) <= 0; y = !x; swap[2 + y] = array[2]; swap[2 + x] = array[3];

	parity_merge_two(swap, array, x, y, ptl, ptr, pts, a, length);

}

void parity_swap_eight(struct coo_t *array, int* a, int length)
{//排序8个数
	struct coo_t swap[8];
	struct coo_t ptl[1];
	struct coo_t  ptr[1];
	struct coo_t pts[1];
	unsigned char x = 0;
	unsigned char y = 0;

	if (cmp(array + 0, array + 1, a, length) > 0) { swap[0] = array[0]; array[0] = array[1]; array[1] = swap[0]; }
	if (cmp(array + 2, array + 3, a, length) > 0) { swap[0] = array[2]; array[2] = array[3]; array[3] = swap[0]; }
	if (cmp(array + 4, array + 5, a, length) > 0) { swap[0] = array[4]; array[4] = array[5]; array[5] = swap[0]; }
	if (cmp(array + 6, array + 7, a, length) > 0) { swap[0] = array[6]; array[6] = array[7]; array[7] = swap[0]; }
	else if (cmp(array + 1, array + 2, a, length) <= 0 && cmp(array + 3, array + 4, a, length) <= 0 && cmp(array + 5, array + 6, a, length) <= 0)
	{
		return;
	}
	parity_merge_two(array + 0, swap + 0, x, y, ptl, ptr, pts, a, length);
	parity_merge_two(array + 4, swap + 4, x, y, ptl, ptr, pts, a, length);

	parity_merge_four(swap, array, x, y, ptl, ptr, pts, a, length);
}

void parity_merge(struct coo_t *dest, struct coo_t *from, size_t block, size_t nmemb, int* a, int length)
{//目标数组 交换后数组 块长 块尾  比较
	//排序两组块
	struct coo_t *ptl, *ptr, *tpl, *tpr, *tpd, *ptd;
	unsigned char x, y;

	ptl = from;
	ptr = from + block;
	ptd = dest;
	tpl = from + block - 1;
	tpr = from + nmemb - 1;
	tpd = dest + nmemb - 1;

	for (block--; block; block--)
	{//前向后向一起排，每次循环排序两个
		x = cmp(ptl, ptr, a, length) <= 0; y = !x; ptd[x] = *ptr; ptr += y; ptd[y] = *ptl; ptl += x; ptd++;
		x = cmp(tpl, tpr, a, length) <= 0; y = !x; tpd--; tpd[x] = *tpr; tpr -= x; tpd[y] = *tpl; tpl -= y;
	}
	*ptd = cmp(ptl, ptr, a, length) <= 0 ? *ptl : *ptr;
	*tpd = cmp(tpl, tpr, a, length) > 0 ? *tpl : *tpr;
}

void parity_swap_sixteen(struct coo_t *array, int* a, int length)
{//四个数一排，排序16个数
	struct coo_t swap[16];
	struct coo_t ptl[1];
	struct coo_t  ptr[1];
	struct coo_t pts[1];
	unsigned char x = 0;
	unsigned char y = 0;

	parity_swap_four(array + 0, a, length);
	parity_swap_four(array + 4, a, length);
	parity_swap_four(array + 8, a, length);
	parity_swap_four(array + 12, a, length);

	if (cmp(array + 3, array + 4, a, length) <= 0 && cmp(array + 7, array + 8, a, length) <= 0 && cmp(array + 11, array + 12, a, length) <= 0)
	{
		return;
	}
	parity_merge_four(array + 0, swap + 0, x, y, ptl, ptr, pts, a, length);
	parity_merge_four(array + 8, swap + 8, x, y, ptl, ptr, pts, a, length);

	parity_merge(array, swap, 8, 16, a, length);
}

void tail_swap(struct coo_t *array, size_t nmemb, int* a, int length)
{
	if (nmemb < 4)
	{
		unguarded_insert(array, 1, nmemb, a, length);
		return;
	}
	if (nmemb < 8)
	{
		parity_swap_four(array, a, length);
		unguarded_insert(array, 4, nmemb, a, length);
		return;
	}
	if (nmemb < 16)
	{
		parity_swap_eight(array, a, length);
		unguarded_insert(array, 8, nmemb, a, length);
		return;
	}
	parity_swap_sixteen(array, a, length);
	unguarded_insert(array, 16, nmemb, a, length);
}

// the next three functions create sorted blocks of 32 elements接下来的三个函数创建32个元素的排序块

void parity_tail_swap_eight(struct coo_t *array, int* a, int length)
{//前四个排序好，后四个不清楚
	struct coo_t swap[8];
	struct coo_t ptl[1];
	struct coo_t  ptr[1];
	struct coo_t pts[1];
	unsigned char x = 0;
	unsigned char y = 0;

	if (cmp(array + 4, array + 5, a, length) > 0) { swap[5] = array[4]; array[4] = array[5]; array[5] = swap[5]; }
	if (cmp(array + 6, array + 7, a, length) > 0) { swap[7] = array[6]; array[6] = array[7]; array[7] = swap[7]; }
	else

		if (cmp(array + 3, array + 4, a, length) <= 0 && cmp(array + 5, array + 6, a, length) <= 0)
		{
			return;
		}
	swap[0] = array[0]; swap[1] = array[1]; swap[2] = array[2]; swap[3] = array[3];

	parity_merge_two(array + 4, swap + 4, x, y, ptl, ptr, pts, a, length);

	parity_merge_four(swap, array, x, y, ptl, ptr, pts, a, length);
}

void parity_tail_flip_eight(struct coo_t *array, int* a, int length)
{//前四个排好序，后四个排好序，前后关系不清楚
	struct coo_t swap[8];
	struct coo_t ptl[1];
	struct coo_t  ptr[1];
	struct coo_t pts[1];
	unsigned char x = 0;
	unsigned char y = 0;

	if (cmp(array + 3, array + 4, a, length) <= 0)
	{
		return;
	}
	swap[0] = array[0]; swap[1] = array[1]; swap[2] = array[2]; swap[3] = array[3];
	swap[4] = array[4]; swap[5] = array[5]; swap[6] = array[6]; swap[7] = array[7];

	parity_merge_four(swap, array, x, y, ptl, ptr, pts, a, length);
}
void tail_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, int* a, int length);

size_t quad_swap(struct coo_t *array, size_t nmemb, int* a, int length)
{//每八个数排好序，排32个，确定是否有顺序和逆序，减少开销
	struct coo_t swap[32];
	size_t count, reverse;
	struct coo_t *pta, *pts, *pte, tmp;

	pta = array;

	count = nmemb / 8 * 2;

	while (count--)
	{
		if (cmp(&pta[0], &pta[1], a, length) > 0)//前两个逆序
		{
			if (cmp(&pta[2], &pta[3], a, length) > 0)
			{
				if (cmp(&pta[1], &pta[2], a, length) > 0)
				{//逆序
					pts = pta;
					pta += 4;
					goto swapper;
				}
				tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}
			tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp;
		}
		else if (cmp(&pta[2], &pta[3], a, length) > 0)//前两个有序，后两个无序
		{
			tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
		}
		//排序四个数
		if (cmp(&pta[1], &pta[2], a, length) > 0)
		{
			if (cmp(&pta[0], &pta[2], a, length) <= 0)
			{
				if (cmp(&pta[1], &pta[3], a, length) <= 0)
				{
					tmp = pta[1]; pta[1] = pta[2]; pta[2] = tmp;
				}
				else
				{
					tmp = pta[1]; pta[1] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}
			}
			else if (cmp(&pta[0], &pta[3], a, length) > 0)
			{
				tmp = pta[1]; pta[1] = pta[3]; pta[3] = tmp; tmp = pta[0]; pta[0] = pta[2]; pta[2] = tmp;
			}
			else if (cmp(&pta[1], &pta[3], a, length) <= 0)
			{
				tmp = pta[1]; pta[1] = pta[0]; pta[0] = pta[2]; pta[2] = tmp;
			}
			else
			{
				tmp = pta[1]; pta[1] = pta[0]; pta[0] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}
		}
		count--;

		parity_tail_swap_eight(pta, a, length);//排序好前四个，排序整个八个数

		pta += 8;

		continue;

	swapper:
		//将排序好的前两个数和排序好的后两个数的大小情况列举出来，进行排序
		if (count--)
		{
			if (cmp(&pta[0], &pta[1], a, length) > 0)
			{
				if (cmp(&pta[2], &pta[3], a, length) > 0)
				{
					if (cmp(&pta[1], &pta[2], a, length) > 0)
					{
						if (cmp(&pta[-1], &pta[0], a, length) > 0)
						{//整体前向都逆序
							pta += 4;

							goto swapper;
						}
					}
					tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}
				tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp;
			}
			else if (cmp(&pta[2], &pta[3], a, length) > 0)
			{
				tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}

			if (cmp(&pta[1], &pta[2], a, length) > 0)
			{
				if (cmp(&pta[0], &pta[2], a, length) <= 0)
				{
					if (cmp(&pta[1], &pta[3], a, length) <= 0)
					{
						tmp = pta[1]; pta[1] = pta[2]; pta[2] = tmp;
					}
					else
					{
						tmp = pta[1]; pta[1] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
					}
				}
				else if (cmp(&pta[0], &pta[3], a, length) > 0)
				{
					tmp = pta[0]; pta[0] = pta[2]; pta[2] = tmp; tmp = pta[1]; pta[1] = pta[3]; pta[3] = tmp;
				}
				else if (cmp(&pta[1], &pta[3], a, length) <= 0)
				{
					tmp = pta[0]; pta[0] = pta[2]; pta[2] = pta[1]; pta[1] = tmp;
				}
				else
				{
					tmp = pta[0]; pta[0] = pta[2]; pta[2] = pta[3]; pta[3] = pta[1]; pta[1] = tmp;
				}
			}
			pte = pta - 1;

			reverse = (pte - pts) / 2;

			do
			{//将逆序的序列纠正
				tmp = *pts; *pts++ = *pte; *pte-- = tmp;
			} while (reverse--);

			if (count % 2 == 0)
			{
				pta -= 4;
				//排列八个以后,指向后四个，且后四个排好序
				parity_tail_flip_eight(pta, a, length);
			}
			else
			{
				count--;

				parity_tail_swap_eight(pta, a, length);//调用函数排序 前四个排好，后四个还未
			}
			pta += 8;

			continue;
		}

		if (pts == array)
		{//没有逆序
			switch (nmemb % 8)
			{
			case 7: if (cmp(&pta[5], &pta[6], a, length) <= 0) break;
			case 6: if (cmp(&pta[4], &pta[5], a, length) <= 0) break;
			case 5: if (cmp(&pta[3], &pta[4], a, length) <= 0) break;
			case 4: if (cmp(&pta[2], &pta[3], a, length) <= 0) break;
			case 3: if (cmp(&pta[1], &pta[2], a, length) <= 0) break;
			case 2: if (cmp(&pta[0], &pta[1], a, length) <= 0) break;
			case 1: if (cmp(&pta[-1], &pta[0], a, length) <= 0) break;
			case 0:
				pte = pts + nmemb - 1;

				reverse = (pte - pts) / 2;

				do
				{
					tmp = *pts; *pts++ = *pte; *pte-- = tmp;
				} while (reverse--);

				return 1;
			}
		}
		pte = pta - 1;

		reverse = (pte - pts) / 2;

		do
		{
			tmp = *pts; *pts++ = *pte; *pte-- = tmp;
		} while (reverse--);

		break;
	}

	tail_swap(pta, nmemb % 8, a, length);

	pta = array;

	count = nmemb / 32;

	for (count = nmemb / 32; count--; pta += 32)
	{
		if (cmp(pta + 7, pta + 8, a, length) <= 0 && cmp(pta + 15, pta + 16, a, length) <= 0 && cmp(pta + 23, pta + 24, a, length) <= 0)
		{
			continue;
		}
		parity_merge(swap, pta, 8, 16, a, length);//排序两组块长8，总16的数
		parity_merge(swap + 16, pta + 16, 8, 16, a, length);
		parity_merge(pta, swap, 16, 32, a, length);
	}

	if (nmemb % 32 > 8)
	{
		tail_merge(pta, swap, 32, nmemb % 32, 8, a, length);//多出的
	}
	return 0;
}

// quad merge support routines

void forward_merge(struct coo_t *dest, struct coo_t *from, size_t block, int* a, int length)
{//合并两个块，前后关系比较明显
	struct coo_t *ptl, *ptr, *m, *e; // left, right, middle, end

	ptl = from;
	ptr = from + block;
	m = ptr;
	e = ptr + block;

	if (cmp(m - 1, e - block / 4, a, length) <= 0)
	{
		//前面的尾部比后面一组小，大约等于前一组比较后一组小
		do
		{
			if (cmp(ptl, ptr, a, length) <= 0)
			{//从头排序
				*dest++ = *ptl++;
				continue;
			}
			*dest++ = *ptr++;//后面块的有比前面的小的
			if (cmp(ptl, ptr, a, length) <= 0)
			{
				*dest++ = *ptl++;
				continue;
			}
			*dest++ = *ptr++;
			if (cmp(ptl, ptr, a, length) <= 0)
			{
				*dest++ = *ptl++;
				continue;
			}
			*dest++ = *ptr++;
		} while (ptl < m);

		do *dest++ = *ptr++; while (ptr < e);
	}
	else if (cmp(m - block / 4, e - 1, a, length) > 0)
	{//前面的尾部倒数二比后面一组大，大约等于前一组比较后一组大
		do
		{
			if (cmp(ptl, ptr, a, length) > 0)
			{
				*dest++ = *ptr++;
				continue;
			}
			*dest++ = *ptl++;
			if (cmp(ptl, ptr, a, length) > 0)
			{
				*dest++ = *ptr++;
				continue;
			}
			*dest++ = *ptl++;
			if (cmp(ptl, ptr, a, length) > 0)
			{
				*dest++ = *ptr++;
				continue;
			}
			*dest++ = *ptl++;
		} while (ptr < e);

		do *dest++ = *ptl++; while (ptl < m);
	}
	else
	{//尾部比较复杂的关系,就用块排
		parity_merge(dest, from, block, block * 2, a, length);
	}
}

// main memory: [A][B][C][D]
// swap memory: [A  B]       step 1
// swap memory: [A  B][C  D] step 2
// main memory: [A  B  C  D] step 3

void quad_merge_block(struct coo_t *array, struct coo_t *swap, size_t block, int* a, int length)//对四个块排序
{
	register struct coo_t *pts, *c, *c_max;
	size_t block_x_2 = block * 2;

	c_max = array + block;

	if (cmp(c_max - 1, c_max, a, length) <= 0)//A的末尾和B的首位比较
	{
		c_max += block_x_2;

		if (cmp(c_max - 1, c_max, a, length) <= 0)//C的末尾和D的首位比较
		{
			c_max -= block;//c的首位

			if (cmp(c_max - 1, c_max, a, length) <= 0)
			{//理想情况，全体有序
				return;
			}
			pts = swap;

			c = array;

			do *pts++ = *c++; while (c < c_max); // step 1

			c_max = c + block_x_2;

			do *pts++ = *c++; while (c < c_max); // step 2

			return forward_merge(array, swap, block_x_2, a, length); // step 3
		}
		//c d 不是基本有序
		pts = swap;

		c = array;
		c_max = array + block_x_2;

		do *pts++ = *c++; while (c < c_max); // step 1
	}
	else
	{
		forward_merge(swap, array, block, a, length); // step 1
	}
	forward_merge(swap + block_x_2, array + block_x_2, block, a, length); // step 2，对不是有序c，d排序

	forward_merge(array, swap, block_x_2, a, length); // step 3
}

void quad_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, int* a, int length)
{
	register struct coo_t *pta, *pte;

	pte = array + nmemb;

	block *= 4;

	while (block <= nmemb && block <= swap_size)
	{//循环排列四块
		pta = array;

		do
		{
			quad_merge_block(pta, swap, block / 4, a, length);

			pta += block;
		} while (pta + block <= pte);

		tail_merge(pta, swap, swap_size, pte - pta, block / 4, a, length);//排block四倍之后多余的

		block *= 4;
	}

	tail_merge(array, swap, swap_size, nmemb, block / 4, a, length);//全体排序
}

void partial_forward_merge(struct coo_t *array, struct coo_t *swap, size_t nmemb, size_t block, int* a, int length)
{//已经排序好的相邻两块排序
	struct coo_t *r, *m, *e, *s; // right, middle, end, swap

	r = array + block;//右边一块的起始位
	e = array + nmemb - 1;

	memcpy(swap, array, block * sizeof(struct coo_t));

	s = swap;
	m = swap + block - 1;//第一块末尾

	if (cmp(m, e, a, length) <= 0)
	{
		do
		{//比较两块，并起来
			while (cmp(s, r, a, length) > 0)
			{
				*array++ = *r++;
			}
			*array++ = *s++;
		} while (s <= m);
	}
	else
	{
		do
		{
			if (cmp(s, r, a, length) > 0)
			{
				*array++ = *r++;
				continue;
			}
			*array++ = *s++;

			if (cmp(s, r, a, length) > 0)
			{
				*array++ = *r++;
				continue;
			}
			*array++ = *s++;

			if (cmp(s, r, a, length) > 0)
			{
				*array++ = *r++;
				continue;
			}
			*array++ = *s++;
		} while (r <= e);

		do *array++ = *s++; while (s <= m);
	}
}

void partial_backward_merge(struct coo_t *array, struct coo_t *swap, size_t nmemb, size_t block, int* a, int length)
{//从后向前排，可以考虑不足情况
	struct coo_t *r, *m, *e, *s; // right, middle, end, swap

	m = array + block;
	e = array + nmemb - 1;
	r = m--;//右边起始，m变成右边末尾

	if (cmp(m, r, a, length) <= 0)
	{
		return;
	}

	while (cmp(m, e, a, length) <= 0)
	{
		e--;//找到右边比左边小的
	}

	s = swap;

	do *s++ = *r++; while (r <= e);

	s--;

	*e-- = *m--;//右边用左边末尾等长填充
	//剩余排序好
	if (cmp(array, swap, a, length) <= 0)
	{
		do
		{
			while (cmp(m, s, a, length) > 0)
			{
				*e-- = *m--;
			}
			*e-- = *s--;
		} while (s >= swap);
	}
	else
	{
		do
		{
			if (cmp(m, s, a, length) > 0)
			{
				*e-- = *m--;
				continue;
			}
			*e-- = *s--;
			if (cmp(m, s, a, length) > 0)
			{
				*e-- = *m--;
				continue;
			}
			*e-- = *s--;
			if (cmp(m, s, a, length) > 0)
			{
				*e-- = *m--;
				continue;
			}
			*e-- = *s--;
		} while (m >= array);

		do *e-- = *s--; while (s >= swap);
	}
}

void tail_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, int* a, int length)
{
	register struct coo_t *pta, *pte;

	pte = array + nmemb;

	while (block < nmemb && block <= swap_size)
	{//两个块的从后排序
		pta = array;

		for (pta = array; pta + block < pte; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				partial_backward_merge(pta, swap, block * 2, block, a, length);

				continue;
			}
			partial_backward_merge(pta, swap, pte - pta, block, a, length);

			break;
		}
		block *= 2;
	}
}

// the next four functions provide in-place rotate merge support
//下面四个函数提供就地旋转合并支持
void trinity_rotation(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t left)
{ //将left反转到末尾
	size_t bridge, right = nmemb - left;

	if (left < right)
	{
		if (left <= swap_size)
		{//把left先存在swap
			memcpy(swap, array, left * sizeof(struct coo_t));
			memmove(array, array + left, right * sizeof(struct coo_t));//把右边的剩余往前挪
			memcpy(array + right, swap, left * sizeof(struct coo_t));//left放后面
		}
		else
		{
			struct coo_t *pta, *ptb, *ptc, *ptd;

			pta = array;
			ptb = pta + left;

			bridge = right - left;//往前挪left

			if (bridge <= swap_size && bridge > 3)
			{
				ptc = pta + right;//前right和后面隔开left位
				ptd = ptc + left;//末尾

				memcpy(swap, ptb, bridge * sizeof(struct coo_t));//ptb到ptc的放入swap

				while (left--)
				{
					*--ptc = *--ptd; *ptd = *--ptb;//*ptd = *--ptb;将bridge后的前挪。
				}
				memcpy(pta, swap, bridge * sizeof(struct coo_t));
			}
			else
			{
				ptc = ptb;
				ptd = ptc + right;

				bridge = left / 2;

				while (bridge--)
				{
					*swap = *--ptb; *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = *swap;//Conjoined Triple Reversal Rotation图在totate
				}

				bridge = (ptd - ptc) / 2;

				while (bridge--)
				{
					*swap = *ptc; *ptc++ = *--ptd; *ptd = *pta; *pta++ = *swap;
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					*swap = *pta; *pta++ = *--ptd; *ptd = *swap;
				}
			}
		}
	}
	else if (right < left)
	{
		if (right <= swap_size)
		{
			memcpy(swap, array + left, right * sizeof(struct coo_t));
			memmove(array + right, array, left * sizeof(struct coo_t));
			memcpy(array, swap, right * sizeof(struct coo_t));
		}
		else
		{
			struct coo_t *pta, *ptb, *ptc, *ptd;

			pta = array;
			ptb = pta + left;

			bridge = left - right;

			if (bridge <= swap_size && bridge > 3)
			{
				ptc = pta + right;
				ptd = ptc + left;

				memcpy(swap, ptc, bridge * sizeof(struct coo_t));

				while (right--)
				{
					*ptc++ = *pta; *pta++ = *ptb++;//Bridge Rotation
				}
				memcpy(ptd - bridge, swap, bridge * sizeof(struct coo_t));
			}
			else
			{
				ptc = ptb;
				ptd = ptc + right;

				bridge = right / 2;

				while (bridge--)
				{
					*swap = *--ptb; *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = *swap;
				}

				bridge = (ptb - pta) / 2;

				while (bridge--)
				{
					*swap = *--ptb; *ptb = *pta; *pta++ = *--ptd; *ptd = *swap;
				}

				bridge = (ptd - pta) / 2;

				while (bridge--)
				{
					*swap = *pta; *pta++ = *--ptd; *ptd = *swap;//收尾
				}
			}
		}
	}
	else
	{
		struct coo_t *pta, *ptb;

		pta = array;
		ptb = pta + left;

		while (left--)
		{
			*swap = *pta; *pta++ = *ptb; *ptb++ = *swap;
		}
	}
}

size_t monobound_binary_first(struct coo_t *array, struct coo_t *value, size_t top, int* a, int length)
{//二分查找位置，找到右边的等于或者小于前面的位置
	struct coo_t *end;
	size_t mid;

	end = array + top;

	while (top > 1)
	{
		mid = top / 2;

		if (cmp(value, end - mid, a, length) <= 0)
		{
			end -= mid;
		}
		top -= mid;
	}

	if (cmp(value, end - 1, a, length) <= 0)
	{
		end--;
	}
	return (end - array);
}

void blit_merge_block(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t block, size_t right, int* a, int length)
{
	size_t left;

	if (cmp(array + block - 1, array + block, a, length) <= 0)
	{
		return;
	}

	left = monobound_binary_first(array + block, array + block / 2, right, a, length);//查找右边比左边中间小的位置

	right -= left;

	block /= 2;

	if (left)
	{//右边有比左边中间小的
		trinity_rotation(array + block, swap, swap_size, block + left, block);//旋转两块，分割线是第一块末尾

		if (left <= swap_size)
		{
			partial_backward_merge(array, swap, block + left, block, a, length);
		}
		else if (block <= swap_size)
		{
			partial_forward_merge(array, swap, block + left, block, a, length);
		}
		else
		{
			blit_merge_block(array, swap, swap_size, block, left, a, length);
		}
	}

	if (right)
	{
		if (right <= swap_size)
		{
			partial_backward_merge(array + block + left, swap, block + right, block, a, length);
		}
		else if (block <= swap_size)
		{
			partial_forward_merge(array + block + left, swap, block + right, block, a, length);
		}
		else
		{
			blit_merge_block(array + block + left, swap, swap_size, block, right, a, length);
		}
	}
}

void blit_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, int* a, int length)
{//排序两个块
	struct coo_t *pta, *pte;

	pte = array + nmemb;

	while (block < nmemb)
	{
		pta = array;

		for (pta = array; pta + block < pte; pta += block * 2)
		{
			if (pta + block * 2 < pte)
			{
				blit_merge_block(pta, swap, swap_size, block, block, a, length);

				continue;
			}
			blit_merge_block(pta, swap, swap_size, block, pte - pta - block, a, length);

			break;
		}
		block *= 2;
	}
}

void blitsort(struct coo_t *array, size_t nmemb, int* a, int length)
{
	if (nmemb < 32)
	{
		tail_swap(array, nmemb, a, length);
	}
	else if (quad_swap(array, nmemb, a, length) == 0)
	{
#if BLITCACHE
		size_t swap_size = BLITCACHE;
#else
		size_t swap_size = 32;

		while (swap_size * swap_size <= nmemb)
		{
			swap_size *= 4;
		}
#endif
		struct coo_t *swap = new struct coo_t[swap_size];

		quad_merge(array, swap, swap_size, nmemb, 32, a, length);

		blit_merge(array, swap, swap_size, nmemb, swap_size * 2, a, length);

		delete[] swap;
		swap = NULL;
	}
}
/*
	void blitsort_swap(void *array, void *swap, size_t nmemb, size_t swap_size, cmp)
	{
		if (nmemb < 32)
		{
			tail_swap(array, nmemb, cmp);//整体小于32个数
		}
		else if (quad_swap(array, nmemb, cmp) == 0)//非8整数倍
		{
			quad_merge(array, swap, swap_size, nmemb, 32, cmp);//一次排32

			blit_merge(array, swap, swap_size, nmemb, swap_size * 2, cmp);//排剩下的
		}
	}
	*/

