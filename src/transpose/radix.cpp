#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "taco/taco_tensor_t.h"
#include <vector>

namespace taco
{

struct coord_t
{
    uint64_t crd;
    double val;
};

int transpose_radixsort_all(taco_tensor_t *A, taco_tensor_t *B)
{
    uint32_t B0_size = B->dimensions[0];
    uint32_t B1_size = B->dimensions[1];
    uint32_t B2_size = B->dimensions[2];

    uint8_t B0_dim_size = 32;
    uint64_t A2_mask = 0xFFFFFFFF00000000;
    while (!(B0_size & (1 << (B0_dim_size - 1))))
    {
        B0_dim_size--;
        A2_mask |= (1 << (B0_dim_size));
    }
    A2_mask = ~A2_mask;

    uint8_t B1_dim_size = 32;
    uint64_t A1_mask = 0xFFFFFFFF00000000;
    while (!(B1_size & (1 << (B1_dim_size - 1))))
    {
        B1_dim_size--;
        A1_mask |= (1 << (B1_dim_size));
    }
    A1_mask = ~A1_mask;

    uint8_t B2_dim_size = 32;
    uint64_t A0_mask = 0xFFFFFFFF00000000;
    while (!(B2_size & (1 << (B2_dim_size - 1))))
    {
        B2_dim_size--;
        A0_mask |= (1 << (B2_dim_size));
    }
    A0_mask = ~A0_mask;

    if (B0_dim_size + B1_dim_size + B2_dim_size > 64)
    {
        std::cout << "FAIL" << std::endl;
        std::cout << B0_dim_size + B1_dim_size + B2_dim_size << std::endl;
        return -1;
    }

    uint8_t sh1 = B0_dim_size;
    uint8_t sh2 = sh1 + B1_dim_size;

    int *A0_pos_arr = (int *)(A->indices[0][0]);
    int *A0_idx_arr = (int *)(A->indices[0][1]);
    int *A1_pos_arr = (int *)(A->indices[1][0]);
    int *A1_idx_arr = (int *)(A->indices[1][1]);
    int *A2_pos_arr = (int *)(A->indices[2][0]);
    int *A2_idx_arr = (int *)(A->indices[2][1]);
    double *A_val_arr = (double *)(A->vals);

    int *B0_pos_arr = (int *)(B->indices[0][0]);
    int *B0_idx_arr = (int *)(B->indices[0][1]);
    int *B1_pos_arr = (int *)(B->indices[1][0]);
    int *B1_idx_arr = (int *)(B->indices[1][1]);
    int *B2_pos_arr = (int *)(B->indices[2][0]);
    int *B2_idx_arr = (int *)(B->indices[2][1]);
    double *B_val_arr = (double *)(B->vals);

    int32_t init_alloc_size = 1048576;
    int32_t C_capacity = init_alloc_size;
    int32_t c_size = 0;
    struct coord_t *C = (struct coord_t *)malloc(sizeof(struct coord_t) * C_capacity);

    uint64_t max = 0; 
    for (int32_t B0_pos = B0_pos_arr[0]; B0_pos < B0_pos_arr[1]; B0_pos++)
    {
        uint64_t iB = (uint64_t)B0_idx_arr[B0_pos];
        int32_t B0_end = B0_pos + 1;
        for (int32_t B1_pos = B1_pos_arr[B0_pos]; B1_pos < B1_pos_arr[B0_end]; B1_pos++)
        {
            uint64_t jB = (uint64_t)B1_idx_arr[B1_pos];
            int32_t B1_end = B1_pos + 1;
            for (int32_t B2_pos = B2_pos_arr[B1_pos]; B2_pos < B2_pos_arr[B1_end]; B2_pos++)
            {
                uint64_t kB = (uint64_t)B2_idx_arr[B2_pos];
                if (C_capacity <= (c_size + 1))
                {
                    int32_t C_capacity_new = 2 * (c_size + 1);
                    C = (struct coord_t *)realloc(C, sizeof(struct coord_t) * C_capacity_new);
                    C_capacity = C_capacity_new;
                }
                C[c_size].val = B_val_arr[B2_pos];
                uint64_t crd = (iB) | (jB << sh1) | (kB << (sh2));
                C[c_size].crd = crd;

                if(crd > max)
                    max = crd;

                c_size++;
            }
        }
    }

    int base = 16;
    struct coord_t *C_new = (struct coord_t *)malloc(sizeof(struct coord_t) * c_size);
    struct coord_t *C_temp;
    int32_t *count = (int32_t *)malloc(base* sizeof(int32_t));
    for (uint64_t exp = 1; exp <= max; exp *= base)
    {
        memset(count, 0, sizeof(int32_t)*base);
        // Bucket based on ith digit
        for (int cidx = 0; cidx < c_size; cidx++)
        {
            uint64_t crd = C[cidx].crd;
            crd /= (exp);
            crd = crd % base;
            count[crd]++;
        }

        for (int i = 1; i < base; i++)
        {
           count[i] += count[i - 1];
        }

        // Move into C_new
        for (int cidx = c_size - 1; cidx >= 0; cidx--)
        {
            uint64_t crd = C[cidx].crd;
            crd /= (exp);
            crd = crd % base;
            C_new[count[crd] - 1] = C[cidx];
            count[crd]--;
        }

        // Swap
        C_temp = C_new;
        C_new = C;
        C = C_temp;
    }



    // pack C into A
    int32_t A0_pos_capacity = 2;
    int32_t A0_idx_capacity = init_alloc_size;
    A0_pos_arr = (int *)malloc(sizeof(int) * A0_pos_capacity);
    A0_idx_arr = (int *)malloc(sizeof(int) * A0_idx_capacity);
    A0_pos_arr[0] = 0;
    int32_t A1_pos_capacity = init_alloc_size;
    int32_t A1_idx_capacity = init_alloc_size;
    A1_pos_arr = (int *)malloc(sizeof(int) * A1_pos_capacity);
    A1_idx_arr = (int *)malloc(sizeof(int) * A1_idx_capacity);
    A1_pos_arr[0] = 0;
    int32_t A2_pos_capacity = init_alloc_size;
    int32_t A2_idx_capacity = init_alloc_size;
    A2_pos_arr = (int *)malloc(sizeof(int) * A2_pos_capacity);
    A2_idx_arr = (int *)malloc(sizeof(int) * A2_idx_capacity);
    A2_pos_arr[0] = 0;
    int32_t A_vals_capacity = init_alloc_size;
    A_val_arr = (double *)malloc(sizeof(double) * A_vals_capacity);

    int32_t A0_pos = 0;
    int32_t A1_pos = 0;
    int32_t A2_pos = 0;
    int32_t C0_pos = 0;
    while (C0_pos < c_size)
    {
        uint64_t iC = ((C[C0_pos].crd >> (sh2)) & A0_mask);
        int32_t C0_end = C0_pos + 1;
        while ((C0_end < c_size) && (((C[C0_end].crd >> (sh2)) & A0_mask) == iC))
        {
            C0_end++;
        }

        int32_t A1_pos_start = A1_pos;
        int32_t C1_pos = C0_pos;
        while (C1_pos < C0_end)
        {
            uint64_t jC = (C[C1_pos].crd >> sh1) & A1_mask;
            int32_t C1_end = C1_pos + 1;
            while ((C1_end < C0_end) && (((C[C1_end].crd >> (sh1)) & A1_mask) == jC))
            {
                C1_end++;
            }

            int32_t A2_pos_start = A2_pos;
            for (int32_t C2_pos = C1_pos; C2_pos < C1_end; C2_pos++)
            {
                uint64_t kC = (C[C2_pos].crd) & A2_mask;

                // Pack values into A.
                if (A_vals_capacity <= ((A2_pos + 1) * 1))
                {
                    int32_t A_vals_capacity_new = 2 * ((A2_pos + 1) * 1);
                    A_val_arr = (double *)realloc(A_val_arr, sizeof(double) * A_vals_capacity_new);
                    A_vals_capacity = A_vals_capacity_new;
                }
                A_val_arr[A2_pos] = C[C2_pos].val;

                if (A2_idx_capacity <= A2_pos)
                {
                    A2_idx_capacity = 2 * A2_pos;
                    A2_idx_arr = (int *)realloc(A2_idx_arr, sizeof(int) * A2_idx_capacity);
                }
                A2_idx_arr[A2_pos] = kC;
                A2_pos++;
            }
            if (A2_pos_capacity <= (A1_pos + 1))
            {
                A2_pos_capacity = 2 * (A1_pos + 1);
                A2_pos_arr = (int *)realloc(A2_pos_arr, sizeof(int) * A2_pos_capacity);
            }
            A2_pos_arr[(A1_pos + 1)] = A2_pos;
            int32_t A2_pos_inserted = A2_pos - A2_pos_start;
            if (A2_pos_inserted > 0)
            {
                if (A1_idx_capacity <= A1_pos)
                {
                    A1_idx_capacity = 2 * A1_pos;
                    A1_idx_arr = (int *)realloc(A1_idx_arr, sizeof(int) * A1_idx_capacity);
                }
                A1_idx_arr[A1_pos] = jC;
                A1_pos++;
            }
            C1_pos = C1_end;
        }
        if (A1_pos_capacity <= (A0_pos + 1))
        {
            A1_pos_capacity = 2 * (A0_pos + 1);
            A1_pos_arr = (int *)realloc(A1_pos_arr, sizeof(int) * A1_pos_capacity);
        }
        A1_pos_arr[(A0_pos + 1)] = A1_pos;
        int32_t A1_pos_inserted = A1_pos - A1_pos_start;
        if (A1_pos_inserted > 0)
        {
            if (A0_idx_capacity <= A0_pos)
            {
                A0_idx_capacity = 2 * A0_pos;
                A0_idx_arr = (int *)realloc(A0_idx_arr, sizeof(int) * A0_idx_capacity);
            }
            A0_idx_arr[A0_pos] = iC;
            A0_pos++;
        }
        C0_pos = C0_end;
    }

    free(C);

    A0_pos_arr[(0 + 1)] = A0_pos;

    A->indices[0][0] = (uint8_t *)(A0_pos_arr);
    A->indices[0][1] = (uint8_t *)(A0_idx_arr);
    A->indices[1][0] = (uint8_t *)(A1_pos_arr);
    A->indices[1][1] = (uint8_t *)(A1_idx_arr);
    A->indices[2][0] = (uint8_t *)(A2_pos_arr);
    A->indices[2][1] = (uint8_t *)(A2_idx_arr);
    A->vals = (uint8_t *)A_val_arr;

    return 0;
}

int transpose_radixsort_pow2_all(taco_tensor_t *A, taco_tensor_t *B)
{
    uint32_t B0_size = B->dimensions[0];
    uint32_t B1_size = B->dimensions[1];
    uint32_t B2_size = B->dimensions[2];

    uint8_t B0_dim_size = 32;
    uint64_t A2_mask = 0xFFFFFFFF00000000;
    while (!(B0_size & (1 << (B0_dim_size - 1))))
    {
        B0_dim_size--;
        A2_mask |= (1 << (B0_dim_size));
    }
    A2_mask = ~A2_mask;

    uint8_t B1_dim_size = 32;
    uint64_t A1_mask = 0xFFFFFFFF00000000;
    while (!(B1_size & (1 << (B1_dim_size - 1))))
    {
        B1_dim_size--;
        A1_mask |= (1 << (B1_dim_size));
    }
    A1_mask = ~A1_mask;

    uint8_t B2_dim_size = 32;
    uint64_t A0_mask = 0xFFFFFFFF00000000;
    while (!(B2_size & (1 << (B2_dim_size - 1))))
    {
        B2_dim_size--;
        A0_mask |= (1 << (B2_dim_size));
    }
    A0_mask = ~A0_mask;

    if (B0_dim_size + B1_dim_size + B2_dim_size > 64)
    {
        std::cout << "FAIL" << std::endl;
        std::cout << B0_dim_size + B1_dim_size + B2_dim_size << std::endl;
        return -1;
    }

    uint8_t sh1 = B0_dim_size;
    uint8_t sh2 = sh1 + B1_dim_size;
    uint64_t coord_size = B0_dim_size + B1_dim_size + B2_dim_size;

    int *A0_pos_arr = (int *)(A->indices[0][0]);
    int *A0_idx_arr = (int *)(A->indices[0][1]);
    int *A1_pos_arr = (int *)(A->indices[1][0]);
    int *A1_idx_arr = (int *)(A->indices[1][1]);
    int *A2_pos_arr = (int *)(A->indices[2][0]);
    int *A2_idx_arr = (int *)(A->indices[2][1]);
    double *A_val_arr = (double *)(A->vals);

    int *B0_pos_arr = (int *)(B->indices[0][0]);
    int *B0_idx_arr = (int *)(B->indices[0][1]);
    int *B1_pos_arr = (int *)(B->indices[1][0]);
    int *B1_idx_arr = (int *)(B->indices[1][1]);
    int *B2_pos_arr = (int *)(B->indices[2][0]);
    int *B2_idx_arr = (int *)(B->indices[2][1]);
    double *B_val_arr = (double *)(B->vals);

    int32_t init_alloc_size = 1048576;
    int32_t C_capacity = init_alloc_size;
    int32_t c_size = 0;
    struct coord_t *C = (struct coord_t *)malloc(sizeof(struct coord_t) * C_capacity);

    for (int32_t B0_pos = B0_pos_arr[0]; B0_pos < B0_pos_arr[1]; B0_pos++)
    {
        uint64_t iB = (uint64_t)B0_idx_arr[B0_pos];
        int32_t B0_end = B0_pos + 1;
        for (int32_t B1_pos = B1_pos_arr[B0_pos]; B1_pos < B1_pos_arr[B0_end]; B1_pos++)
        {
            uint64_t jB = (uint64_t)B1_idx_arr[B1_pos];
            int32_t B1_end = B1_pos + 1;
            for (int32_t B2_pos = B2_pos_arr[B1_pos]; B2_pos < B2_pos_arr[B1_end]; B2_pos++)
            {
                uint64_t kB = (uint64_t)B2_idx_arr[B2_pos];
                if (C_capacity <= (c_size + 1))
                {
                    int32_t C_capacity_new = 2 * (c_size + 1);
                    C = (struct coord_t *)realloc(C, sizeof(struct coord_t) * C_capacity_new);
                    C_capacity = C_capacity_new;
                }
                C[c_size].val = B_val_arr[B2_pos];
                uint64_t crd = (iB) | (jB << sh1) | (kB << (sh2));
                C[c_size].crd = crd;

                c_size++;
            }
        }
    }

    int pow = 2;
    int base = (1 << pow);
    int64_t base_mask = 0;
    for(int i = 0; i < pow; i++) {
        base_mask |= (1 << i);
    }
    struct coord_t *C_new = (struct coord_t *)malloc(sizeof(struct coord_t) * c_size);
    struct coord_t *C_temp;
    int32_t *count = (int32_t *)malloc(base* sizeof(int32_t));
    for (uint64_t shift = 0; shift < coord_size; shift += pow)
    {
        memset(count, 0, sizeof(int32_t)*base);
        // Bucket based on ith digit
        for (int cidx = 0; cidx < c_size; cidx++)
        {
            uint64_t crd = C[cidx].crd;
            crd = crd >> shift;
            crd = crd & base_mask;
            count[crd]++;
        }

        for (int i = 1; i < base; i++)
        {
           count[i] += count[i - 1];
        }

        // Move into C_new
        for (int cidx = c_size - 1; cidx >= 0; cidx--)
        {
            uint64_t crd = C[cidx].crd;
            crd = crd >> shift;
            crd = crd & base_mask;
            C_new[count[crd] - 1] = C[cidx];
            count[crd]--;
        }

        // Swap
        C_temp = C_new;
        C_new = C;
        C = C_temp;
    }



    // pack C into A
    int32_t A0_pos_capacity = 2;
    int32_t A0_idx_capacity = init_alloc_size;
    A0_pos_arr = (int *)malloc(sizeof(int) * A0_pos_capacity);
    A0_idx_arr = (int *)malloc(sizeof(int) * A0_idx_capacity);
    A0_pos_arr[0] = 0;
    int32_t A1_pos_capacity = init_alloc_size;
    int32_t A1_idx_capacity = init_alloc_size;
    A1_pos_arr = (int *)malloc(sizeof(int) * A1_pos_capacity);
    A1_idx_arr = (int *)malloc(sizeof(int) * A1_idx_capacity);
    A1_pos_arr[0] = 0;
    int32_t A2_pos_capacity = init_alloc_size;
    int32_t A2_idx_capacity = init_alloc_size;
    A2_pos_arr = (int *)malloc(sizeof(int) * A2_pos_capacity);
    A2_idx_arr = (int *)malloc(sizeof(int) * A2_idx_capacity);
    A2_pos_arr[0] = 0;
    int32_t A_vals_capacity = init_alloc_size;
    A_val_arr = (double *)malloc(sizeof(double) * A_vals_capacity);

    int32_t A0_pos = 0;
    int32_t A1_pos = 0;
    int32_t A2_pos = 0;
    int32_t C0_pos = 0;
    while (C0_pos < c_size)
    {
        uint64_t iC = ((C[C0_pos].crd >> (sh2)) & A0_mask);
        int32_t C0_end = C0_pos + 1;
        while ((C0_end < c_size) && (((C[C0_end].crd >> (sh2)) & A0_mask) == iC))
        {
            C0_end++;
        }

        int32_t A1_pos_start = A1_pos;
        int32_t C1_pos = C0_pos;
        while (C1_pos < C0_end)
        {
            uint64_t jC = (C[C1_pos].crd >> sh1) & A1_mask;
            int32_t C1_end = C1_pos + 1;
            while ((C1_end < C0_end) && (((C[C1_end].crd >> (sh1)) & A1_mask) == jC))
            {
                C1_end++;
            }

            int32_t A2_pos_start = A2_pos;
            for (int32_t C2_pos = C1_pos; C2_pos < C1_end; C2_pos++)
            {
                uint64_t kC = (C[C2_pos].crd) & A2_mask;

                // Pack values into A.
                if (A_vals_capacity <= ((A2_pos + 1) * 1))
                {
                    int32_t A_vals_capacity_new = 2 * ((A2_pos + 1) * 1);
                    A_val_arr = (double *)realloc(A_val_arr, sizeof(double) * A_vals_capacity_new);
                    A_vals_capacity = A_vals_capacity_new;
                }
                A_val_arr[A2_pos] = C[C2_pos].val;

                if (A2_idx_capacity <= A2_pos)
                {
                    A2_idx_capacity = 2 * A2_pos;
                    A2_idx_arr = (int *)realloc(A2_idx_arr, sizeof(int) * A2_idx_capacity);
                }
                A2_idx_arr[A2_pos] = kC;
                A2_pos++;
            }
            if (A2_pos_capacity <= (A1_pos + 1))
            {
                A2_pos_capacity = 2 * (A1_pos + 1);
                A2_pos_arr = (int *)realloc(A2_pos_arr, sizeof(int) * A2_pos_capacity);
            }
            A2_pos_arr[(A1_pos + 1)] = A2_pos;
            int32_t A2_pos_inserted = A2_pos - A2_pos_start;
            if (A2_pos_inserted > 0)
            {
                if (A1_idx_capacity <= A1_pos)
                {
                    A1_idx_capacity = 2 * A1_pos;
                    A1_idx_arr = (int *)realloc(A1_idx_arr, sizeof(int) * A1_idx_capacity);
                }
                A1_idx_arr[A1_pos] = jC;
                A1_pos++;
            }
            C1_pos = C1_end;
        }
        if (A1_pos_capacity <= (A0_pos + 1))
        {
            A1_pos_capacity = 2 * (A0_pos + 1);
            A1_pos_arr = (int *)realloc(A1_pos_arr, sizeof(int) * A1_pos_capacity);
        }
        A1_pos_arr[(A0_pos + 1)] = A1_pos;
        int32_t A1_pos_inserted = A1_pos - A1_pos_start;
        if (A1_pos_inserted > 0)
        {
            if (A0_idx_capacity <= A0_pos)
            {
                A0_idx_capacity = 2 * A0_pos;
                A0_idx_arr = (int *)realloc(A0_idx_arr, sizeof(int) * A0_idx_capacity);
            }
            A0_idx_arr[A0_pos] = iC;
            A0_pos++;
        }
        C0_pos = C0_end;
    }

    free(C);

    A0_pos_arr[(0 + 1)] = A0_pos;

    A->indices[0][0] = (uint8_t *)(A0_pos_arr);
    A->indices[0][1] = (uint8_t *)(A0_idx_arr);
    A->indices[1][0] = (uint8_t *)(A1_pos_arr);
    A->indices[1][1] = (uint8_t *)(A1_idx_arr);
    A->indices[2][0] = (uint8_t *)(A2_pos_arr);
    A->indices[2][1] = (uint8_t *)(A2_idx_arr);
    A->vals = (uint8_t *)A_val_arr;

    return 0;
}

int transpose_radixsort_pow2_skip(taco_tensor_t *A, taco_tensor_t *B)
{
    uint32_t B0_size = B->dimensions[0];
    uint32_t B1_size = B->dimensions[1];
    uint32_t B2_size = B->dimensions[2];

    uint8_t B0_dim_size = 32;
    uint64_t A2_mask = 0xFFFFFFFF00000000;
    while (!(B0_size & (1 << (B0_dim_size - 1))))
    {
        B0_dim_size--;
        A2_mask |= (1 << (B0_dim_size));
    }
    A2_mask = ~A2_mask;

    uint8_t B1_dim_size = 32;
    uint64_t A1_mask = 0xFFFFFFFF00000000;
    while (!(B1_size & (1 << (B1_dim_size - 1))))
    {
        B1_dim_size--;
        A1_mask |= (1 << (B1_dim_size));
    }
    A1_mask = ~A1_mask;

    uint8_t B2_dim_size = 32;
    uint64_t A0_mask = 0xFFFFFFFF00000000;
    while (!(B2_size & (1 << (B2_dim_size - 1))))
    {
        B2_dim_size--;
        A0_mask |= (1 << (B2_dim_size));
    }
    A0_mask = ~A0_mask;

    if (B0_dim_size + B1_dim_size + B2_dim_size > 64)
    {
        std::cout << "FAIL" << std::endl;
        std::cout << B0_dim_size + B1_dim_size + B2_dim_size << std::endl;
        return -1;
    }

    uint8_t sh1 = B0_dim_size;
    uint8_t sh2 = sh1 + B1_dim_size;
    uint64_t coord_size = B0_dim_size + B1_dim_size + B2_dim_size;

    int *A0_pos_arr = (int *)(A->indices[0][0]);
    int *A0_idx_arr = (int *)(A->indices[0][1]);
    int *A1_pos_arr = (int *)(A->indices[1][0]);
    int *A1_idx_arr = (int *)(A->indices[1][1]);
    int *A2_pos_arr = (int *)(A->indices[2][0]);
    int *A2_idx_arr = (int *)(A->indices[2][1]);
    double *A_val_arr = (double *)(A->vals);

    int *B0_pos_arr = (int *)(B->indices[0][0]);
    int *B0_idx_arr = (int *)(B->indices[0][1]);
    int *B1_pos_arr = (int *)(B->indices[1][0]);
    int *B1_idx_arr = (int *)(B->indices[1][1]);
    int *B2_pos_arr = (int *)(B->indices[2][0]);
    int *B2_idx_arr = (int *)(B->indices[2][1]);
    double *B_val_arr = (double *)(B->vals);

    int32_t init_alloc_size = 1048576;
    int32_t C_capacity = init_alloc_size;
    int32_t c_size = 0;
    struct coord_t *C = (struct coord_t *)malloc(sizeof(struct coord_t) * C_capacity);

    for (int32_t B0_pos = B0_pos_arr[0]; B0_pos < B0_pos_arr[1]; B0_pos++)
    {
        uint64_t iB = (uint64_t)B0_idx_arr[B0_pos];
        int32_t B0_end = B0_pos + 1;
        for (int32_t B1_pos = B1_pos_arr[B0_pos]; B1_pos < B1_pos_arr[B0_end]; B1_pos++)
        {
            uint64_t jB = (uint64_t)B1_idx_arr[B1_pos];
            int32_t B1_end = B1_pos + 1;
            for (int32_t B2_pos = B2_pos_arr[B1_pos]; B2_pos < B2_pos_arr[B1_end]; B2_pos++)
            {
                uint64_t kB = (uint64_t)B2_idx_arr[B2_pos];
                if (C_capacity <= (c_size + 1))
                {
                    int32_t C_capacity_new = 2 * (c_size + 1);
                    C = (struct coord_t *)realloc(C, sizeof(struct coord_t) * C_capacity_new);
                    C_capacity = C_capacity_new;
                }
                C[c_size].val = B_val_arr[B2_pos];
                uint64_t crd = (iB) | (jB << sh1) | (kB << (sh2));
                C[c_size].crd = crd;

                c_size++;
            }
        }
    }

    int pow = 6;
    int base = (1 << pow);
    int64_t base_mask = 0;
    for(int i = 0; i < pow; i++) {
        base_mask |= (1 << i);
    }
    struct coord_t *C_new = (struct coord_t *)malloc(sizeof(struct coord_t) * c_size);
    struct coord_t *C_temp;
    int32_t *count = (int32_t *)malloc(base* sizeof(int32_t));
    for (uint64_t shift = B0_dim_size; shift < coord_size; shift += pow)
    {
        memset(count, 0, sizeof(int32_t)*base);
        // Bucket based on ith digit
        for (int cidx = 0; cidx < c_size; cidx++)
        {
            uint64_t crd = C[cidx].crd;
            crd = crd >> shift;
            crd = crd & base_mask;
            count[crd]++;
        }

        for (int i = 1; i < base; i++)
        {
           count[i] += count[i - 1];
        }

        // Move into C_new
        for (int cidx = c_size - 1; cidx >= 0; cidx--)
        {
            uint64_t crd = C[cidx].crd;
            crd = crd >> shift;
            crd = crd & base_mask;
            C_new[count[crd] - 1] = C[cidx];
            count[crd]--;
        }

        // Swap
        C_temp = C_new;
        C_new = C;
        C = C_temp;
    }



    // pack C into A
    int32_t A0_pos_capacity = 2;
    int32_t A0_idx_capacity = init_alloc_size;
    A0_pos_arr = (int *)malloc(sizeof(int) * A0_pos_capacity);
    A0_idx_arr = (int *)malloc(sizeof(int) * A0_idx_capacity);
    A0_pos_arr[0] = 0;
    int32_t A1_pos_capacity = init_alloc_size;
    int32_t A1_idx_capacity = init_alloc_size;
    A1_pos_arr = (int *)malloc(sizeof(int) * A1_pos_capacity);
    A1_idx_arr = (int *)malloc(sizeof(int) * A1_idx_capacity);
    A1_pos_arr[0] = 0;
    int32_t A2_pos_capacity = init_alloc_size;
    int32_t A2_idx_capacity = init_alloc_size;
    A2_pos_arr = (int *)malloc(sizeof(int) * A2_pos_capacity);
    A2_idx_arr = (int *)malloc(sizeof(int) * A2_idx_capacity);
    A2_pos_arr[0] = 0;
    int32_t A_vals_capacity = init_alloc_size;
    A_val_arr = (double *)malloc(sizeof(double) * A_vals_capacity);

    int32_t A0_pos = 0;
    int32_t A1_pos = 0;
    int32_t A2_pos = 0;
    int32_t C0_pos = 0;
    while (C0_pos < c_size)
    {
        uint64_t iC = ((C[C0_pos].crd >> (sh2)) & A0_mask);
        int32_t C0_end = C0_pos + 1;
        while ((C0_end < c_size) && (((C[C0_end].crd >> (sh2)) & A0_mask) == iC))
        {
            C0_end++;
        }

        int32_t A1_pos_start = A1_pos;
        int32_t C1_pos = C0_pos;
        while (C1_pos < C0_end)
        {
            uint64_t jC = (C[C1_pos].crd >> sh1) & A1_mask;
            int32_t C1_end = C1_pos + 1;
            while ((C1_end < C0_end) && (((C[C1_end].crd >> (sh1)) & A1_mask) == jC))
            {
                C1_end++;
            }

            int32_t A2_pos_start = A2_pos;
            for (int32_t C2_pos = C1_pos; C2_pos < C1_end; C2_pos++)
            {
                uint64_t kC = (C[C2_pos].crd) & A2_mask;

                // Pack values into A.
                if (A_vals_capacity <= ((A2_pos + 1) * 1))
                {
                    int32_t A_vals_capacity_new = 2 * ((A2_pos + 1) * 1);
                    A_val_arr = (double *)realloc(A_val_arr, sizeof(double) * A_vals_capacity_new);
                    A_vals_capacity = A_vals_capacity_new;
                }
                A_val_arr[A2_pos] = C[C2_pos].val;

                if (A2_idx_capacity <= A2_pos)
                {
                    A2_idx_capacity = 2 * A2_pos;
                    A2_idx_arr = (int *)realloc(A2_idx_arr, sizeof(int) * A2_idx_capacity);
                }
                A2_idx_arr[A2_pos] = kC;
                A2_pos++;
            }
            if (A2_pos_capacity <= (A1_pos + 1))
            {
                A2_pos_capacity = 2 * (A1_pos + 1);
                A2_pos_arr = (int *)realloc(A2_pos_arr, sizeof(int) * A2_pos_capacity);
            }
            A2_pos_arr[(A1_pos + 1)] = A2_pos;
            int32_t A2_pos_inserted = A2_pos - A2_pos_start;
            if (A2_pos_inserted > 0)
            {
                if (A1_idx_capacity <= A1_pos)
                {
                    A1_idx_capacity = 2 * A1_pos;
                    A1_idx_arr = (int *)realloc(A1_idx_arr, sizeof(int) * A1_idx_capacity);
                }
                A1_idx_arr[A1_pos] = jC;
                A1_pos++;
            }
            C1_pos = C1_end;
        }
        if (A1_pos_capacity <= (A0_pos + 1))
        {
            A1_pos_capacity = 2 * (A0_pos + 1);
            A1_pos_arr = (int *)realloc(A1_pos_arr, sizeof(int) * A1_pos_capacity);
        }
        A1_pos_arr[(A0_pos + 1)] = A1_pos;
        int32_t A1_pos_inserted = A1_pos - A1_pos_start;
        if (A1_pos_inserted > 0)
        {
            if (A0_idx_capacity <= A0_pos)
            {
                A0_idx_capacity = 2 * A0_pos;
                A0_idx_arr = (int *)realloc(A0_idx_arr, sizeof(int) * A0_idx_capacity);
            }
            A0_idx_arr[A0_pos] = iC;
            A0_pos++;
        }
        C0_pos = C0_end;
    }

    free(C);

    A0_pos_arr[(0 + 1)] = A0_pos;

    A->indices[0][0] = (uint8_t *)(A0_pos_arr);
    A->indices[0][1] = (uint8_t *)(A0_idx_arr);
    A->indices[1][0] = (uint8_t *)(A1_pos_arr);
    A->indices[1][1] = (uint8_t *)(A1_idx_arr);
    A->indices[2][0] = (uint8_t *)(A2_pos_arr);
    A->indices[2][1] = (uint8_t *)(A2_idx_arr);
    A->vals = (uint8_t *)A_val_arr;

    return 0;
}


} // namespace taco
