#include <iostream>
#include "taco.h"
#include "taco/util/timers.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <cmath>
#include <climits>

#include "taco/tensor.h"
#include "taco/format.h"
#include "taco/error.h"
#include "taco/util/strings.h"
#include "taco/util/files.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include"sort.h"


using namespace taco;

//传入比较的对象和需要比较的维度，和比较的维度值,在基于桶排序才用的上 (10)432
int cmp(const void *p, const void *q, int *a, int length) {
	struct coo_t * r = (struct coo_t *)p;
	struct coo_t * s = (struct coo_t *)q;
	int count = 0;
	while (count < length) {
		int one = a[count];
		if (r->idx[one] < s->idx[one]) {
			return -1;
		}
		else if (r->idx[one] > s->idx[one]) {
			return 1;
		}
		count++;

	}
	return 0;

}


int right_num(int *a, int *b, int order) {
	for (int i = 0; i < order; i++) {
		for (int j = i + 1; j < order; j++) {
			if (a[i] == a[j]) {
				std::cout << "right,OK" << std::endl;
				return 1;
			}
		}
	}
	for (int k = 0; k < order; k++) {
		for (int n = k + 1; n < order; n++) {
			if (b[k] == b[n]) {
				std::cout << "False" << std::endl;
				return 1;
			}
		}
	}
	std::cout << "数据未出错，无存在重复数据" << std::endl;
	return 0;
}
//判断前面顺序的找到位置
int find_before(int x, int *a, int order) {
	for (int i = 0; i < order; i++) {
		if (*(a + i) == x) {
			return i;
		}
	}
	std::cout << "初始顺序存在前后不对应" << std::endl;
	return 0;

}
//确定输入的位数
int checkNumber(int num) {
	int weishu = 0;
	for (int i = 1, num1 = num, shuzu = 0; num / i; i = (i * 10)) {
		num1 = num1 / 10;
		weishu++;
	}
	return weishu;
}
//将输入的顺序存储在数组中
int *NumberInputArr(int num, int *a, int order) {
	int weight = 0;
	int level = 0;
	level = checkNumber(num);
	if (level != order) {
		a[0] = 0;
	}
	weight = order - 1;
	for (int i = 1, num1 = num; num / i; i = (i * 10)) {
		//		std::cout << num1 << std::endl;
		a[weight] = num1 % 10;
		num1 = num1 / 10;
		weight--;
	}
	return a;
}
//将前面a指定位置段的顺序导入后面b
int *bucketArr(int *a, int* b, int start, int end) {
	int count = 0;
	for (int i = start; i < end; i++) {
		b[count++] = a[i];
	}
	return b;
}





//传进两个顺序，判断某位置开始后的集合关系,如果b属于a返回1/yes，不属于返回0
int func(int *a, int *b, int i, int j, int order) {
	int numa = i;
	int numb = j + 1;
	if (j == order - 1) {
		std::cout << "Yes" << std::endl;
		return 1;
	}
	for (int k = j + 1; k < order; k++) {
		for (int n = i + 1; n < order; n++) {
			//	std::cout << "n+:" << n << std::endl;
			if (*(b + k) == *(a + n))
			{//在数据输入无误情况下，如果j未到达末尾，说明存在b中不属于a
				numb++;
				//		std::cout << "n:" <<  n << std::endl;
				break;
			}
		}
	}
	if (numb == order)
	{

		std::cout << "Yes" << std::endl;
		return 1;
	}
	else {
		std::cout << "No" << std::endl;
	}
	return 0;
}


//counting sort
int counting(struct coo_t *C_coords, int c_size, int order, int *dimensions, int x) {//x用来确定排序到维度
	struct coo_t *coo_t_coords_scratch = (struct coo_t *)malloc(sizeof(struct coo_t) * c_size);
	int Bx_size = dimensions[x] + 1;
	int *Bx_count = (int *)calloc(Bx_size, sizeof(int));
	for (int i = 0; i < c_size; i++)
	{
		int idx1 = C_coords[i].idx[x];
		Bx_count[idx1]++;
	}

	// Prefix sum over Bx_count
	for (int idx1 = 1; idx1 < Bx_size; idx1++)
	{
		Bx_count[idx1] += Bx_count[idx1 - 1];
	}
	for (int i = c_size - 1; i >= 0; i--)
	{
		int idx1 = C_coords[i].idx[x];
		int idx = Bx_count[idx1] - 1;
		coo_t_coords_scratch[idx] = C_coords[i];
		Bx_count[idx1]--;
	}

	memcpy(C_coords, coo_t_coords_scratch, c_size * sizeof(struct coo_t));


	free(Bx_count);
	free(coo_t_coords_scratch);
	return 0;
}

int counting1(int a) {
	std::cout << "**" << a << std::endl;
	return 0;
}



int bucketsort(struct coo_t *C_coords, int c_size, int order, int* a, int bucket_size, int *dimensions, int x) {
	//原文中的桶排序方式
		//b中放置的是需要排序的顺序,length是排序的维度个数长度
		//当桶排序的排序位为最后一位，就不需要排序了
		//a是桶的顺序
	int y = 0;
	int qsort_start = 0;
	for (int i = 1; i < c_size; i++)
	{
		int qsort_same = 1;
		for (int j = 0; j < bucket_size; j++) {
			y = a[j];
			if (C_coords[i].idx[y] != C_coords[i - 1].idx[y]) {
				qsort_same = 0;
				break;
			}
		}
		if (!qsort_same || i == c_size - 1) { // Sort if at the end of a segment
			counting(&C_coords[qsort_start], (i - qsort_start), order, dimensions,x);
//			qsort(&C_coords[qsort_start], (i - qsort_start), sizeof(struct coo_t), cmp_32);
			qsort_start = i;
		}
	}
	return 0;
}




int bucketsort1(struct coo_t *C_coords, int c_size, int order, int* a, int bucket_size, int* b, int length) {
	//b中放置的是需要排序的顺序,length是排序的维度个数长度
	//当桶排序的排序位为最后一位，就不需要排序了
	//a是桶的顺序

	int x = 0;
	int qsort_start = 0;
	for (int i = 1; i < c_size; i++)
	{
		int qsort_same = 1;
		for (int j = 0; j < bucket_size; j++) {
			x = a[j];
			if (C_coords[i].idx[x] != C_coords[i - 1].idx[x]) {
				qsort_same = 0;
				break;
			}
		}
		if (!qsort_same || i == c_size - 1) { // Sort if at the end of a segment
			blitsort(&C_coords[qsort_start], (i - qsort_start), b, length);
			qsort_start = i;
		}
	}

	return 0;
}
int transpose_coo_40132_k5(struct coo_t *C_coords, int c_size, int order, int *dimensions, int* a, int bucket_size, int num) {
	struct coo_t* C_coords_scratch = (struct coo_t *)malloc(sizeof(struct coo_t) * c_size);
	int* perm = (int *)malloc(sizeof(int) * c_size);
	int *bucket = (int *)malloc(c_size * sizeof(int));
	int *quotient = (int *)malloc(c_size * sizeof(int));
	int current = 0;
	bucket[0] = 0;
	quotient[0] = 0;
	int x = 0;
	for (int i = 1; i < c_size; i++)
	{

		int qsort_same = 1;
		for (int j = 0; j < bucket_size; j++) {
			x = a[j];
			if (C_coords[i].idx[x] != C_coords[i - 1].idx[x]) {
				qsort_same = 0;
				current++;
				quotient[current] = i;
				break;
			}
		}


		bucket[i] = current;
	}

	// Histogram sort on mode 3 into the permutation array
	int Bx_size = dimensions[num] + 1;
	int32_t *Bx_count = (int32_t *)calloc(Bx_size, sizeof(int32_t));
	for (int i = 0; i < c_size; i++)
	{
		int32_t idx3 = C_coords[i].idx[num];
		Bx_count[idx3]++;
	}

	// Prefix sum over B3_count
	for (int idx3 = 1; idx3 < Bx_size; idx3++)
	{
		Bx_count[idx3] += Bx_count[idx3 - 1];
	}
	for (int i = c_size - 1; i >= 0; i--)
	{
		int32_t idx3 = C_coords[i].idx[num];
		int idx = Bx_count[idx3] - 1;
		C_coords_scratch[idx] = C_coords[i];
		perm[idx] = bucket[i];
		Bx_count[idx3]--;
	}
	free(bucket);

	// Move the items back to the buckets.
	for (int i = 0; i < c_size; i++) {
		C_coords[quotient[perm[i]]] = C_coords_scratch[i];
		quotient[perm[i]] ++;
	}
	free(quotient);
	// Free the scratch space.
	free(C_coords_scratch);
	free(perm);
	return 0;
}


int main(int argc, char* argv[]) {
	std::string filename = "/home/xwb/taco/transpose/taco/apps/tensor_5_fusion/lbnl-network.tns";
	char *p;
	int TOTAL;
	int SPLIT;
	int branch;
	int bucket_size = 0;
	int length = 0;
	long conv = strtol(argv[1], &p, 10);

	TOTAL = conv;

	conv = strtol(argv[2], &p, 10);
	SPLIT = conv;

	conv = strtol(argv[3], &p, 10);
	branch = conv;
	// Read tensor data  
	int order = 5;
	int * dimensions = (int*)malloc(sizeof(int)*order);
	memset(dimensions, 0, sizeof(int)*order);

	int32_t init_alloc_size = 1048576;
	int32_t A_capacity = init_alloc_size;
	struct coo_t *A = (struct coo_t *)malloc(sizeof(struct coo_t)* A_capacity);

	std::fstream stream;
	util::openStream(stream, filename, fstream::in);

	std::string line;
	if (!std::getline(stream, line)) {
		return -1;
	}


	int size = 0;

	do {
		if (size >= A_capacity) {
			int32_t A_capacity_new = 2 * (A_capacity);
			A = (struct coo_t*)realloc(A, sizeof(struct coo_t) * A_capacity_new);
			A_capacity = A_capacity_new;
		}

		char* linePtr = (char*)line.data();

		// Generated by coo/gen_taco_to_list.py

		long idx0 = strtol(linePtr, &linePtr, 10);
		A[size].idx[0] = (int32_t)idx0;
		dimensions[0] = std::max(dimensions[0], (int)idx0);
		long idx1 = strtol(linePtr, &linePtr, 10);
		A[size].idx[1] = (int32_t)idx1;
		dimensions[1] = std::max(dimensions[1], (int)idx1);
		long idx2 = strtol(linePtr, &linePtr, 10);
		A[size].idx[2] = (int32_t)idx2;
		dimensions[2] = std::max(dimensions[2], (int)idx2);
		long idx3 = strtol(linePtr, &linePtr, 10);
		A[size].idx[3] = (int32_t)idx3;
		dimensions[3] = std::max(dimensions[3], (int)idx3);
		long idx4 = strtol(linePtr, &linePtr, 10);
		A[size].idx[4] = (int32_t)idx4;
		dimensions[4] = std::max(dimensions[4], (int)idx4);

		double val = strtod(linePtr, &linePtr);
		A[size].val = val;
		size++;

	} while (std::getline(stream, line));

	stream.close();
	int a[5];
	NumberInputArr(TOTAL, a, order);
	std::cout << a[0] << a[1] << a[2] << a[3] << a[4] << std::endl;

	int b[5];
	NumberInputArr(SPLIT, b, order);
	std::cout << b[0] << b[1] << b[2] << b[3] << b[4] << std::endl;
	int number = right_num(a, b, order);
	if (number) {
		exit(0);
	}
	for (int i = 4; i >= 0; i--) {
		std::cout << a[i] << " ";
		counting(A, size, order, dimensions, a[i]);
	}
	int L1 = 0;
	int L2 = 0;
	//	std::cout << func(a, b, 2, 0, order) << std::endl;
	//	std::cout << func(a, b, 0, 1, order) << std::endl;
	//	std::cout << func(a, b, 1, 2, order) << std::endl;
	int c[5] = { 0,0,0,0,0 };
	//	qsort(A, size, sizeof(struct coo_t), cmp_01234);

	taco::util::Timer timer;
        timer.start();

	while (L1 < order) {
		int k = L1;
		int after_b = b[L1];
		int i_a = find_before(after_b, a, order);
		while ((k < order) && (!func(a, b, i_a, k, order))) {
			k = k + 1;
			after_b = b[k];
			i_a = find_before(after_b, a, order);
		}
		L2 = k + 1;
		if (L1 == 0) {//(210)354
			while (L1 < k) {
				int x = b[k - 1];//排序前一位开始
				counting(A, size, order, dimensions, x);
				k = k - 1;

			}
		}
		else {
			if ((L1 != order - 1) && (L1 != k)) {
				std::cout << "xyz" << std::endl;
				if (branch == 1) {
					//计数排序桶内方法
					while (L1 < k) {
						int x = b[k - 1];//排序前一位开始
						bucket_size = L1;
						//将b中需要比较放在c
						bucketsort(A, size, order, b, bucket_size, dimensions, x);
						k = k - 1;
					}
				}
				if (branch == 2) {
					//归并排序方法
					bucket_size = L1;
					length = k - L1;
					//将b中需要比较放在c
					bucketArr(b, c, L1, k);
					std::cout << c[0] << c[1] << c[2] << std::endl;
					bucketsort1(A, size, order, b, bucket_size, c, length);
				}
				if (branch == 3) {
					//桶排序方法
					while (L1 < k) {
						int x = b[k - 1];//排序前一位开始
						bucket_size = L1;
						//将b中需要比较放在c
						transpose_coo_40132_k5(A, size, order, dimensions, b, bucket_size, x);

						k = k - 1;
					}

				}
			}
		}

		L1 = L2;
	}
	timer.stop();
	taco::util::TimeResults res = timer.getResult();


	std::cout <<  res  << std::endl;
	/*
	std::ofstream output;

	output.open("/home/xwb/taco/transpose/taco/apps/tensor_5_fusion/tensor5.tns", std::ios::out);

	for(int i = 0; i < size; ++i) {
		output << A[i].idx[0] << " " << A[i].idx[1] << " " << A[i].idx[2] << " " << A[i].idx[3] << " " << A[i].idx[4] << " " << A[i].val << std::endl;
	}

	output.close();
*/
	free(A);
	free(dimensions);

}
