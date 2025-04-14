
#ifndef _REPEATS_SUPERMAX_H_
#define _REPEATS_SUPERMAX_H_

typedef struct supermax_node {
  int *S;
  int M, num_witness, num_leaves, percent;
  struct supermax_node *next;
  int *start_indices;           // Tableau d'indices d'apparition des supermaximals
  int num_start_indices;      // Nombre d'indices
} STRUCT_SUPERMAX, *SUPERMAXIMALS;

SUPERMAXIMALS supermax_find(int *S, int M, int min_percent, int min_length);

#endif
