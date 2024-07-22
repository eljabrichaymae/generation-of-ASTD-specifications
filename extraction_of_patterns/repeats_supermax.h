
#ifndef _REPEATS_SUPERMAX_H_
#define _REPEATS_SUPERMAX_H_

typedef struct supermax_node {
  int *S;
  int M, num_witness, num_leaves, percent;
  struct supermax_node *next;
} STRUCT_SUPERMAX, *SUPERMAXIMALS;

SUPERMAXIMALS supermax_find(int *S, int M, int min_percent, int min_length);

#endif
