
#ifndef _REPEATS_TANDEM_H_
#define _REPEATS_TANDEM_H_

typedef struct {
  int *string, *raw_string;
  int length;

  SUFFIX_TREE tree;

  unsigned int *D,*S,*G,*N,*nonprimitive;

  unsigned int num_branching_primitive_tandem_repeats;
  unsigned int num_non_branching_primitive_tandem_repeats;
  unsigned int num_right_maximal_primitive_tandem_arrays;
  unsigned int num_branching_non_primitive_tandem_repeats;
  unsigned int num_non_branching_non_primitive_tandem_repeats;

#ifdef STATS
  unsigned int num_prep;
  unsigned int num_compares;
#endif

} TANDEM_STRUCT, *TANDEM;

TANDEM_STRUCT *tandem_prep(SUFFIX_TREE tree,
                           int *string, int *raw_string, int length);
void tandem_free(TANDEM_STRUCT *tandem);

void tandem_lookup(TANDEM_STRUCT *tandem);

#endif
