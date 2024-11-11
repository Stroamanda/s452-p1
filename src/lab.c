#include "lab.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
/**
 * @brief Standard insertion sort that is faster than merge sort for small array's
 *
 * @param A The array to sort
 * @param p The starting index
 * @param r The ending index
 */
static void insertion_sort(int A[], int p, int r) {
  int j;

  for (j = p + 1; j <= r; j++)
    {
      int key = A[j];
      int i = j - 1;
      while ((i > p - 1) && (A[i] > key))
        {
	  A[i + 1] = A[i];
	  i--;
        }
      A[i + 1] = key;
    }
}


void mergesort_s(int A[], int p, int r) {
  if (r - p + 1 <=  INSERTION_SORT_THRESHOLD)
    {
      insertion_sort(A, p, r);
    }
  else
    {
      int q = (p + r) / 2;
      mergesort_s(A, p, q);
      mergesort_s(A, q + 1, r);
      merge_s(A, p, q, r);
    }

}

void merge_s(int A[], int p, int q, int r) {
  int *B = (int *)malloc(sizeof(int) * (r - p + 1));

  int i = p;
  int j = q + 1;
  int k = 0;
  int l;

  /* as long as both lists have unexamined elements */
  /*  this loop keeps executing. */
  while ((i <= q) && (j <= r)) {
      if (A[i] < A[j])
        {
	  B[k] = A[i];
	  i++;
        }
      else
        {
	  B[k] = A[j];
	  j++;
        }
      k++;
    }

  /* now only at most one list has unprocessed elements. */
  if (i <= q) {
      /* copy remaining elements from the first list */
      for (l = i; l <= q; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }
  else {
      /* copy remaining elements from the second list */
      for (l = j; l <= r; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }

  /* copy merged output from array B back to array A */
  k = 0;
  for (l = p; l <= r; l++) {
      A[l] = B[k];
      k++;
    }

  free(B);
}

  /**
   * @brief Sorts an array of ints into ascending order using multiple
   * threads
   *
   * @param A A pointer to the start of the array
   * @param n The size of the array
   * @param num_threads The number of threads to use.
   */
void mergesort_mt(int *A, int n, int num_thread) {
    pthread_t threads[MAX_THREADS];
    struct parallel_args args[num_thread];

    // Get the number that the array will be split on
    int splitSize = n / num_thread;

    for (int i = 0; i < num_thread; i++) {
        args[i].A = A;
        args[i].start = i * splitSize;

        // if last thread then end at the last value index in the array
        // otherwise the end is the starting index plus the number being split on
        if (i == num_thread - 1) {
            args[i].end = n - 1;
        } else {
            args[i].end = (args[i].start + splitSize) - 1;
        }

        // create thread based on thread index
        pthread_create(&threads[i], NULL, parallel_mergesort, &args[i]);
    }

    // join together all the threads
    for (int i = 0; i < num_thread; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Have 0 be the starting point, then merge based on the
    // end of the current section with the end of the next section.
    // Once these sections are sorted, they are formed into a "new"
    // sorted section, and then retrieve the end index of the next sorted
    // section as the next end section.
    int start = 0;
    int end1 = splitSize - 1;
    int end2 = (2 * splitSize) - 1;
    for (int i = 0; i < num_thread; i++) {
      if (end2 >= n) {
          end2 = n - 1;
      }
      merge_s(A, start, end1, end2);
      end1 = end2;
      end2 = (end2 + 1 + splitSize) - 1;
    }

}

  /**
   * @brief The function that is called by each thread to sort their chunk
   *
   * @param args see struct parallel_args
   * @return void* always NULL
   */
void *parallel_mergesort(void *args) {
    struct parallel_args *pargs = (struct parallel_args *)args;

    if (pargs->end - pargs->start + 1 <=  INSERTION_SORT_THRESHOLD) {
        insertion_sort(pargs->A, pargs->start, pargs->end);
    } else {
        int q = (pargs->start + pargs->end) / 2;
        struct parallel_args leftArgs = {pargs->A, pargs->start, q, 0};
        struct parallel_args rightArgs = {pargs->A, q + 1, pargs->end, 0};

        parallel_mergesort(&leftArgs);
        parallel_mergesort(&rightArgs);
        merge_s(pargs->A, pargs->start, q, pargs->end);
    }

    return NULL;
}

double getMilliSeconds() {
  struct timeval now;
  gettimeofday(&now, (struct timezone *)0);
  return (double)now.tv_sec * 1000.0 + now.tv_usec / 1000.0;
}