#include <iostream>
#include "taco.h"
#include "taco/coo_transpose.h"
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


using namespace taco;
int cmp_012(const void *p, const void *q)
{
    struct coo_t * r = (struct coo_t *)p;
    struct coo_t * s = (struct coo_t *)q;
    if(r->idx0 < s->idx0) {
        return -1;
    } else if (r->idx0 > s->idx0) {
        return 1;
    }
    if(r->idx1 < s->idx1) {
        return -1;
    }else if(r->idx1 > s->idx1) {
        return 1;
    }
        if(r->idx2 < s->idx2) {
        return -1;
    }else if(r->idx2 > s->idx2) {
        return 1;
    }
    return 0;

}
int cmp_021(const void *p, const void *q)
{
    struct coo_t * r = (struct coo_t *)p;
    struct coo_t * s = (struct coo_t *)q;
if(r->idx0 < s->idx0) {
        return -1;
    } else if (r->idx0 > s->idx0) {
        return 1;
    }
     if(r->idx2 < s->idx2) {
        return -1;
    }else if(r->idx2 > s->idx2) {
        return 1;
    }

    if(r->idx1 < s->idx1) {
        return -1;
    }else if(r->idx1 > s->idx1) {
        return 1;
    }
    
    
       
    return 0;

}


int cmp_102(const void *p, const void *q)
{
    struct coo_t * r = (struct coo_t *)p;
    struct coo_t * s = (struct coo_t *)q;
    if(r->idx1 < s->idx1) {
        return -1;
    }else if(r->idx1 > s->idx1) {
        return 1;
    }
    if(r->idx0 < s->idx0) {
        return -1;
    } else if (r->idx0 > s->idx0) {
        return 1;
    }
    
        if(r->idx2 < s->idx2) {
        return -1;
    }else if(r->idx2 > s->idx2) {
        return 1;
    }
    return 0;

}

int cmp_210(const void *p, const void *q)
{
    struct coo_t * r = (struct coo_t *)p;
    struct coo_t * s = (struct coo_t *)q;
        if(r->idx2 < s->idx2) {
        return -1;
    }else if(r->idx2 > s->idx2) {
        return 1;
    }
    if(r->idx1 < s->idx1) {
        return -1;
    }else if(r->idx1 > s->idx1) {
        return 1;
    }
    if(r->idx0 < s->idx0) {
        return -1;
    } else if (r->idx0 > s->idx0) {
        return 1;
    }
    
    
    return 0;

}



int main(int argc, char* argv[]) {
  std::string filename = "~/research/benchmark/facebook.tns";
  // Read tensor data  
  int order = 3;
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
      if(size >= A_capacity){
          int32_t A_capacity_new = 2 * (A_capacity);
          A = (struct coo_t*)realloc(A, sizeof(struct coo_t) * A_capacity_new);
          A_capacity = A_capacity_new;
        }

    char* linePtr = (char*)line.data();

    // Generated by coo/gen_taco_to_list.py



    long idx0 = strtol(linePtr, &linePtr, 10);
    A[size].idx0 = (int32_t)idx0 - 1;
    dimensions[0] = std::max(dimensions[0], (int)idx0 - 1);
            long idx1 = strtol(linePtr, &linePtr, 10);
    A[size].idx1 = (int32_t)idx1 - 1;
    dimensions[1] = std::max(dimensions[1], (int)idx1 - 1);

            long idx2 = strtol(linePtr, &linePtr, 10);
    A[size].idx2 = (int32_t)idx2 - 1;
    dimensions[2] = std::max(dimensions[2], (int)idx2 - 1);





    double val = strtod(linePtr, &linePtr);
    A[size].val = val;
    size ++;

  } while (std::getline(stream, line));

  stream.close();
    
    cout << "size = " << size << endl;
    cout<< "[" << dimensions[0] << ", "<< dimensions[1] << ", "<< dimensions[2] << "]" << endl;

    // for(int i = 0; i < size; i ++){
    //     cout << i << ": " << A[i].idx0 << ", "<< A[i].idx1 << ", "<< A[i].idx2 << ", "<< A[i].val << endl;
    // }
	struct coo_t* A_scratch = (struct coo_t *)malloc(sizeof(struct coo_t) * size);

  // Create tensors

  
   // 021
          cout << "[0, 2, 1]" << endl;

    qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer5;                                   
  timer5.start();                                             
  taco::transpose_coo_021(A, size, order, dimensions);
  timer5.stop();                                              
  taco::util::TimeResults result5 = timer5.getResult();        
  cout << filename << " " << result5 << " ms" << endl;   
  timer5.clear_cache();

      qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer5t;                                   
  timer5t.start();                                             
  taco::transpose_coo_count_021_skip(A, size, order, dimensions);
  timer5t.stop();                                              
  taco::util::TimeResults result5t = timer5t.getResult();        
  cout << filename << " " << result5t << " ms" << endl;   
  timer5t.clear_cache();
  // 102
  cout << "[1, 0, 2]" << endl;
    qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer;                                   
  timer.start();                                             
  taco::transpose_coo_102(A, size, order, dimensions);
  timer.stop();                                              
  taco::util::TimeResults result = timer.getResult();        
  cout << filename << " " << result << " ms" << endl;   
  timer.clear_cache();

      qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timert;                                   
  timert.start();                                             
  taco::transpose_coo_count_102(A, size, order, dimensions);
  timert.stop();                                              
  result = timert.getResult();        
  cout << filename << " " << result << " ms" << endl;   
  timert.clear_cache();


    // reset
    qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer2;                                  
  timer2.start();    

//   taco::merge_102_sort(A, A_scratch, 0, size);                                     
qsort(A, size, sizeof(struct coo_t), cmp_102);
  timer2.stop();                                              
  result = timer2.getResult();        
  cout << filename << " " << result << " ms" << endl;   
  timer.clear_cache();

      // 210
        cout << "[2, 1, 0]" << endl;

    qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer3;                                   
  timer3.start();                                             
  taco::transpose_coo_210(A, size, order, dimensions);
  timer3.stop();                                              
  result = timer3.getResult();        
  cout << filename << " " << result << " ms" << endl;   
  timer.clear_cache();

      qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer3t;                                   
  timer3t.start();                                             
  taco::transpose_coo_count_210(A, size, order, dimensions);
  timer3t.stop();                                              
  result = timer3t.getResult();        
  cout << filename << " " << result << " ms" << endl;   
  timer.clear_cache();


    // reset
    qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer4;                                  
  timer4.start();   
    // taco::merge_210_sort(A, A_scratch, 0, size);                                     
                                          
    qsort(A, size, sizeof(struct coo_t), cmp_210);
  timer4.stop();                                              
  result = timer4.getResult();        
  cout << filename << " " << result << " ms" << endl;   
  timer4.clear_cache();

//     taco::util::Timer timer4t;                                  
//   timer4t.start();   
//     taco::merge_210_sort_opt(A, A_scratch, 0, size);                                     
                                          
//     // qsort(A, size, sizeof(struct coo_t), cmp_210);
//   timer4t.stop();                                              
//   result = timer4t.getResult();        
//   cout << filename << " " << result << " ms" << endl;   
//   timer4t.clear_cache();


  



    // for(int i = 0; i < size; i ++){
    //     cout << i << ": " << A[i].idx0 << ", "<< A[i].idx1 << ", "<< A[i].idx2 << ", "<< A[i].val << endl;
    // }

    // reset
    qsort(A, size, sizeof(struct coo_t), cmp_012);

  taco::util::Timer timer6;                                  
  timer6.start(); 
    // taco::merge_021_sort(A, A_scratch, 0, size);                                     
                                            
    qsort(A, size, sizeof(struct coo_t), cmp_021);
  timer6.stop();                                              
  result5 = timer6.getResult();        
  cout << filename << " " << result5 << " ms" << endl;   
  timer6.clear_cache();
 



    // for(int i = 0; i < size; i ++){
    //     cout << i << ": " << A[i].idx0 << ", "<< A[i].idx1 << ", "<< A[i].idx2 << ", "<< A[i].val << endl;
    // }


}
