

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "sort.h"
#include "timer.h"
#include "io.h"
#include "thd_info.h"


/******************************************************************************
 * DEFINES
 *****************************************************************************/
/* switch to insertion sort past this point */
#define MIN_QUICKSORT_SIZE 8

/* don't bother spawning threads for small sorts */
#define SMALL_SORT_SIZE 1000


/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

/**
* @brief Compares ind*[i] and ind*[j] for two-mode tensors.
*
* @param ind0 The primary mode to compare. Defer tie-breaks to ind1.
* @param ind1 The secondary mode to compare.
* @param i The index into ind*.
* @param j The second index into ind*.
*�Ƚ�����λ��,01�Ƚϣ�ǰС����-1.��С����1
* @return Returns -1 if ind[i] < ind[j], 1 if ind[i] > ind[j], and 0 if they
*         are equal.
*/
static inline int p_ttcmp2(
  idx_t const * const ind0,
  idx_t const * const ind1,
  idx_t const i,
  idx_t const j)
{
  if(ind0[i] < ind0[j]) {
    return -1;
  } else if(ind0[j] < ind0[i]) {
    return 1;
  }
  if(ind1[i] < ind1[j]) {
    return -1;
  } else if(ind1[j] < ind1[i]) {
    return 1;
  }
  return 0;
}

/**
* @brief Compares ind*[i] and j[*] for two-mode tensors.
*
* @param ind0 The primary mode to compare. Defer tie-breaks to ind1.
* @param ind1 The secondary mode to compare.
* @param i The index into ind*[]
* @param j[2] The indices we are comparing i against.
*j�洢��idx0��idx1
* @return Returns -1 if ind[i] < j, 1 if ind[i] > j, and 0 if they are equal.
*/
static inline int p_ttqcmp2(
  idx_t const * const ind0,
  idx_t const * const ind1,
  idx_t const i,
  idx_t const j[2])
{
  if(ind0[i] < j[0]) {
    return -1;
  } else if(j[0] < ind0[i]) {
    return 1;
  }
  if(ind1[i] < j[1]) {
    return -1;
  } else if(j[1] < ind1[i]) {
    return 1;
  }

  return 0;
}

/**
* @brief Perform insertion sort on a 2-mode tensor between start and end,
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param start The first nonzero to sort.
* @param end The last nonzero to sort.
*ͨ�����뵥������
*/
static void p_tt_insertionsort2(
  sptensor_t * const tt,
  idx_t const * const cmplt,
  idx_t const start,
  idx_t const end)
{
  idx_t * const ind0 = tt->ind[cmplt[0]];
  idx_t * const ind1 = tt->ind[cmplt[1]];
  val_t * const vals = tt->vals;

  val_t vbuf; 
  idx_t ibuf;

  for(size_t i=start+1; i < end; ++i) {
    size_t j = i;
    while (j > start && p_ttcmp2(ind0, ind1, i, j-1) < 0) {
      --j;
    }

    vbuf = vals[i];

    /* shift all data */
	//��xxxx (j)(j+1)xx (i)xxx-->xxxx (i) (j)(j+1)xx xxx
    memmove(vals+j+1, vals+j, (i-j)*sizeof(val_t));
    vals[j] = vbuf;
    ibuf = ind0[i];
    memmove(ind0+j+1, ind0+j, (i-j)*sizeof(idx_t));
    ind0[j] = ibuf;
    ibuf = ind1[i];
    memmove(ind1+j+1, ind1+j, (i-j)*sizeof(idx_t));
    ind1[j] = ibuf;
  }
}

/**
* @brief Perform quicksort on a 2-mode tensor between start and end.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param start The first nonzero to sort.
* @param end The last nonzero to sort.
*/
static void p_tt_quicksort2(
  sptensor_t * const tt,
  idx_t const * const cmplt,
  idx_t const start,
  idx_t const end)
{
  val_t vmid;
  idx_t imid[2];

  idx_t * const ind0 = tt->ind[cmplt[0]];
  idx_t * const ind1 = tt->ind[cmplt[1]];
  val_t * const vals = tt->vals;

  if((end-start) <= MIN_QUICKSORT_SIZE) {
    p_tt_insertionsort2(tt, cmplt, start, end);
  } else {
    size_t i = start+1;
    size_t j = end-1;
    size_t k = start + ((end - start) / 2);

    /* grab pivot */
    vmid = vals[k];
    vals[k] = vals[start];
    imid[0] = ind0[k];
    imid[1] = ind1[k];
    ind0[k] = ind0[start];
    ind1[k] = ind1[start];

    while(i < j) {
      /* if tt[i] > mid  -> tt[i] is on wrong side */
      if(p_ttqcmp2(ind0,ind1,i,imid) == 1) {//ǰ������м�
        /* if tt[j] <= mid  -> swap tt[i] and tt[j]  ����С���м�*/
        if(p_ttqcmp2(ind0,ind1,j,imid) < 1) {
          val_t vtmp = vals[i];
          vals[i] = vals[j];
          vals[j] = vtmp;
          idx_t itmp = ind0[i];
          ind0[i] = ind0[j];
          ind0[j] = itmp;
          itmp = ind1[i];
          ind1[i] = ind1[j];
          ind1[j] = itmp;
          ++i;
        }
        --j;
      }
	  else {//ǰ��С���м�
        /* if tt[j] > mid  -> tt[j] is on right side */
        if(p_ttqcmp2(ind0,ind1,j,imid) == 1) {
          --j;
        }
        ++i;
      }
    }

    /* if tt[i] > mid */
    if(p_ttqcmp2(ind0,ind1,i,imid) == 1) {
      --i;
    }
    vals[start] = vals[i];
    vals[i] = vmid;
    ind0[start] = ind0[i];
    ind1[start] = ind1[i];
    ind0[i] = imid[0];
    ind1[i] = imid[1];

    if(i > start + 1) {
      p_tt_quicksort2(tt, cmplt, start, i);
    }
    ++i; /* skip the pivot element */
    if(end - i > 1) {
      p_tt_quicksort2(tt, cmplt, i, end);
    }
  }
}

/**
* @brief Compares ind*[i] and j[*] for three-mode tensors.
*
* @param ind0 The primary mode to compare. Defer tie-breaks to ind1.
* @param ind1 The secondary mode to compare. Defer tie-breaks to ind2.
* @param ind2 The final tie-breaking mode.
* @param i The index into ind*[]
* @param j[3] The indices we are comparing i against.
*
* @return Returns -1 if ind[i] < j, 1 if ind[i] > j, and 0 if they are equal.
*/
static inline int p_ttqcmp3(
  idx_t const * const ind0,
  idx_t const * const ind1,
  idx_t const * const ind2,
  idx_t const i,
  idx_t const j[3])
{
  if(ind0[i] < j[0]) {
    return -1;
  } else if(j[0] < ind0[i]) {
    return 1;
  }
  if(ind1[i] < j[1]) {
    return -1;
  } else if(j[1] < ind1[i]) {
    return 1;
  }
  if(ind2[i] < j[2]) {
    return -1;
  } else if(j[2] < ind2[i]) {
    return 1;
  }

  return 0;
}


/**
* @brief Compares ind*[i] and ind*[j] for three-mode tensors.
*
* @param ind0 The primary mode to compare. Defer tie-breaks to ind1.
* @param ind1 The secondary mode to compare. Defer tie-breaks to ind2.
* @param ind2 The final tie-breaking mode.
* @param i The index into ind*.
* @param j The second index into ind*.
*
* @return Returns -1 if ind[i] < ind[j], 1 if ind[i] > ind[j], and 0 if they
*         are equal.
*/
static inline int p_ttcmp3(
  idx_t const * const ind0,
  idx_t const * const ind1,
  idx_t const * const ind2,
  idx_t const i,
  idx_t const j)
{
  if(ind0[i] < ind0[j]) {
    return -1;
  } else if(ind0[j] < ind0[i]) {
    return 1;
  }
  if(ind1[i] < ind1[j]) {
    return -1;
  } else if(ind1[j] < ind1[i]) {
    return 1;
  }
  if(ind2[i] < ind2[j]) {
    return -1;
  } else if(ind2[j] < ind2[i]) {
    return 1;
  }
  return 0;
}


/**
* @brief Compares ind*[i] and ind*[j] for n-mode tensors.
*
* @param tt The tensor we are sorting.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param i The index into ind*.
* @param j The second index into ind*.
*n��ά��
* @return Returns -1 if ind[i] < ind[j], 1 if ind[i] > ind[j], and 0 if they
*         are equal.
*/
static inline int p_ttcmp(
  sptensor_t const * const tt,
  idx_t const * const cmplt,
  idx_t const i,
  idx_t const j)
{
  for(idx_t m=0; m < tt->nmodes; ++m) {
    if(tt->ind[cmplt[m]][i] < tt->ind[cmplt[m]][j]) {
      return -1;
    } else if(tt->ind[cmplt[m]][j] < tt->ind[cmplt[m]][i]) {
      return 1;
    }
  }
  return 0;
}


/**
* @brief Compares ind*[i] and ind*[j] for n-mode tensors.
*
* @param tt The tensor we are sorting.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param i The index into ind*.
* @param j The coordinate we are comparing against.
*
* @return Returns -1 if ind[i] < j, 1 if ind[i] > j, and 0 if they are equal.
*/
static inline int p_ttqcmp(
  sptensor_t const * const tt,
  idx_t const * const cmplt,
  idx_t const i,
  idx_t const j[MAX_NMODES])
{
  for(idx_t m=0; m < tt->nmodes; ++m) {
    if(tt->ind[cmplt[m]][i] < j[cmplt[m]]) {
      return -1;
    } else if(j[cmplt[m]] < tt->ind[cmplt[m]][i]) {
      return 1;
    }
  }
  return 0;
}


/**
* @brief Swap nonzeros i and j.
*
* @param tt The tensor to operate on.
* @param i The first nonzero to swap.
* @param j The second nonzero to swap with.
��������λ���ϵĴ洢���ݣ�����ֵ��ά��ֵ
*/
static inline void p_ttswap(
  sptensor_t * const tt,
  idx_t const i,
  idx_t const j)
{
  val_t vtmp = tt->vals[i];
  tt->vals[i] = tt->vals[j];
  tt->vals[j] = vtmp;

  idx_t itmp;
  for(idx_t m=0; m < tt->nmodes; ++m) {
    itmp = tt->ind[m][i];
    tt->ind[m][i] = tt->ind[m][j];
    tt->ind[m][j] = itmp;
  }
}


/**
* @brief Perform insertion sort on a 3-mode tensor between start and end.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param start The first nonzero to sort.
* @param end The last nonzero to sort.
*/
static void p_tt_insertionsort3(
  sptensor_t * const tt,
  idx_t const * const cmplt,
  idx_t const start,
  idx_t const end)
{
  idx_t * const ind0 = tt->ind[cmplt[0]];
  idx_t * const ind1 = tt->ind[cmplt[1]];
  idx_t * const ind2 = tt->ind[cmplt[2]];
  val_t * const vals = tt->vals;

  val_t vbuf;
  idx_t ibuf;

  for(size_t i=start+1; i < end; ++i) {
    size_t j = i;
    while (j > start && p_ttcmp3(ind0, ind1, ind2, i, j-1) < 0) {
      --j;
    }

    vbuf = vals[i];

    /* shift all data */
    memmove(vals+j+1, vals+j, (i-j)*sizeof(val_t));
    vals[j] = vbuf;
    ibuf = ind0[i];
    memmove(ind0+j+1, ind0+j, (i-j)*sizeof(idx_t));
    ind0[j] = ibuf;
    ibuf = ind1[i];
    memmove(ind1+j+1, ind1+j, (i-j)*sizeof(idx_t));
    ind1[j] = ibuf;
    ibuf = ind2[i];
    memmove(ind2+j+1, ind2+j, (i-j)*sizeof(idx_t));
    ind2[j] = ibuf;
  }
}


/**
* @brief Perform insertion sort on an n-mode tensor between start and end.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param start The first nonzero to sort.
* @param end The last nonzero to sort.
*/
static void p_tt_insertionsort(
  sptensor_t * const tt,
  idx_t const * const cmplt,
  idx_t const start,
  idx_t const end)
{
  idx_t * ind;
  val_t * const vals = tt->vals;
  idx_t const nmodes = tt->nmodes;

  val_t vbuf;
  idx_t ibuf;

  for(size_t i=start+1; i < end; ++i) {
    size_t j = i;
    while (j > start && p_ttcmp(tt, cmplt, i, j-1) < 0) {
      --j;
    }

    vbuf = vals[i];

    /* shift all data */
    memmove(vals+j+1, vals+j, (i-j)*sizeof(val_t));
    vals[j] = vbuf;
    for(idx_t m=0; m < nmodes; ++m) {
      ind = tt->ind[m];
      ibuf = ind[i];
      memmove(ind+j+1, ind+j, (i-j)*sizeof(idx_t));
      ind[j] = ibuf;
    }
  }
}


/**
* @brief Perform quicksort on a 3-mode tensor between start and end.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param start The first nonzero to sort.
* @param end The last nonzero to sort.
*/
static void p_tt_quicksort3(
  sptensor_t * const tt,
  idx_t const * const cmplt,
  idx_t const start,
  idx_t const end)
{
  val_t vmid;
  idx_t imid[3];

  idx_t * const ind0 = tt->ind[cmplt[0]];
  idx_t * const ind1 = tt->ind[cmplt[1]];
  idx_t * const ind2 = tt->ind[cmplt[2]];
  val_t * const vals = tt->vals;

  if((end-start) <= MIN_QUICKSORT_SIZE) {
    p_tt_insertionsort3(tt, cmplt, start, end);
  } else {
    size_t i = start+1;
    size_t j = end-1;
    size_t k = start + ((end - start) / 2);

    /* grab pivot */
    vmid = vals[k];
    vals[k] = vals[start];
    imid[0] = ind0[k];
    imid[1] = ind1[k];
    imid[2] = ind2[k];
    ind0[k] = ind0[start];
    ind1[k] = ind1[start];
    ind2[k] = ind2[start];

    while(i < j) {
      /* if tt[i] > mid  -> tt[i] is on wrong side */
      if(p_ttqcmp3(ind0,ind1,ind2,i,imid) == 1) {
        /* if tt[j] <= mid  -> swap tt[i] and tt[j] */
        if(p_ttqcmp3(ind0,ind1,ind2,j,imid) < 1) {
          val_t vtmp = vals[i];
          vals[i] = vals[j];
          vals[j] = vtmp;
          idx_t itmp = ind0[i];
          ind0[i] = ind0[j];
          ind0[j] = itmp;
          itmp = ind1[i];
          ind1[i] = ind1[j];
          ind1[j] = itmp;
          itmp = ind2[i];
          ind2[i] = ind2[j];
          ind2[j] = itmp;
          ++i;
        }
        --j;
      } else {
        /* if tt[j] > mid  -> tt[j] is on right side */
        if(p_ttqcmp3(ind0,ind1,ind2,j,imid) == 1) {
          --j;
        }
        ++i;
      }
    }

    /* if tt[i] > mid */
    if(p_ttqcmp3(ind0,ind1,ind2,i,imid) == 1) {
      --i;
    }
    vals[start] = vals[i];
    vals[i] = vmid;
    ind0[start] = ind0[i];
    ind1[start] = ind1[i];
    ind2[start] = ind2[i];
    ind0[i] = imid[0];
    ind1[i] = imid[1];
    ind2[i] = imid[2];

    if(i > start + 1) {
      p_tt_quicksort3(tt, cmplt, start, i);
    }
    ++i; /* skip the pivot element */
    if(end - i > 1) {
      p_tt_quicksort3(tt, cmplt, i, end);
    }
  }
}


/**
* @brief Perform quicksort on a n-mode tensor between start and end.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param start The first nonzero to sort.
* @param end The last nonzero to sort.
*/
static void p_tt_quicksort(
  sptensor_t * const tt,
  idx_t const * const cmplt,
  idx_t const start,
  idx_t const end)
{
  val_t vmid;
  idx_t imid[MAX_NMODES];

  idx_t * ind;
  val_t * const vals = tt->vals;
  idx_t const nmodes = tt->nmodes;

  if((end-start) <= MIN_QUICKSORT_SIZE) {
    p_tt_insertionsort(tt, cmplt, start, end);
  } else {
    size_t i = start+1;
    size_t j = end-1;
    size_t k = start + ((end - start) / 2);

    /* grab pivot */
    vmid = vals[k];
    vals[k] = vals[start];
    for(idx_t m=0; m < nmodes; ++m) {
      ind = tt->ind[m];
      imid[m] = ind[k];
      ind[k] = ind[start];
    }

    while(i < j) {
      /* if tt[i] > mid  -> tt[i] is on wrong side */
      if(p_ttqcmp(tt,cmplt,i,imid) == 1) {
        /* if tt[j] <= mid  -> swap tt[i] and tt[j] */
        if(p_ttqcmp(tt,cmplt,j,imid) < 1) {
          p_ttswap(tt,i,j);
          ++i;
        }
        --j;
      } else {
        /* if tt[j] > mid  -> tt[j] is on right side */
        if(p_ttqcmp(tt,cmplt,j,imid) == 1) {
          --j;
        }
        ++i;
      }
    }

    /* if tt[i] > mid */
    if(p_ttqcmp(tt,cmplt,i,imid) == 1) {
      --i;
    }
    vals[start] = vals[i];
    vals[i] = vmid;
    for(idx_t m=0; m < nmodes; ++m) {
      ind = tt->ind[m];
      ind[start] = ind[i];
      ind[i] = imid[m];
    }

    if(i > start + 1) {
      p_tt_quicksort(tt, cmplt, start, i);
    }
    ++i; /* skip the pivot element */
    if(end - i > 1) {
      p_tt_quicksort(tt, cmplt, i, end);
    }
  }
}


/**
* @brief Perform a simple serial quicksort.
*
* @param a The array to sort.
* @param n The length of the array.
*/
static void p_quicksort(
  idx_t * const a,
  idx_t const n)
{
  if(n < MIN_QUICKSORT_SIZE) {
    insertion_sort(a, n);
  } else {
    size_t i = 1;
    size_t j = n-1;
    size_t k = n >> 1;
    idx_t mid = a[k];
    a[k] = a[0];
    while(i < j) {
      if(a[i] > mid) { /* a[i] is on the wrong side */
        if(a[j] <= mid) { /* swap a[i] and a[j] */
          idx_t tmp = a[i];
          a[i] = a[j];
          a[j] = tmp;
          ++i;
        }
        --j;
      } else {
        if(a[j] > mid) { /* a[j] is on the right side */
          --j;
        }
        ++i;
      }
    }

    if(a[i] > mid) {
      --i;
    }
    a[0] = a[i];
    a[i] = mid;

    if(i > 1) {
      p_quicksort(a,i);
    }
    ++i; /* skip the pivot element */
    if(n-i > 1) {
      p_quicksort(a+i, n-i);
    }
  }
}


/**
* @brief Perform a simple serial quicksort with permutation tracking.
*
* @param a The array to sort.
* @param perm The permutation array.
* @param n The length of the array.
*restrict�޶���ָ���������޷��Ա��������޸�
*/
static void p_quicksort_perm(
  idx_t * const restrict a,
  idx_t * const restrict perm,
  idx_t const n)
{
  if(n < MIN_QUICKSORT_SIZE) {
    insertion_sort_perm(a, perm, n);
  } else {
    size_t i = 1;
    size_t j = n-1;
    size_t k = n >> 1;
    idx_t mid = a[k];
    idx_t pmid = perm[k];
    a[k] = a[0];
    perm[k] = perm[0];
    while(i < j) {
      if(a[i] > mid) { /* a[i] is on the wrong side */
        if(a[j] <= mid) { /* swap a[i] and a[j] */
          idx_t tmp = a[i];
          a[i] = a[j];
          a[j] = tmp;

          /* swap perm */
          tmp = perm[i];
          perm[i] = perm[j];
          perm[j] = tmp;
          ++i;
        }
        --j;
      } else {
        if(a[j] > mid) { /* a[j] is on the right side */
          --j;
        }
        ++i;
      }
    }

    if(a[i] > mid) {
      --i;
    }
    a[0] = a[i];
    a[i] = mid;

    /* track median too */
    perm[0] = perm[i];
    perm[i] = pmid;

    if(i > 1) {
      p_quicksort_perm(a, perm, i);
    }
    ++i; /* skip the pivot element */
    if(n-i > 1) {
      p_quicksort_perm(a+i, perm+i, n-i);
    }
  }
}


/**
 * idx = idx2*dim1 + idx1
 * -> ret = idx1*dim2 + idx2
 *        = (idx%dim1)*dim2 + idx/dim1
 */
static inline idx_t p_transpose_idx(
    idx_t const idx,
    idx_t const dim1,
    idx_t const dim2)
{
  return idx%dim1*dim2 + idx/dim1;
}

/**
* @brief Perform a counting sort on the most significant mode (cmplt[0]) and
*        then parallel quicksorts on each of slices.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
*/
static void p_counting_sort_hybrid(
    sptensor_t * const tt,
    idx_t * const cmplt)
{
  idx_t m = cmplt[0];
  idx_t nslices = tt->dims[m];

  idx_t * new_ind[MAX_NMODES];
  for(idx_t i = 0; i < tt->nmodes; ++i) {
    if(i != m) {
      new_ind[i] = splatt_malloc(tt->nnz * sizeof(**new_ind));
    }
  }
  val_t * new_vals = splatt_malloc(tt->nnz * sizeof(*new_vals));

  idx_t * histogram_array = splatt_malloc(
      (nslices * splatt_omp_get_max_threads() + 1) * sizeof(*histogram_array));

  #pragma omp parallel
  {
    int nthreads = splatt_omp_get_num_threads();
    int tid = splatt_omp_get_thread_num();

    idx_t * histogram = histogram_array + (nslices * tid);
    memset(histogram, 0, nslices * sizeof(idx_t));

    idx_t j_per_thread = (tt->nnz + nthreads - 1)/nthreads;
    idx_t jbegin = SS_MIN(j_per_thread*tid, tt->nnz);
    idx_t jend = SS_MIN(jbegin + j_per_thread, tt->nnz);

    /* count */
    for(idx_t j = jbegin; j < jend; ++j) {
      idx_t idx = tt->ind[m][j];
      ++histogram[idx];
    }

    #pragma omp barrier

    /* prefix sum */
    for(idx_t j = (tid*nslices) + 1; j < (tid+1) * nslices; ++j) {
      idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);
      idx_t transpose_j_minus_1 = p_transpose_idx(j - 1, nthreads, nslices);

      histogram_array[transpose_j] += histogram_array[transpose_j_minus_1];
    }

    #pragma omp barrier
    #pragma omp master
    {
      for(int t = 1; t < nthreads; ++t) {
        idx_t j0 = (nslices*t) - 1, j1 = nslices * (t+1) - 1;
        idx_t transpose_j0 = p_transpose_idx(j0, nthreads, nslices);
        idx_t transpose_j1 = p_transpose_idx(j1, nthreads, nslices);

        histogram_array[transpose_j1] += histogram_array[transpose_j0];
      }
    }
    #pragma omp barrier

    if (tid > 0) {
      idx_t transpose_j0 = p_transpose_idx(nslices*tid - 1, nthreads, nslices);

      for(idx_t j = tid*nslices; j < (tid+1) * nslices - 1; ++j) {

        idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);

        histogram_array[transpose_j] += histogram_array[transpose_j0];
      }
    }

    #pragma omp barrier


    /* now copy values into new structures (but not the mode we are sorting */
    for(idx_t j_off = 0; j_off < (jend-jbegin); ++j_off) {
      /* we are actually going backwards */
      idx_t const j = jend - j_off - 1;

      idx_t idx = tt->ind[m][j];
      --histogram[idx];

      idx_t offset = histogram[idx];

      new_vals[offset] = tt->vals[j];
      for(idx_t mode=0; mode < tt->nmodes; ++mode) {
        if(mode != m) {
          new_ind[mode][offset] = tt->ind[mode][j];
        }
      }
    }
  } /* omp parallel */

  for(idx_t i = 0; i < tt->nmodes; ++i) {
    if(i != m) {
      splatt_free(tt->ind[i]);
      tt->ind[i] = new_ind[i];
    }
  }
  splatt_free(tt->vals);
  tt->vals = new_vals;


  histogram_array[nslices] = tt->nnz;

  /* for 3/4D, we can use quicksort on only the leftover modes */
  if(tt->nmodes == 3) {
    #pragma omp parallel for schedule(dynamic)
    for(idx_t i = 0; i < nslices; ++i) {
      p_tt_quicksort2(tt, cmplt+1, histogram_array[i], histogram_array[i + 1]);
      for(idx_t j = histogram_array[i]; j < histogram_array[i + 1]; ++j) {
        tt->ind[m][j] = i;
      }
    }

  } else if(tt->nmodes == 4) {
    #pragma omp parallel for schedule(dynamic)
    for(idx_t i = 0; i < nslices; ++i) {
      p_tt_quicksort3(tt, cmplt+1, histogram_array[i], histogram_array[i + 1]);
      for(idx_t j = histogram_array[i]; j < histogram_array[i + 1]; ++j) {
        tt->ind[m][j] = i;
      }
    }

  } else {
    /* shift cmplt left one time, then do normal quicksort */
    idx_t saved = cmplt[0];
    memmove(cmplt, cmplt+1, (tt->nmodes - 1) * sizeof(*cmplt));
    cmplt[tt->nmodes-1] = saved;

    #pragma omp parallel for schedule(dynamic)
    for(idx_t i = 0; i < nslices; ++i) {
      p_tt_quicksort(tt, cmplt, histogram_array[i], histogram_array[i + 1]);
      for(idx_t j = histogram_array[i]; j < histogram_array[i + 1]; ++j) {
        tt->ind[m][j] = i;
      }
    }

    /* undo cmplt changes */
    saved = cmplt[tt->nmodes-1];
    memmove(cmplt+1, cmplt, (tt->nmodes - 1) * sizeof(*cmplt));
    cmplt[0] = saved;
  }

  splatt_free(histogram_array);
}

/**
* @brief Perform a counting sort on the most significant mode (cmplt[0]) and
*        then parallel quicksorts on each of slices.
*���Ͱ��λ��ֻ��һλ�Ŀ�������
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param bucket_size The number of top buckets that are already sorted.
*/
static void p_quicksort_bottom(
    sptensor_t * const tt,
    idx_t * const cmplt,
    idx_t bucket_size)
{
    idx_t * pos = splatt_malloc((tt->nnz + 1)* sizeof(*pos));//Ͱ����λ��
    idx_t *pos2 = splatt_malloc((tt->nnz + 1) * sizeof(*pos2));
    idx_t * totals = splatt_malloc(splatt_omp_get_max_threads() * sizeof(*totals));
    memset(totals, 0, sizeof(splatt_omp_get_max_threads() * (*totals)));

  #pragma omp parallel
  {
    int nthreads = splatt_omp_get_num_threads();
    int tid = splatt_omp_get_thread_num();

    idx_t j_per_thread = (tt->nnz + nthreads - 1)/nthreads;
    idx_t jbegin = SS_MIN(j_per_thread*tid, tt->nnz);
    idx_t jend = SS_MIN(jbegin + j_per_thread, tt->nnz);

    // Save the buckets
    idx_t curr = jbegin;//ȷ��ÿ���̵߳�Ͱ���໥����
    for(idx_t j = jbegin; j < jend; j ++){
        if (j == 0){
            pos[0] = 0;
            curr++;
            continue;
        }

        idx_t diff = 0;
        for(idx_t idx = 0; idx < bucket_size; idx++) {
            idx_t mi = cmplt[idx];
            if(tt->ind[mi][j] != tt->ind[mi][j-1]){
                diff ++;
                break;
            }
        }
        if(diff > 0){
            // Put the start of a bucket into the position array.
            pos[curr] = j; 
            curr++;
        }
    }

    if(tid == nthreads - 1){
        pos[curr] = tt->nnz;
        curr ++;
    }

    totals[tid] = curr - jbegin;//ÿ���߳���Ͱ����Ŀ

    #pragma omp barrier

    idx_t prefix = 0; 
    for(idx_t i = 0; i < tid; i ++){
        prefix += totals[i];
    }

    memcpy(&pos2[prefix], &pos[jbegin], sizeof(*pos2)*(totals[tid]));//��pos�п��ܲ���˳���Ͱ�����ϳ�˳��pos2�з�Ͱ���׵�ַ����Сֻ��ҪͰ����

  } /* omp parallel */

  idx_t size = 0;
  for(idx_t i = 0; i < splatt_omp_get_max_threads(); i ++){
      size += totals[i];
  }

    /* shift cmplt left one time, then do normal quicksort */
    for(int i = 0; i < bucket_size; i ++){
        idx_t saved = cmplt[0];
        memmove(cmplt, cmplt+1, (tt->nmodes - 1) * sizeof(*cmplt));//��Ͱ��ʱ�Ƴ�����ʼ���к���Ŀ�������
        cmplt[tt->nmodes-1] = saved;//Ͱ��ά�ȷź��棬��ΪͰ�ڵ�Ͱ��һ�£���Ӱ��Ƚ�
    }
	//guided������һ�ֲ���ָ���Ե�����ʽ�Ե��ȷ�����
	//��ʼʱÿ���̻߳���䵽�ϴ�ĵ����飬֮����䵽�ĵ�������𽥵ݼ���
	//������Ĵ�С�ᰴָ�����½���ָ����size��С�����û��ָ��size��������ô�������С��С�ή��1
    #pragma omp parallel for schedule(guided)
    for(idx_t i = 0; i < size-1; i ++){
        p_tt_quicksort(tt, cmplt, pos2[i], pos2[i + 1]);
    }

    /* undo cmplt changes */
    for(int i = 0; i < bucket_size; i ++){
        idx_t saved = cmplt[tt->nmodes-1];
        memmove(cmplt+1, cmplt, (tt->nmodes - 1) * sizeof(*cmplt));
        cmplt[0] = saved;
    }

    splatt_free(pos);
    splatt_free(pos2);  
    splatt_free(totals);
}

/**
* @brief Perform a stable counting sort on mode m.
*
* @param tt The tensor to sort.
* @param m Mode to be sorted on
*/
static void p_counting_sort(
    sptensor_t * const tt,
    idx_t m)
{
  idx_t nslices = tt->dims[m];

  idx_t * new_ind[MAX_NMODES];
  for(idx_t i = 0; i < tt->nmodes; ++i) {
      new_ind[i] = splatt_malloc(tt->nnz * sizeof(**new_ind));
  }
  val_t * new_vals = splatt_malloc(tt->nnz * sizeof(*new_vals));
  idx_t * histogram_array = splatt_malloc(
      (nslices * splatt_omp_get_max_threads() + 1) * sizeof(*histogram_array));//ȷ�����ÿ�����̶���Ψһ˳��ģ��ܹ��洢��

  #pragma omp parallel
  {
    int nthreads = splatt_omp_get_num_threads();
    int tid = splatt_omp_get_thread_num();

    idx_t * histogram = histogram_array + (nslices * tid);
    memset(histogram, 0, nslices * sizeof(idx_t));

    idx_t j_per_thread = (tt->nnz + nthreads - 1)/nthreads;
    idx_t jbegin = SS_MIN(j_per_thread*tid, tt->nnz);
    idx_t jend = SS_MIN(jbegin + j_per_thread, tt->nnz);

    /* count */
    for(idx_t j = jbegin; j < jend; ++j) {
      idx_t idx = tt->ind[m][j];
      ++histogram[idx];
    }

    #pragma omp barrier

    /* prefix sum */
	//��ÿ�����̵ĵ��������ϵĴ�С�����ۼӣ�����ٴ洢��һ�����������еļ����ϵ��ܺͣ����ۼӵ�ĩβ��ַ��ÿ���̸߳�����Եļ�����С
    for(idx_t j = (tid*nslices) + 1; j < (tid+1) * nslices; ++j) {
      idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);
      idx_t transpose_j_minus_1 = p_transpose_idx(j - 1, nthreads, nslices);

      histogram_array[transpose_j] += histogram_array[transpose_j_minus_1];
    }

    #pragma omp barrier
    #pragma omp master
    {//ÿ���̵߳�ĩβ�ۼӣ�����ÿ����ĩβ��������
      for(int t = 1; t < nthreads; ++t) {
        idx_t j0 = (nslices*t) - 1, j1 = nslices * (t+1) - 1;
        idx_t transpose_j0 = p_transpose_idx(j0, nthreads, nslices);
        idx_t transpose_j1 = p_transpose_idx(j1, nthreads, nslices);

        histogram_array[transpose_j1] += histogram_array[transpose_j0];
      }
    }
    #pragma omp barrier

    if (tid > 0) {//��ÿ�����̵���λλ�����ӣ���֤˳��
      idx_t transpose_j0 = p_transpose_idx(nslices*tid - 1, nthreads, nslices);

      for(idx_t j = tid*nslices; j < (tid+1) * nslices - 1; ++j) {

        idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);

        histogram_array[transpose_j] += histogram_array[transpose_j0];
      }
    }

    #pragma omp barrier


    /* now copy values into new structures */
	//�Ӻ���ǰ�İ������ݣ�histogram��ĩβ����
    for(idx_t j_off = 0; j_off < (jend-jbegin); ++j_off) {
      /* we are actually going backwards */
      idx_t const j = jend - j_off - 1;

      idx_t idx = tt->ind[m][j];
      --histogram[idx];

      idx_t offset = histogram[idx];

      new_vals[offset] = tt->vals[j];
      for(idx_t mode=0; mode < tt->nmodes; ++mode) {
          new_ind[mode][offset] = tt->ind[mode][j];
      }
    }
  } /* omp parallel */


  for(idx_t i = 0; i < tt->nmodes; ++i) {
      splatt_free(tt->ind[i]);
      tt->ind[i] = new_ind[i];
  }
  splatt_free(tt->vals);
  tt->vals = new_vals;

  splatt_free(histogram_array);
}

/**
* @brief Perform a single bucket counting sort pass.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param bucket_size The top 'bucket_size' modes in cmplt create buckets to sort within.
* @param m The mode to be sorted on.
*/
static void p_bucket_counting_sort_old(
    sptensor_t * const tt,
    idx_t * const cmplt,
    idx_t bucket_size,
    idx_t m
    )
{
    idx_t nslices = tt->dims[m];
    idx_t * secret_ind = splatt_malloc(tt->nnz * sizeof(**tt->ind));
    secret_ind[0] = 1;
    idx_t * new_ind[MAX_NMODES];
    for(idx_t i = 0; i < tt->nmodes + 1; ++i) {
        new_ind[i] = splatt_malloc(tt->nnz * sizeof(**new_ind));
    }
    val_t * new_vals = splatt_malloc(tt->nnz * sizeof(*new_vals));


     idx_t * histogram_array = splatt_malloc(
      (nslices * splatt_omp_get_max_threads() + 1) * sizeof(*histogram_array));

    idx_t *histogram_array2 = splatt_malloc(
      (tt->nnz * splatt_omp_get_max_threads() + 1) * sizeof(*histogram_array2));

    idx_t *totals = splatt_malloc(splatt_omp_get_max_threads() * sizeof(*totals));
    memset(totals, 0, splatt_omp_get_max_threads() * sizeof(*totals));


  #pragma omp parallel
  {
    int nthreads = splatt_omp_get_num_threads();
    int tid = splatt_omp_get_thread_num();


    idx_t * histogram = histogram_array + (nslices * tid);
    memset(histogram, 0, nslices * sizeof(idx_t));

    idx_t j_per_thread = (tt->nnz + nthreads - 1)/nthreads;
    idx_t jbegin = SS_MIN(j_per_thread*tid, tt->nnz);
    idx_t jend = SS_MIN(jbegin + j_per_thread, tt->nnz);

    // Save the buckets
    for(idx_t j = jbegin; j < jend; j ++){
        if (j == 0){
            totals[tid] ++;
            secret_ind[j] = 1;
            continue;
        }

        idx_t diff = 0;
        for(idx_t idx = 0; idx < bucket_size; idx++) {
            idx_t mi = cmplt[idx];
            if(tt->ind[mi][j] != tt->ind[mi][j-1]){
                diff ++;
                break;
            }
        }
        if(diff > 0){
            totals[tid]++;
            secret_ind[j] = 1;
        }else{
             secret_ind[j] = 0;
        }
    }

    #pragma omp barrier

    // we want to finish the prefix sum in parallel

    /* prefix sum */
    // Sum the previous totals
    idx_t previous = 0;
    for(int t = 0; t < tid; t ++){
        previous += totals[t];
    }
    secret_ind[jbegin] += previous; 
    for(idx_t j = jbegin + 1; j < jend; ++j) {
        secret_ind[j] += secret_ind[j-1];
    }


    #pragma omp barrier

    idx_t total_buckets = previous;
    for(idx_t i = tid; i < nthreads; i ++){
        total_buckets += totals[i];
    }


    /* count */
    for(idx_t j = jbegin; j < jend; ++j) {
      idx_t idx = tt->ind[m][j];
      ++histogram[idx];
    }

    #pragma omp barrier

    /* prefix sum */
    for(idx_t j = (tid*nslices) + 1; j < (tid+1) * nslices; ++j) {
      idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);
      idx_t transpose_j_minus_1 = p_transpose_idx(j - 1, nthreads, nslices);

      histogram_array[transpose_j] += histogram_array[transpose_j_minus_1];
    }

    #pragma omp barrier
    #pragma omp master
    {
      for(int t = 1; t < nthreads; ++t) {
        idx_t j0 = (nslices*t) - 1, j1 = nslices * (t+1) - 1;
        idx_t transpose_j0 = p_transpose_idx(j0, nthreads, nslices);
        idx_t transpose_j1 = p_transpose_idx(j1, nthreads, nslices);

        histogram_array[transpose_j1] += histogram_array[transpose_j0];
      }
    }
    #pragma omp barrier

    if (tid > 0) {
      idx_t transpose_j0 = p_transpose_idx(nslices*tid - 1, nthreads, nslices);

      for(idx_t j = tid*nslices; j < (tid+1) * nslices - 1; ++j) {

        idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);

        histogram_array[transpose_j] += histogram_array[transpose_j0];
      }
    }

    #pragma omp barrier

    /* now copy values into new structures (but not the mode we are sorting */
    for(idx_t j_off = 0; j_off < (jend-jbegin); ++j_off) {
      /* we are actually going backwards */
      idx_t const j = jend - j_off - 1;

      idx_t idx = tt->ind[m][j];
      --histogram[idx];

      idx_t offset = histogram[idx];

      new_vals[offset] = tt->vals[j];
      for(idx_t mode=0; mode < tt->nmodes; ++mode) {
            new_ind[mode][offset] = tt->ind[mode][j];
      }
      new_ind[tt->nmodes][offset] = secret_ind[j];
    }

    #pragma omp barrier

    // Move back with histogram index.
    nslices = total_buckets + 1;
    m = tt->nmodes;

    idx_t * histogram2 = histogram_array2 + (nslices * tid);
    memset(histogram2, 0, nslices * sizeof(idx_t));

    /* count */
    for(idx_t j = jbegin; j < jend; ++j) {
      idx_t idx = new_ind[m][j];
      ++histogram2[idx];
    }

    #pragma omp barrier

    /* prefix sum */
    for(idx_t j = (tid*nslices) + 1; j < (tid+1) * nslices; ++j) {
      idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);
      idx_t transpose_j_minus_1 = p_transpose_idx(j - 1, nthreads, nslices);

      histogram_array2[transpose_j] += histogram_array2[transpose_j_minus_1];
    }

    #pragma omp barrier
    #pragma omp master
    {
      for(int t = 1; t < nthreads; ++t) {
        idx_t j0 = (nslices*t) - 1, j1 = nslices * (t+1) - 1;
        idx_t transpose_j0 = p_transpose_idx(j0, nthreads, nslices);
        idx_t transpose_j1 = p_transpose_idx(j1, nthreads, nslices);

        histogram_array2[transpose_j1] += histogram_array2[transpose_j0];
      }
    }
    #pragma omp barrier

    if (tid > 0) {
      idx_t transpose_j0 = p_transpose_idx(nslices*tid - 1, nthreads, nslices);

      for(idx_t j = tid*nslices; j < (tid+1) * nslices - 1; ++j) {

        idx_t transpose_j = p_transpose_idx(j, nthreads, nslices);

        histogram_array2[transpose_j] += histogram_array2[transpose_j0];
      }
    }

    #pragma omp barrier


    /* now copy values into new structures (but not the mode we are sorting */
    for(idx_t j_off = 0; j_off < (jend-jbegin); ++j_off) {
      /* we are actually going backwards */
      idx_t const j = jend - j_off - 1;

      idx_t idx = new_ind[m][j];
      --histogram2[idx];

      idx_t offset = histogram2[idx];
      tt->vals[offset] = new_vals[j];
      for(idx_t mode=0; mode < tt->nmodes; ++mode) {
          tt->ind[mode][offset] = new_ind[mode][j];
      }
    }

  } /* omp parallel */

  
    for(idx_t i = 0; i < tt->nmodes + 1; ++i) {
        splatt_free(new_ind[i]);
    }
    splatt_free(new_vals);

    splatt_free(secret_ind);


    splatt_free(histogram_array);
    splatt_free(histogram_array2);
    splatt_free(totals);

}

/**
* @brief Perform a single bucket counting sort pass.
*
* @param tt The tensor to sort.
* @param cmplt Mode permutation used for defining tie-breaking order.
* @param bucket_size The top 'bucket_size' modes in cmplt create buckets to sort within.
* @param m The mode to be sorted on.
*/
static void p_bucket_counting_sort(
    sptensor_t * const tt,
    idx_t * const cmplt,
    idx_t bucket_size,
    idx_t m
    )
{
    idx_t nslices = tt->dims[m];
    idx_t * secret_ind = splatt_malloc(tt->nnz * sizeof(**tt->ind));

    idx_t * new_ind[MAX_NMODES];
    for(idx_t i = 0; i < tt->nmodes + 1; ++i) {
        new_ind[i] = splatt_malloc(tt->nnz * sizeof(**new_ind));
    }
    val_t * new_vals = splatt_malloc(tt->nnz * sizeof(*new_vals));

    idx_t * histogram_array = splatt_malloc(
      ((nslices + 1)* splatt_omp_get_max_threads() + 1) * sizeof(*histogram_array));

    // Need to allocate for an additional max_threads for imperfect balancing. 
    idx_t *histogram_array2 = splatt_malloc(
      (tt->nnz + 2*splatt_omp_get_max_threads() + 1) * sizeof(*histogram_array2));

  #pragma omp parallel
  {
    int nthreads = splatt_omp_get_num_threads();
    int tid = splatt_omp_get_thread_num();


    idx_t * histogram = histogram_array + ((nslices + 1) * tid);
    memset(histogram, 0, ((nslices + 1) * sizeof(idx_t)));


    idx_t j_per_thread = (tt->nnz + nthreads - 1)/nthreads;
    idx_t jbegin = SS_MIN(j_per_thread*tid, tt->nnz);
    idx_t jend = SS_MIN(jbegin + j_per_thread, tt->nnz);

        idx_t * histogram2 = histogram_array2 + ((j_per_thread + 1) * tid);
        memset(histogram2, 0, (j_per_thread + 1) * sizeof(idx_t));


    idx_t curr = 0;
    int start = -1;

    // Save the buckets
    for(idx_t j = jbegin; j < jend; j ++){
        if (j == 0){
            secret_ind[j] = curr;
            start = 0;  
            histogram2[0] = 0;
            continue;
        }

        idx_t diff = 0;
        for(idx_t idx = 0; idx < bucket_size; idx++) {
            idx_t mi = cmplt[idx];
            if(tt->ind[mi][j] != tt->ind[mi][j-1]){
                diff ++;
                break;
            }
        }
        if(diff > 0){
            if(start < 0){
                start = j; // Start at the first new bucket
                histogram2[curr] = j;
            }else {
                curr++;
                histogram2[curr] = j;
            }
        }
        if(start >= 0){
            secret_ind[j] = curr;//ÿ�����ݵ�Ͱ��
        }
    }
    #pragma omp barrier
        
    idx_t j = jend;

    if(start >= 0){
        for(j = jend; j < tt->nnz; j++){
            idx_t diff = 0;
            for(idx_t idx = 0; idx < bucket_size; idx++) {
                idx_t mi = cmplt[idx];
                if(tt->ind[mi][j] != tt->ind[mi][j-1]){
                    diff ++;
                    break;
                }
            }
            if(diff > 0){
                break;        
            }
            secret_ind[j] = curr;
        }
        histogram2[curr + 1] = j;
    }
    idx_t end = j;

    // Reassign.
    jbegin = start;
    jend = end;
	//���������̼߳������������ͬ��Ͱ�����ڶ���������,ǰ���Ͱֻ��Ҫ��������ǰ��ģ�����ͺ���Ƚ�
    if(start >= 0){
        /* count */
        for(idx_t j = jbegin; j < jend; ++j) {
            idx_t idx = tt->ind[m][j];
            ++histogram[idx + 1];
        }

        /* prefix sum */
        for(idx_t idx = 1; idx < nslices + 1; ++idx) {
            histogram[idx] += histogram[idx-1];
        }

        /* now copy values into new structures*/
        for(idx_t j = jbegin; j < jend; ++j){
            idx_t idx = tt->ind[m][j];

            idx_t offset = histogram[idx] + jbegin;

            new_vals[offset] = tt->vals[j];

            for(idx_t mode=0; mode < tt->nmodes; ++mode) {
                    new_ind[mode][offset] = tt->ind[mode][j];
            }
            new_ind[tt->nmodes][offset] = secret_ind[j];
            ++histogram[idx];
        }

    }
    #pragma omp barrier
    if(start >= 0){
        // Move back with histogram index.
        m = tt->nmodes;
        

        for(idx_t j = jbegin; j < jend; ++j){
            idx_t idx = new_ind[m][j];
            idx_t offset = histogram2[idx];

            tt->vals[offset] = new_vals[j];
            for(idx_t mode=0; mode < tt->nmodes; ++mode) {
                tt->ind[mode][offset] = new_ind[mode][j];
            }
            ++histogram2[idx];
        }
    }
  } /* omp parallel */  

    for(idx_t i = 0; i < tt->nmodes + 1; ++i) {
        splatt_free(new_ind[i]);
    }

    splatt_free(new_vals);
    splatt_free(secret_ind);
    splatt_free(histogram_array);
    splatt_free(histogram_array2);

}



/******************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/
void tt_sort(
  sptensor_t * const tt,
  idx_t const mode,
  idx_t * dim_perm)
{
  tt_sort_range(tt, mode, dim_perm, 0, tt->nnz);
}


void tt_sort_range(
  sptensor_t * const tt,
  idx_t const mode,
  idx_t * dim_perm,
  idx_t const start,
  idx_t const end)
{
    idx_t * cmplt;
    if(dim_perm == NULL) {
        cmplt = (idx_t*) splatt_malloc(tt->nmodes * sizeof(idx_t));
        cmplt[0] = mode;
        for(idx_t m=1; m < tt->nmodes; ++m) {
            cmplt[m] = (mode + m) % tt->nmodes;
        }
    } else {
        cmplt = dim_perm;
    }

    timer_start(&timers[TIMER_SORT]);

    if(start == 0 && end == tt->nnz) {
        p_counting_sort_hybrid(tt, cmplt);
    /* sort a subtensor */
    } else {
        switch(tt->type) {
        case SPLATT_NMODE:
        p_tt_quicksort(tt, cmplt, start, end);
        break;

        case SPLATT_3MODE:
        p_tt_quicksort3(tt, cmplt, start, end);
        break;
        }
    }

  if(dim_perm == NULL) {
    free(cmplt);
  }
  timer_stop(&timers[TIMER_SORT]);
}

void tt_sort_ksadilla(
  sptensor_t * const tt,
  idx_t const mode,
  idx_t * dim_perm,
  enum SortType typ,
  idx_t k)
{
    switch(typ){
        case Splatt:
            tt_sort_range(tt, mode, dim_perm, 0, tt->nnz);
            break;
        case KSadilla:
            tt_sort_range_ksadilla(tt, mode, dim_perm, 0, tt->nnz, k);
            break;
        case Radix:
            tt_sort_range_radix(tt, mode, dim_perm, 0, tt->nnz);
            break;
    }
}


void tt_sort_range_ksadilla(//tt�Ƕ���,nmodes������ά��ֵ��dim_perm��������˳��k���߳�����mode�������������ά��ֵ
    sptensor_t * const tt,
    idx_t const mode,
    idx_t * dim_perm,
    idx_t const start,
    idx_t const end,
    idx_t k)
{
    idx_t * cmplt;
    if(dim_perm == NULL) {
        cmplt = (idx_t*) splatt_malloc(tt->nmodes * sizeof(idx_t));
        cmplt[0] = mode;
        for(idx_t m=1; m < tt->nmodes; ++m) {
        cmplt[m] = (mode + m) % tt->nmodes;
        }
    } else {
        cmplt = dim_perm;
    }

    timer_start(&timers[TIMER_SORT]);
    if(k == 0){ // 0 is just a quicksort
        p_tt_quicksort(tt, cmplt, start, end);
    }
    else { // Somewhere in the middle
        if(tt->nmodes == 3){
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2){
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2){
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1){
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
        }
        if(tt->nmodes == 4){
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3){
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
        }
        if(tt->nmodes == 5){
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
            }
            if(k == 1 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 0 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_bucket_counting_sort(tt, cmplt, 1, 2);
                p_bucket_counting_sort(tt, cmplt, 1, 3);
                p_bucket_counting_sort(tt, cmplt, 1, 4);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 1 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 3 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 2 && cmplt[1] == 4 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 0 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 4);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 1 && cmplt[2] == 4 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 4 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 4);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 4){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 4 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 2 && cmplt[2] == 4 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_bucket_counting_sort(tt, cmplt, 3, 2);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 3 && cmplt[1] == 4 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_counting_sort(tt, 3);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 0 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 2, 2);
                p_bucket_counting_sort(tt, cmplt, 2, 3);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 2 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 1 && cmplt[2] == 3 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 0 && cmplt[3] == 3 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 3);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 3){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 1 && cmplt[3] == 3 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 2 && cmplt[2] == 3 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 1 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 2);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 0 && cmplt[3] == 2 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_bucket_counting_sort(tt, cmplt, 3, 2);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 0 && cmplt[4] == 2){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 1 && cmplt[3] == 2 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 0 && cmplt[4] == 1){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
            }
            if(k == 1 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 2 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 3 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 4 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
                p_quicksort_bottom(tt, cmplt, k);
            }
            if(k == 5 && cmplt[0] == 4 && cmplt[1] == 3 && cmplt[2] == 2 && cmplt[3] == 1 && cmplt[4] == 0){
                p_counting_sort(tt, 1);
                p_counting_sort(tt, 2);
                p_counting_sort(tt, 3);
                p_counting_sort(tt, 4);
            }
        }

    }

  if(dim_perm == NULL) {
    free(cmplt);
  }
  timer_stop(&timers[TIMER_SORT]);
}

void tt_sort_range_radix(
  sptensor_t * const tt,
  idx_t const mode,
  idx_t * dim_perm,
  idx_t const start,
  idx_t const end)
{
    idx_t * cmplt;
    if(dim_perm == NULL) {
        cmplt = (idx_t*) splatt_malloc(tt->nmodes * sizeof(idx_t));
        cmplt[0] = mode;
        for(idx_t m=1; m < tt->nmodes; ++m) {
        cmplt[m] = (mode + m) % tt->nmodes;
        }
    } else {
        cmplt = dim_perm;
    }

    timer_start(&timers[TIMER_SORT]);

    for(int j = tt->nmodes - 1; j >= 0; j--){
        p_counting_sort(tt, cmplt[j]);
    }

    if(dim_perm == NULL) {
        free(cmplt);
    }
    timer_stop(&timers[TIMER_SORT]);
}



void insertion_sort(
  idx_t * const a,
  idx_t const n)
{
  timer_start(&timers[TIMER_SORT]);
  for(size_t i=1; i < n; ++i) {
    idx_t b = a[i];
    size_t j = i;
    while (j > 0 &&  a[j-1] > b) {
      --j;
    }
    memmove(a+(j+1), a+j, sizeof(*a)*(i-j));
    a[j] = b;
  }
  timer_stop(&timers[TIMER_SORT]);
}


void quicksort(
  idx_t * const a,
  idx_t const n)
{
  timer_start(&timers[TIMER_SORT]);
  p_quicksort(a,n);
  timer_stop(&timers[TIMER_SORT]);
}



void insertion_sort_perm(
  idx_t * const restrict a,
  idx_t * const restrict perm,
  idx_t const n)
{
  timer_start(&timers[TIMER_SORT]);
  for(size_t i=1; i < n; ++i) {
    idx_t b = a[i];
    idx_t pb = perm[i];

    size_t j = i;
    while (j > 0 &&  a[j-1] > b) {
      --j;
    }
    memmove(a+(j+1), a+j, sizeof(*a)*(i-j));
    a[j] = b;

    memmove(perm+(j+1), perm+j, sizeof(*perm)*(i-j));
    perm[j] = pb;
  }
  timer_stop(&timers[TIMER_SORT]);
}


void quicksort_perm(
  idx_t * const restrict a,
  idx_t * const restrict perm,
  idx_t const n)
{
  timer_start(&timers[TIMER_SORT]);
  /* initialize permutation ��ʼ������*/
  for(idx_t i=0; i < n; ++i) {
    perm[i] = i;
  }

  p_quicksort_perm(a, perm, n);
  timer_stop(&timers[TIMER_SORT]);
}



