
#ifndef _STREE_UKKONEN_H_
#define _STREE_UKKONEN_H_

//#include "strmat.h"
#include "stree_strmat.h"


#define IDENT_LENGTH 50
#define TITLE_LENGTH 200

typedef struct {
  int sequence[1000], desc_string[1000], raw_seq[1000];
  int length, alphabet, raw_alpha, db_type;
  int alpha_size;
  int ident[IDENT_LENGTH+1], title[TITLE_LENGTH+1];
} STRING;

int stree_ukkonen_add_string(SUFFIX_TREE tree, int *S, int *Sraw,
                             int M, int strid);

SUFFIX_TREE stree_ukkonen_build(STRING *string, int build_policy,
                                int build_threshold);
SUFFIX_TREE stree_gen_ukkonen_build(STRING **strings, int num_strings,
                                    int build_policy, int build_threshold);

#endif
