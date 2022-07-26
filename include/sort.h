
#define _SORT_H_
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
/*/ home / xwb / taco / transpose / taco / apps / tensor_4_fusion
'4a037ec7b4c80987320379feba850bc31de8c0b9949ef1045551c4866fe3c1c6'
*/
typedef int CMPFUNC (const void *a, const void *b);
	
	struct coo_t
	{
		int32_t	idx0;
		int32_t	idx1;
		int32_t	idx2;
		int32_t	idx3;	
		int32_t	idx4;
		double	val;
	};
	

	/*
		blitsort 1.1.5.1
	*/

void G_qsort(struct coo_t *a, int first, int mid, int last, struct coo_t *temp, CMPFUNC *cmp)
{
	int n = mid, m = last;
	int k = 0;
	int i = first, j = mid + 1;
	while (i <= n && j <= m)  // ¿¿¿¿¿¿
	{
		if (cmp(a+i,a+j))  //  i¿j¿¿¿¿
		{
			temp[k++] = a[j++];  // ¿¿i <= j,¿i¿¿¿¿¿temp¿¿
		}
		else
		{
			temp[k++] = a[i++];   // ¿¿i > j,¿j¿¿¿¿¿temp¿¿
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

void G_sort(struct coo_t*a, int first, int last, struct coo_t*temp, CMPFUNC *cmp)
{
	if (first < last)  //  ¿¿¿¿¿¿¿¿¿¿¿¿¿
	{
		int mid = (first + last) / 2;  // ¿¿¿¿¿
		G_sort(a, first, mid, temp,cmp);  //  ¿¿¿¿¿¿
		G_sort(a, mid + 1, last, temp,cmp);   // ¿¿
		G_qsort(a, first, mid, last, temp,cmp);  // ¿¿¿¿
	}
}

void sort(struct coo_t*a, int n,CMPFUNC *cmp)
{
	struct coo_t* p = (struct coo_t *)malloc(sizeof(struct coo_t) * n);
	G_sort(a, 0, n - 1, p,cmp);  // ¿¿¿¿¿¿¿0¿n-1¿¿¿¿¿¿¿¿¿¿¿¿
	free(p);
	
}
	// the next six functions are used for sorting 0 to 31 elements
void parity_merge_two(struct coo_t *array, struct coo_t *swap,int x,int y,struct coo_t * ptl,struct coo_t * ptr, struct coo_t *pts, CMPFUNC *cmp)  
{  //±È½ÏËÄ¸öÊý£¬Á½¸öÁ½¸öÒ»×é½øÐÐÇ°ºó±È½Ï£¬½«Á½×é¹²ËÄ¸öÊý
	ptl = array + 0; ptr = array + 2; pts = swap + 0;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;  
	*pts = cmp(ptl, ptr) <= 0 ? *ptl : *ptr;  
  
	ptl = array + 1; ptr = array + 3; pts = swap + 3;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;  
	*pts = cmp(ptl, ptr)  > 0 ? *ptl : *ptr;  
}

void parity_merge_four(struct coo_t *array, struct coo_t *swap, int x, int y, struct coo_t * ptl, struct coo_t * ptr, struct coo_t *pts, CMPFUNC *cmp)
{  //±È½Ï°Ë¸öÊý£¬ÔÚÇ°ºó¸÷ËÄ¸öÁ½×é¼äÊ¹ÓÃËÄ¸öÊýµÄ±È½Ï·½·¨
	ptl = array + 0; ptr = array + 4; pts = swap;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts[x] = *ptr; ptr += y; pts[y] = *ptl; ptl += x; pts++;  
	*pts = cmp(ptl, ptr) <= 0 ? *ptl : *ptr;  
  
	ptl = array + 3; ptr = array + 7; pts = swap + 7;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;  
	x = cmp(ptl, ptr) <= 0; y = !x; pts--; pts[x] = *ptr; ptr -= x; pts[y] = *ptl; ptl -= y;  
	*pts = cmp(ptl, ptr)  > 0 ? *ptl : *ptr;  
}
	void unguarded_insert(struct coo_t *array, size_t offset, size_t nmemb, CMPFUNC *cmp)
	{//¸øÊý×é´ÓÐ¡µ½´óÅÅÐò£¬--ptaÊÇendµÄÇ°Ò»Ïî£¬ÅÅÐò½ØÖÁnmembµÄÊý

		struct coo_t key, *pta, *end;//sitruct coo_t char
		size_t i, top;

		for (i = offset; i < nmemb; i++)
		{
			pta = end = array + i;

			if (cmp(--pta, end) <= 0)
			{
				continue;
			}

			key = *end;

			if (cmp(array, &key) > 0)
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
				} while (cmp(pta, &key) > 0);

				*end = key;
			}
		}
	}

	void parity_swap_four(struct coo_t *array, CMPFUNC *cmp)
	{//½«ËÄ¸öÊý·Ö³ÉÁ½×é£¬Ç°Á½¸öÓÐÐò·Åµ½swapÊý×é£¬ºóÁ½¸öÓÐÐò·Åµ½swapÊý×é£¬ÔÚÓÃ¹é²¢ÅÅÐòÇ°ºóÁ½×éÊý¾Ý
		struct coo_t swap[4], *ptl, *ptr, *pts;
		unsigned char x, y;

		x = cmp(array + 0, array + 1) <= 0; y = !x; swap[0 + y] = array[0]; swap[0 + x] = array[1];
		x = cmp(array + 2, array + 3) <= 0; y = !x; swap[2 + y] = array[2]; swap[2 + x] = array[3];

		parity_merge_two(swap, array, x, y, ptl, ptr, pts, cmp);

	}

	void parity_swap_eight(struct coo_t *array, CMPFUNC *cmp)
	{//ÅÅÐò8¸öÊý
		struct coo_t swap[8], *ptl, *ptr, *pts;
		unsigned char x, y;

		if (cmp(array + 0, array + 1) > 0) { swap[0] = array[0]; array[0] = array[1]; array[1] = swap[0]; }
		if (cmp(array + 2, array + 3) > 0) { swap[0] = array[2]; array[2] = array[3]; array[3] = swap[0]; }
		if (cmp(array + 4, array + 5) > 0) { swap[0] = array[4]; array[4] = array[5]; array[5] = swap[0]; }
		if (cmp(array + 6, array + 7) > 0) { swap[0] = array[6]; array[6] = array[7]; array[7] = swap[0]; }
		else if (cmp(array + 1, array + 2) <= 0 && cmp(array + 3, array + 4) <= 0 && cmp(array + 5, array + 6) <= 0)
		{
			return;
		}
		parity_merge_two(array + 0, swap + 0, x, y, ptl, ptr, pts, cmp);
		parity_merge_two(array + 4, swap + 4, x, y, ptl, ptr, pts, cmp);

		parity_merge_four(swap, array, x, y, ptl, ptr, pts, cmp);
	}

	void parity_merge(struct coo_t *dest, struct coo_t *from, size_t block, size_t nmemb, CMPFUNC *cmp)
	{//Ä¿±êÊý×é ½»»»ºóÊý×é ¿é³¤ ¿éÎ²  ±È½Ï
		//ÅÅÐòÁ½×é¿é
		struct coo_t *ptl, *ptr, *tpl, *tpr, *tpd, *ptd;
		unsigned char x, y;

		ptl = from;
		ptr = from + block;
		ptd = dest;
		tpl = from + block - 1;
		tpr = from + nmemb - 1;
		tpd = dest + nmemb - 1;

		for (block--; block; block--)
		{//Ç°ÏòºóÏòÒ»ÆðÅÅ£¬Ã¿´ÎÑ­»·ÅÅÐòÁ½¸ö
			x = cmp(ptl, ptr) <= 0; y = !x; ptd[x] = *ptr; ptr += y; ptd[y] = *ptl; ptl += x; ptd++;
			x = cmp(tpl, tpr) <= 0; y = !x; tpd--; tpd[x] = *tpr; tpr -= x; tpd[y] = *tpl; tpl -= y;
		}
		*ptd = cmp(ptl, ptr) <= 0 ? *ptl : *ptr;
		*tpd = cmp(tpl, tpr) > 0 ? *tpl : *tpr;
	}

	void parity_swap_sixteen(struct coo_t *array, CMPFUNC *cmp)
	{//ËÄ¸öÊýÒ»ÅÅ£¬ÅÅÐò16¸öÊý
		struct coo_t swap[16], *ptl, *ptr, *pts;
		unsigned char x, y;

		parity_swap_four(array + 0, cmp);
		parity_swap_four(array + 4, cmp);
		parity_swap_four(array + 8, cmp);
		parity_swap_four(array + 12, cmp);

		if (cmp(array + 3, array + 4) <= 0 && cmp(array + 7, array + 8) <= 0 && cmp(array + 11, array + 12) <= 0)
		{
			return;
		}
		parity_merge_four(array + 0, swap + 0, x, y, ptl, ptr, pts, cmp);
		parity_merge_four(array + 8, swap + 8, x, y, ptl, ptr, pts, cmp);

		parity_merge(array, swap, 8, 16, cmp);
	}

	void tail_swap(struct coo_t *array, size_t nmemb, CMPFUNC *cmp)
	{
		if (nmemb < 4)
		{
			unguarded_insert(array, 1, nmemb, cmp);
			return;
		}
		if (nmemb < 8)
		{
			parity_swap_four(array, cmp);
			unguarded_insert(array, 4, nmemb, cmp);
			return;
		}
		if (nmemb < 16)
		{
			parity_swap_eight(array, cmp);
			unguarded_insert(array, 8, nmemb, cmp);
			return;
		}
		parity_swap_sixteen(array, cmp);
		unguarded_insert(array, 16, nmemb, cmp);
	}

	// the next three functions create sorted blocks of 32 elements½ÓÏÂÀ´µÄÈý¸öº¯Êý´´½¨32¸öÔªËØµÄÅÅÐò¿é

	void parity_tail_swap_eight(struct coo_t *array, CMPFUNC *cmp)
	{//Ç°ËÄ¸öÅÅÐòºÃ£¬ºóËÄ¸ö²»Çå³þ
		struct coo_t swap[8], *ptl, *ptr, *pts;
		unsigned char x, y;

		if (cmp(array + 4, array + 5) > 0) { swap[5] = array[4]; array[4] = array[5]; array[5] = swap[5]; }
		if (cmp(array + 6, array + 7) > 0) { swap[7] = array[6]; array[6] = array[7]; array[7] = swap[7]; }
		else

			if (cmp(array + 3, array + 4) <= 0 && cmp(array + 5, array + 6) <= 0)
			{
				return;
			}
		swap[0] = array[0]; swap[1] = array[1]; swap[2] = array[2]; swap[3] = array[3];

		parity_merge_two(array + 4, swap + 4, x, y, ptl, ptr, pts, cmp);

		parity_merge_four(swap, array, x, y, ptl, ptr, pts, cmp);
	}

	void parity_tail_flip_eight(struct coo_t *array, CMPFUNC *cmp)
	{//Ç°ËÄ¸öÅÅºÃÐò£¬ºóËÄ¸öÅÅºÃÐò£¬Ç°ºó¹ØÏµ²»Çå³þ
		struct coo_t swap[8], *ptl, *ptr, *pts;
		unsigned char x, y;

		if (cmp(array + 3, array + 4) <= 0)
		{
			return;
		}
		swap[0] = array[0]; swap[1] = array[1]; swap[2] = array[2]; swap[3] = array[3];
		swap[4] = array[4]; swap[5] = array[5]; swap[6] = array[6]; swap[7] = array[7];

		parity_merge_four(swap, array, x, y, ptl, ptr, pts, cmp);
	}
	void tail_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, CMPFUNC *cmp);

	size_t quad_swap(struct coo_t *array, size_t nmemb, CMPFUNC *cmp)
	{//Ã¿°Ë¸öÊýÅÅºÃÐò£¬ÅÅ32¸ö£¬È·¶¨ÊÇ·ñÓÐË³ÐòºÍÄæÐò£¬¼õÉÙ¿ªÏú
		struct coo_t swap[32];
		size_t count, reverse;
		struct coo_t *pta, *pts, *pte, tmp;

		pta = array;

		count = nmemb / 8 * 2;

		while (count--)
		{
			if (cmp(&pta[0], &pta[1]) > 0)//Ç°Á½¸öÄæÐò
			{
				if (cmp(&pta[2], &pta[3]) > 0)
				{
					if (cmp(&pta[1], &pta[2]) > 0)
					{//ÄæÐò
						pts = pta;
						pta += 4;
						goto swapper;
					}
					tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}
				tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp;
			}
			else if (cmp(&pta[2], &pta[3]) > 0)//Ç°Á½¸öÓÐÐò£¬ºóÁ½¸öÎÞÐò
			{
				tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
			}
			//ÅÅÐòËÄ¸öÊý
			if (cmp(&pta[1], &pta[2]) > 0)
			{
				if (cmp(&pta[0], &pta[2]) <= 0)
				{
					if (cmp(&pta[1], &pta[3]) <= 0)
					{
						tmp = pta[1]; pta[1] = pta[2]; pta[2] = tmp;
					}
					else
					{
						tmp = pta[1]; pta[1] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
					}
				}
				else if (cmp(&pta[0], &pta[3]) > 0)
				{
					tmp = pta[1]; pta[1] = pta[3]; pta[3] = tmp; tmp = pta[0]; pta[0] = pta[2]; pta[2] = tmp;
				}
				else if (cmp(&pta[1], &pta[3]) <= 0)
				{
					tmp = pta[1]; pta[1] = pta[0]; pta[0] = pta[2]; pta[2] = tmp;
				}
				else
				{
					tmp = pta[1]; pta[1] = pta[0]; pta[0] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}
			}
			count--;

			parity_tail_swap_eight(pta, cmp);//ÅÅÐòºÃÇ°ËÄ¸ö£¬ÅÅÐòÕû¸ö°Ë¸öÊý

			pta += 8;

			continue;

		swapper:
			//½«ÅÅÐòºÃµÄÇ°Á½¸öÊýºÍÅÅÐòºÃµÄºóÁ½¸öÊýµÄ´óÐ¡Çé¿öÁÐ¾Ù³öÀ´£¬½øÐÐÅÅÐò
			if (count--)
			{
				if (cmp(&pta[0], &pta[1]) > 0)
				{
					if (cmp(&pta[2], &pta[3]) > 0)
					{
						if (cmp(&pta[1], &pta[2]) > 0)
						{
							if (cmp(&pta[-1], &pta[0]) > 0)
							{//ÕûÌåÇ°Ïò¶¼ÄæÐò
								pta += 4;

								goto swapper;
							}
						}
						tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
					}
					tmp = pta[0]; pta[0] = pta[1]; pta[1] = tmp;
				}
				else if (cmp(&pta[2], &pta[3]) > 0)
				{
					tmp = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
				}

				if (cmp(&pta[1], &pta[2]) > 0)
				{
					if (cmp(&pta[0], &pta[2]) <= 0)
					{
						if (cmp(&pta[1], &pta[3]) <= 0)
						{
							tmp = pta[1]; pta[1] = pta[2]; pta[2] = tmp;
						}
						else
						{
							tmp = pta[1]; pta[1] = pta[2]; pta[2] = pta[3]; pta[3] = tmp;
						}
					}
					else if (cmp(&pta[0], &pta[3]) > 0)
					{
						tmp = pta[0]; pta[0] = pta[2]; pta[2] = tmp; tmp = pta[1]; pta[1] = pta[3]; pta[3] = tmp;
					}
					else if (cmp(&pta[1], &pta[3]) <= 0)
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
				{//½«ÄæÐòµÄÐòÁÐ¾ÀÕý
					tmp = *pts; *pts++ = *pte; *pte-- = tmp;
				} while (reverse--);

				if (count % 2 == 0)
				{
					pta -= 4;
					//ÅÅÁÐ°Ë¸öÒÔºó,Ö¸ÏòºóËÄ¸ö£¬ÇÒºóËÄ¸öÅÅºÃÐò
					parity_tail_flip_eight(pta, cmp);
				}
				else
				{
					count--;

					parity_tail_swap_eight(pta, cmp);//µ÷ÓÃº¯ÊýÅÅÐò Ç°ËÄ¸öÅÅºÃ£¬ºóËÄ¸ö»¹Î´
				}
				pta += 8;

				continue;
			}

			if (pts == array)
			{//Ã»ÓÐÄæÐò
				switch (nmemb % 8)
				{
				case 7: if (cmp(&pta[5], &pta[6]) <= 0) break;
				case 6: if (cmp(&pta[4], &pta[5]) <= 0) break;
				case 5: if (cmp(&pta[3], &pta[4]) <= 0) break;
				case 4: if (cmp(&pta[2], &pta[3]) <= 0) break;
				case 3: if (cmp(&pta[1], &pta[2]) <= 0) break;
				case 2: if (cmp(&pta[0], &pta[1]) <= 0) break;
				case 1: if (cmp(&pta[-1], &pta[0]) <= 0) break;
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

		tail_swap(pta, nmemb % 8, cmp);

		pta = array;

		count = nmemb / 32;

		for (count = nmemb / 32; count--; pta += 32)
		{
			if (cmp(pta + 7, pta + 8) <= 0 && cmp(pta + 15, pta + 16) <= 0 && cmp(pta + 23, pta + 24) <= 0)
			{
				continue;
			}
			parity_merge(swap, pta, 8, 16, cmp);//ÅÅÐòÁ½×é¿é³¤8£¬×Ü16µÄÊý
			parity_merge(swap + 16, pta + 16, 8, 16, cmp);
			parity_merge(pta, swap, 16, 32, cmp);
		}

		if (nmemb % 32 > 8)
		{
			tail_merge(pta, swap, 32, nmemb % 32, 8, cmp);//¶à³öµÄ
		}
		return 0;
	}

	// quad merge support routines

	void forward_merge(struct coo_t *dest, struct coo_t *from, size_t block, CMPFUNC *cmp)
	{//ºÏ²¢Á½¸ö¿é£¬Ç°ºó¹ØÏµ±È½ÏÃ÷ÏÔ
		struct coo_t *ptl, *ptr, *m, *e; // left, right, middle, end

		ptl = from;
		ptr = from + block;
		m = ptr;
		e = ptr + block;

		if (cmp(m - 1, e - block / 4) <= 0)
		{
			//Ç°ÃæµÄÎ²²¿±ÈºóÃæÒ»×éÐ¡£¬´óÔ¼µÈÓÚÇ°Ò»×é±È½ÏºóÒ»×éÐ¡
			do
			{
				if (cmp(ptl, ptr) <= 0)
				{//´ÓÍ·ÅÅÐò
					*dest++ = *ptl++;
					continue;
				}
				*dest++ = *ptr++;//ºóÃæ¿éµÄÓÐ±ÈÇ°ÃæµÄÐ¡µÄ
				if (cmp(ptl, ptr) <= 0)
				{
					*dest++ = *ptl++;
					continue;
				}
				*dest++ = *ptr++;
				if (cmp(ptl, ptr) <= 0)
				{
					*dest++ = *ptl++;
					continue;
				}
				*dest++ = *ptr++;
			} while (ptl < m);

			do *dest++ = *ptr++; while (ptr < e);
		}
		else if (cmp(m - block / 4, e - 1) > 0)
		{//Ç°ÃæµÄÎ²²¿µ¹Êý¶þ±ÈºóÃæÒ»×é´ó£¬´óÔ¼µÈÓÚÇ°Ò»×é±È½ÏºóÒ»×é´ó
			do
			{
				if (cmp(ptl, ptr) > 0)
				{
					*dest++ = *ptr++;
					continue;
				}
				*dest++ = *ptl++;
				if (cmp(ptl, ptr) > 0)
				{
					*dest++ = *ptr++;
					continue;
				}
				*dest++ = *ptl++;
				if (cmp(ptl, ptr) > 0)
				{
					*dest++ = *ptr++;
					continue;
				}
				*dest++ = *ptl++;
			} while (ptr < e);

			do *dest++ = *ptl++; while (ptl < m);
		}
		else
		{//Î²²¿±È½Ï¸´ÔÓµÄ¹ØÏµ,¾ÍÓÃ¿éÅÅ
			parity_merge(dest, from, block, block * 2, cmp);
		}
	}

	// main memory: [A][B][C][D]
	// swap memory: [A  B]       step 1
	// swap memory: [A  B][C  D] step 2
	// main memory: [A  B  C  D] step 3

	void quad_merge_block(struct coo_t *array, struct coo_t *swap, size_t block, CMPFUNC *cmp)//¶ÔËÄ¸ö¿éÅÅÐò
	{
		register struct coo_t *pts, *c, *c_max;
		size_t block_x_2 = block * 2;

		c_max = array + block;

		if (cmp(c_max - 1, c_max) <= 0)//AµÄÄ©Î²ºÍBµÄÊ×Î»±È½Ï
		{
			c_max += block_x_2;

			if (cmp(c_max - 1, c_max) <= 0)//CµÄÄ©Î²ºÍDµÄÊ×Î»±È½Ï
			{
				c_max -= block;//cµÄÊ×Î»

				if (cmp(c_max - 1, c_max) <= 0)
				{//ÀíÏëÇé¿ö£¬È«ÌåÓÐÐò
					return;
				}
				pts = swap;

				c = array;

				do *pts++ = *c++; while (c < c_max); // step 1

				c_max = c + block_x_2;

				do *pts++ = *c++; while (c < c_max); // step 2

				return forward_merge(array, swap, block_x_2, cmp); // step 3
			}
			//c d ²»ÊÇ»ù±¾ÓÐÐò
			pts = swap;

			c = array;
			c_max = array + block_x_2;

			do *pts++ = *c++; while (c < c_max); // step 1
		}
		else
		{
			forward_merge(swap, array, block, cmp); // step 1
		}
		forward_merge(swap + block_x_2, array + block_x_2, block, cmp); // step 2£¬¶Ô²»ÊÇÓÐÐòc£¬dÅÅÐò

		forward_merge(array, swap, block_x_2, cmp); // step 3
	}

	void quad_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, CMPFUNC *cmp)
	{
		register struct coo_t *pta, *pte;

		pte = array + nmemb;

		block *= 4;

		while (block <= nmemb && block <= swap_size)
		{//Ñ­»·ÅÅÁÐËÄ¿é
			pta = array;

			do
			{
				quad_merge_block(pta, swap, block / 4, cmp);

				pta += block;
			} while (pta + block <= pte);

			tail_merge(pta, swap, swap_size, pte - pta, block / 4, cmp);//ÅÅblockËÄ±¶Ö®ºó¶àÓàµÄ

			block *= 4;
		}

		tail_merge(array, swap, swap_size, nmemb, block / 4, cmp);//È«ÌåÅÅÐò
	}

	void partial_forward_merge(struct coo_t *array, struct coo_t *swap, size_t nmemb, size_t block, CMPFUNC *cmp)
	{//ÒÑ¾­ÅÅÐòºÃµÄÏàÁÚÁ½¿éÅÅÐò
		struct coo_t *r, *m, *e, *s; // right, middle, end, swap

		r = array + block;//ÓÒ±ßÒ»¿éµÄÆðÊ¼Î»
		e = array + nmemb - 1;

		memcpy(swap, array, block * sizeof(struct coo_t));

		s = swap;
		m = swap + block - 1;//µÚÒ»¿éÄ©Î²

		if (cmp(m, e) <= 0)
		{
			do
			{//±È½ÏÁ½¿é£¬²¢ÆðÀ´
				while (cmp(s, r) > 0)
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
				if (cmp(s, r) > 0)
				{
					*array++ = *r++;
					continue;
				}
				*array++ = *s++;

				if (cmp(s, r) > 0)
				{
					*array++ = *r++;
					continue;
				}
				*array++ = *s++;

				if (cmp(s, r) > 0)
				{
					*array++ = *r++;
					continue;
				}
				*array++ = *s++;
			} while (r <= e);

			do *array++ = *s++; while (s <= m);
		}
	}

	void partial_backward_merge(struct coo_t *array, struct coo_t *swap, size_t nmemb, size_t block, CMPFUNC *cmp)
	{//´ÓºóÏòÇ°ÅÅ£¬¿ÉÒÔ¿¼ÂÇ²»×ãÇé¿ö
		struct coo_t *r, *m, *e, *s; // right, middle, end, swap

		m = array + block;
		e = array + nmemb - 1;
		r = m--;//ÓÒ±ßÆðÊ¼£¬m±ä³ÉÓÒ±ßÄ©Î²

		if (cmp(m, r) <= 0)
		{
			return;
		}

		while (cmp(m, e) <= 0)
		{
			e--;//ÕÒµ½ÓÒ±ß±È×ó±ßÐ¡µÄ
		}

		s = swap;

		do *s++ = *r++; while (r <= e);

		s--;

		*e-- = *m--;//ÓÒ±ßÓÃ×ó±ßÄ©Î²µÈ³¤Ìî³ä
		//Ê£ÓàÅÅÐòºÃ
		if (cmp(array, swap) <= 0)
		{
			do
			{
				while (cmp(m, s) > 0)
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
				if (cmp(m, s) > 0)
				{
					*e-- = *m--;
					continue;
				}
				*e-- = *s--;
				if (cmp(m, s) > 0)
				{
					*e-- = *m--;
					continue;
				}
				*e-- = *s--;
				if (cmp(m, s) > 0)
				{
					*e-- = *m--;
					continue;
				}
				*e-- = *s--;
			} while (m >= array);

			do *e-- = *s--; while (s >= swap);
		}
	}

	void tail_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, CMPFUNC *cmp)
	{
		register struct coo_t *pta, *pte;

		pte = array + nmemb;

		while (block < nmemb && block <= swap_size)
		{//Á½¸ö¿éµÄ´ÓºóÅÅÐò
			pta = array;

			for (pta = array; pta + block < pte; pta += block * 2)
			{
				if (pta + block * 2 < pte)
				{
					partial_backward_merge(pta, swap, block * 2, block, cmp);

					continue;
				}
				partial_backward_merge(pta, swap, pte - pta, block, cmp);

				break;
			}
			block *= 2;
		}
	}

	// the next four functions provide in-place rotate merge support
	//ÏÂÃæËÄ¸öº¯ÊýÌá¹©¾ÍµØÐý×ªºÏ²¢Ö§³Ö
	void trinity_rotation(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t left)
	{ //½«left·´×ªµ½Ä©Î²
		size_t bridge, right = nmemb - left;

		if (left < right)
		{
			if (left <= swap_size)
			{//°ÑleftÏÈ´æÔÚswap
				memcpy(swap, array, left * sizeof(struct coo_t));
				memmove(array, array + left, right * sizeof(struct coo_t));//°ÑÓÒ±ßµÄÊ£ÓàÍùÇ°Å²
				memcpy(array + right, swap, left * sizeof(struct coo_t));//left·ÅºóÃæ
			}
			else
			{
				struct coo_t *pta, *ptb, *ptc, *ptd;

				pta = array;
				ptb = pta + left;

				bridge = right - left;//ÍùÇ°Å²left

				if (bridge <= swap_size && bridge > 3)
				{
					ptc = pta + right;//Ç°rightºÍºóÃæ¸ô¿ªleftÎ»
					ptd = ptc + left;//Ä©Î²

					memcpy(swap, ptb, bridge * sizeof(struct coo_t));//ptbµ½ptcµÄ·ÅÈëswap

					while (left--)
					{
						*--ptc = *--ptd; *ptd = *--ptb;//*ptd = *--ptb;½«bridgeºóµÄÇ°Å²¡£
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
						*swap = *--ptb; *ptb = *pta; *pta++ = *ptc; *ptc++ = *--ptd; *ptd = *swap;//Conjoined Triple Reversal RotationÍ¼ÔÚtotate
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
						*swap = *pta; *pta++ = *--ptd; *ptd = *swap;//ÊÕÎ²
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

	size_t monobound_binary_first(struct coo_t *array, struct coo_t *value, size_t top, CMPFUNC *cmp)
	{//¶þ·Ö²éÕÒÎ»ÖÃ£¬ÕÒµ½ÓÒ±ßµÄµÈÓÚ»òÕßÐ¡ÓÚÇ°ÃæµÄÎ»ÖÃ
		struct coo_t *end;
		size_t mid;

		end = array + top;

		while (top > 1)
		{
			mid = top / 2;

			if (cmp(value, end - mid) <= 0)
			{
				end -= mid;
			}
			top -= mid;
		}

		if (cmp(value, end - 1) <= 0)
		{
			end--;
		}
		return (end - array);
	}

	void blit_merge_block(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t block, size_t right, CMPFUNC *cmp)
	{
		size_t left;

		if (cmp(array + block - 1, array + block) <= 0)
		{
			return;
		}

		left = monobound_binary_first(array + block, array + block / 2, right, cmp);//²éÕÒÓÒ±ß±È×ó±ßÖÐ¼äÐ¡µÄÎ»ÖÃ

		right -= left;

		block /= 2;

		if (left)
		{//ÓÒ±ßÓÐ±È×ó±ßÖÐ¼äÐ¡µÄ
			trinity_rotation(array + block, swap, swap_size, block + left, block);//Ðý×ªÁ½¿é£¬·Ö¸îÏßÊÇµÚÒ»¿éÄ©Î²

			if (left <= swap_size)
			{
				partial_backward_merge(array, swap, block + left, block, cmp);
			}
			else if (block <= swap_size)
			{
				partial_forward_merge(array, swap, block + left, block, cmp);
			}
			else
			{
				blit_merge_block(array, swap, swap_size, block, left, cmp);
			}
		}

		if (right)
		{
			if (right <= swap_size)
			{
				partial_backward_merge(array + block + left, swap, block + right, block, cmp);
			}
			else if (block <= swap_size)
			{
				partial_forward_merge(array + block + left, swap, block + right, block, cmp);
			}
			else
			{
				blit_merge_block(array + block + left, swap, swap_size, block, right, cmp);
			}
		}
	}

	void blit_merge(struct coo_t *array, struct coo_t *swap, size_t swap_size, size_t nmemb, size_t block, CMPFUNC *cmp)
	{//ÅÅÐòÁ½¸ö¿é
		struct coo_t *pta, *pte;

		pte = array + nmemb;

		while (block < nmemb)
		{
			pta = array;

			for (pta = array; pta + block < pte; pta += block * 2)
			{
				if (pta + block * 2 < pte)
				{
					blit_merge_block(pta, swap, swap_size, block, block, cmp);

					continue;
				}
				blit_merge_block(pta, swap, swap_size, block, pte - pta - block, cmp);

				break;
			}
			block *= 2;
		}
	}

	void blitsort(struct coo_t *array, size_t nmemb, CMPFUNC *cmp)
	{
		if (nmemb < 32)
		{
			tail_swap(array, nmemb, cmp);
		}
		else if (quad_swap(array, nmemb, cmp) == 0)
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
			struct coo_t swap[swap_size];

			quad_merge(array, swap, swap_size, nmemb, 32, cmp);

			blit_merge(array, swap, swap_size, nmemb, swap_size * 2, cmp);
		}
	}
/*
	void blitsort_swap(void *array, void *swap, size_t nmemb, size_t swap_size, cmp)
	{
		if (nmemb < 32)
		{
			tail_swap(array, nmemb, cmp);//ÕûÌåÐ¡ÓÚ32¸öÊý
		}
		else if (quad_swap(array, nmemb, cmp) == 0)//·Ç8ÕûÊý±¶
		{
			quad_merge(array, swap, swap_size, nmemb, 32, cmp);//Ò»´ÎÅÅ32

			blit_merge(array, swap, swap_size, nmemb, swap_size * 2, cmp);//ÅÅÊ£ÏÂµÄ
		}
	}
	*/

