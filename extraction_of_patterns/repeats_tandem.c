/*
 * repeats_tandem.c
 *
 * Implementation of the tandem array algorithm for suffix trees
 * based on the idea of first locating all branching occurrences
 * of tandem repeats.
 *
 * NOTES:
 *    1/98  -  Original implementation of the algorithms (Jens Stoye)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#ifdef STRMAT
#include "stree_strmat.h"
//#else
// #include "stree.h"
// #endif
// #include "more.h"
#include "repeats_tandem.h"


/*
 *
 * Forward References.
 *
 */
static void prepare_subtree(TANDEM_STRUCT *tandem, STREE_NODE node,
                            unsigned int d, unsigned int *num);
static void lookup_subtree(TANDEM_STRUCT *tandem, STREE_NODE node);
static void lookup_sub_subtree(TANDEM_STRUCT *tandem, STREE_NODE node,
                               STREE_NODE father, int max_id, int this_id);
static void lookup_leaf(TANDEM_STRUCT *tandem, int pos,
                        STREE_NODE father, int max_id, int this_id);
static void report_tandem(TANDEM_STRUCT *tandem, STREE_NODE node, int pos);
static void write_tandem(char *comment,int pos,int len,int rep,int *string);



/*
 * tandem_prep
 *
 * Preprocessing for the tandem array algorithm.
 *
 * Parameters:  tree        -  a suffix tree
 *              string      -  the string
 *              raw_string  -  the raw string
 *              length      -  the length of the string
 *
 * Returns:  A TANDEM_STRUCT structure
 */
TANDEM_STRUCT *tandem_prep(SUFFIX_TREE tree,
                           int *string, int *raw_string, int length)
{
  TANDEM_STRUCT *tandem;
  unsigned int num_nodes, num_leaves, num;

  if (tree == NULL)
    return NULL;

  /*
   * Allocate the memory.
   */
  if ((tandem = malloc(sizeof(TANDEM_STRUCT))) == NULL)
    return NULL;
  memset(tandem, 0, sizeof(TANDEM_STRUCT));

  tandem->string = string;
  tandem->raw_string = raw_string;
  tandem->length = length;

  tandem->tree = tree;

  num_nodes = stree_get_num_nodes(tree);
  num_leaves = length;

  if ((tandem->D = malloc(num_nodes * sizeof(unsigned int))) == NULL) {
    tandem_free(tandem);
    return NULL;
  }
  memset(tandem->D, 0, num_nodes * sizeof(unsigned int));

  if ((tandem->S = malloc(num_nodes * sizeof(unsigned int))) == NULL) {
    tandem_free(tandem);
    return NULL;
  }
  memset(tandem->S, 0, num_nodes * sizeof(unsigned int));

  if ((tandem->G = malloc(num_nodes * sizeof(unsigned int))) == NULL) {
    tandem_free(tandem);
    return NULL;
  }
  memset(tandem->G, 0, num_nodes * sizeof(unsigned int));

  if ((tandem->N=malloc(num_leaves * sizeof(unsigned int))) == NULL) {
    tandem_free(tandem);
    return NULL;
  }
  memset(tandem->N, 0, num_leaves * sizeof(unsigned int));

  if ((tandem->nonprimitive = malloc(num_nodes*sizeof(unsigned int)))==NULL) {
    tandem_free(tandem);
    return NULL;
  }
  memset(tandem->nonprimitive, 0, num_nodes * sizeof(unsigned int));

  tandem->num_branching_primitive_tandem_repeats = 0;
  tandem->num_non_branching_primitive_tandem_repeats = 0;
  tandem->num_right_maximal_primitive_tandem_arrays = 0;
  tandem->num_branching_non_primitive_tandem_repeats = 0;
  tandem->num_non_branching_non_primitive_tandem_repeats = 0;

#ifdef STATS
  tandem->num_prep = 0;
  tandem->num_compares = 0;
#endif

  /*
   * Compute the values.
   */
  num = 0;
  prepare_subtree(tandem,stree_get_root(tandem->tree),0,&num);
    
  return tandem;

}


/*
 * tandem_free
 *
 * Free the TANDEM_STRUCT structure.
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *
 * Returns:  nothing
 */
void tandem_free(TANDEM_STRUCT *tandem)
{
  if(tandem->D != NULL)
    free(tandem->D);
  if(tandem->S != NULL)
    free(tandem->S);
  if(tandem->G != NULL)
    free(tandem->G);
  if(tandem->N != NULL)
    free(tandem->N);
  if(tandem->nonprimitive != NULL)
    free(tandem->nonprimitive);
  free(tandem);
}


/*
 * prepare_subtree
 *
 * Compute arrays D, S, G, N, and nonprimitive (recursively, depth-first).
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *              node    -  STREE_NODE whose subtree is prepared.
 *              d       -  string-depth of node.
 *              num     -  depth-first leaf counter.
 *
 * Returns:  nothing
 */
static void prepare_subtree(TANDEM_STRUCT *tandem, STREE_NODE node,
                            unsigned int d, unsigned int *num)
{
  int id, leavesnum, i, pos, dummy_id;
  STREE_NODE child;
  int *dummy_string;

  id = stree_get_ident(tandem->tree, node);

  tandem->D[id] = d;
  tandem->S[id] = *num;

  /* depth-first */
  for(child=stree_get_children(tandem->tree,node);
      child!=NULL;
      child=stree_get_next(tandem->tree,child))
    prepare_subtree(tandem,child,d+stree_get_edgelen(repeats->tree,child),num);

  /* leaves */
  leavesnum = stree_get_num_leaves(tandem->tree,node);
  for(i=1; i<=leavesnum; i++) {
    stree_get_leaf(tandem->tree,node,i,&dummy_string,&pos,&dummy_id);
    tandem->N[pos] = (*num)++;
  }
  
  tandem->G[id] = *num;
  tandem->nonprimitive[id] = 0;

#ifdef STATS
  tandem->num_prep++;
#endif
}


/*
 * tandem_lookup
 *
 * Lookup all (branching) occurrences of (maximal) tandem arrays.
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *
 * Returns:  nothing
 */
void tandem_lookup(TANDEM_STRUCT *tandem)
{
  STREE_NODE root, child;

  /* do not lookup root */
  root = stree_get_root(tandem->tree);
  for(child = stree_get_children(tandem->tree,root);
      child != NULL;
      child = stree_get_next(tandem->tree,child))
    lookup_subtree(tandem,child);
}


/*
 * lookup_subtree
 *
 * Lookup subtree.
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *              node    -  STREE_NODE whose subtree is looked up.
 *
 * Returns:  nothing
 */
static void lookup_subtree(TANDEM_STRUCT *tandem, STREE_NODE node)
{
  int i, id, max_id, child_id, child_num, max_num, leavesnum,pos, dummy_id;
  STREE_NODE max_child, child;
  int *dummy_string;

  /* do not lookup leaves */
  if(stree_get_num_children(tandem->tree,node) != 0) {

    /* find largest subtree */
    id = stree_get_ident(tandem->tree,node);
    max_child = stree_get_children(tandem->tree,node);
    max_id = stree_get_ident(tandem->tree,max_child);
    max_num = tandem->G[max_id] - tandem->S[max_id];
    for(child = stree_get_next(tandem->tree,max_child);
        child != NULL;
        child = stree_get_next(tandem->tree,child)) {
      child_id = stree_get_ident(tandem->tree,child);
      child_num = tandem->G[child_id] - tandem->S[child_id];
      if(child_num > max_num) {
        max_child = child;
        max_id = child_id;
        max_num = child_num;
      }
    }

    /* check all direct leaves */
    leavesnum = stree_get_num_leaves(tandem->tree,node);
    for(i=1; i<=leavesnum; i++) {
      stree_get_leaf(tandem->tree,node,i,&dummy_string,&pos,&dummy_id);
      lookup_leaf(tandem,pos,node,max_id,-1);
    }

    /* check all children except largest subtree */
    for(child = stree_get_children(tandem->tree,node);
        child != NULL;
        child = stree_get_next(tandem->tree,child))
      if(child != max_child) {
        child_id = stree_get_ident(tandem->tree,child);
        lookup_sub_subtree(tandem,child,node,max_id,child_id);
      }

    /* recurse depth first (but any other order possible as well) */
    for(child = stree_get_children(tandem->tree,node);
        child != NULL;
        child = stree_get_next(tandem->tree,child))
      lookup_subtree(tandem,child);

  } /* if not leaf */
}


/*
 * lookup_sub_subtree
 *
 * Lookup sub-subtree.
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *              node    -  top node of sub-subtree.
 *              father  -  internal node where we started.
 *              max_id  -  id of its child with largest subtree.
 *              this_id -  id of its child where we are below.
 *
 * Returns:  nothing
 */
static void lookup_sub_subtree(TANDEM_STRUCT *tandem, STREE_NODE node,
                               STREE_NODE father, int max_id, int this_id)
{
  STREE_NODE child;
  int i, leavesnum,pos, dummy_id;
  int *dummy_string;

  /* depth-first (but order does not matter) */
  for(child = stree_get_children(tandem->tree,node);
      child != NULL;
      child = stree_get_next(tandem->tree,child))
    lookup_sub_subtree(tandem,child,father,max_id,this_id);

  /* lookup leaves */
  leavesnum = stree_get_num_leaves(tandem->tree,node);
  for(i=1; i<=leavesnum; i++) {
    stree_get_leaf(tandem->tree,node,i,&dummy_string,&pos,&dummy_id);
    lookup_leaf(tandem,pos,father,max_id,this_id);
  }
}

/*
 * lookup_leaf
 *
 * Lookup single leaf
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *              pos     -  starting position of L(leaf) in text.
 *              father  -  internal node where we started.
 *              max_id  -  id of its child with largest subtree.
 *              this_id -  id of its child where we are below.
 */
static void lookup_leaf(TANDEM_STRUCT *tandem, int pos,
                        STREE_NODE father, int max_id, int this_id)
{
  int father_id, testPos,testCount;

  father_id = stree_get_ident(tandem->tree,father);

  /* check tandem to the left */
  testPos = pos - tandem->D[father_id];
  if(testPos >= 0) {
    testCount = tandem->N[testPos];
    if(testCount>=tandem->S[father_id] && testCount<tandem->G[father_id] &&
       (this_id < 0 ||
        !(testCount>=tandem->S[this_id] && testCount<tandem->G[this_id])))
       report_tandem(tandem,father,testPos);
  }

  /* check tandem to the right */
  testPos = pos + tandem->D[father_id];
  if(testPos < tandem->length) {
    testCount = tandem->N[testPos];
    if(testCount>=tandem->S[max_id] && testCount<tandem->G[max_id])
      report_tandem(tandem,father,pos);
  }

#ifdef STATS
  tandem->num_compares += 2;
#endif
}


/*
 * report_tandem
 *
 * Report branching tandem repeat starting at position pos
 * and all rotations to the left (if exist) as well as all tandem arrays.
 *
 * Parameters:  tandem  -  a TANDEM_STRUCT structure.
 *              node    -  internal node where we started.
 *              pos     -  starting position of branching tandem repeat.
 */
static void report_tandem(TANDEM_STRUCT *tandem, STREE_NODE node, int pos)
{
  int po,p, len,id, loc_id,loc_pos,loc_len,loc_edgelen;
  STREE_NODE loc_node, loc_child;

  id = stree_get_ident(tandem->tree,node);
  len = tandem->D[id];

  /* write this tandem */
  if(tandem->nonprimitive[id]) {
    write_tandem("branching non-primitive tandem repeat",pos,len,2,
                 tandem->raw_string);
    tandem->num_branching_non_primitive_tandem_repeats++;
  }
  else {
    write_tandem("branching primitive tandem repeat",
                 pos,len,2,tandem->raw_string);
    tandem->num_branching_primitive_tandem_repeats++;
  }

  /*
   * Non-recursively test for rotations to the left (simultaneously go down).
   */
  loc_node = node;
  loc_id = id;
  loc_pos = pos;
  loc_len = 0;
  loc_child = stree_find_child(tandem->tree,loc_node,tandem->string[loc_pos]);
  loc_edgelen = stree_get_edgelen(tandem->tree,loc_child);
  for(p=pos-1; p>=0 && tandem->string[p]==tandem->string[p+len]; p--) {
    loc_len++;
    if(loc_len >= loc_edgelen) {
      loc_len -= loc_edgelen;
      loc_pos += loc_edgelen;
      loc_node = loc_child;
      loc_id = stree_get_ident(tandem->tree,loc_node);
      loc_child = stree_find_child(tandem->tree,loc_node,
                                   tandem->string[loc_pos]);
      loc_edgelen = stree_get_edgelen(tandem->tree,loc_child);
    }
    if(loc_len==0 && tandem->D[loc_id]%len==0) {
      tandem->nonprimitive[loc_id] = 1;
      loc_pos -= len;
    }
    if(tandem->nonprimitive[id]) {
      write_tandem("non-branching non-primitive tandem repeat",
                   p,len,2,tandem->raw_string);
      tandem->num_non_branching_non_primitive_tandem_repeats++;
    }
    else {
      write_tandem("non-branching primitive tandem repeat",
                   p,len,2,tandem->raw_string);
      tandem->num_non_branching_primitive_tandem_repeats++;
    }
  }

  /* non-recursive test for right-maximal primitive tandem arrays */
  if(!tandem->nonprimitive[id])
    for(po=pos-len; po>=0 && tandem->N[po]>=tandem->S[id]
                          && tandem->N[po]<tandem->G[id]; po-=len) {
      write_tandem("right-maximal primitive tandem array",
                   po,len,(pos-po)/len+2,tandem->raw_string);
      tandem->num_right_maximal_primitive_tandem_arrays++;
      for(p=po-1;
          p>=0 && p>po-len && tandem->string[p]==tandem->string[p+len]; p--) {
        write_tandem("right-maximal primitive tandem array",
                     p,len,(pos-po)/len+2,tandem->raw_string);
        tandem->num_right_maximal_primitive_tandem_arrays++;
      }
   }
}

/*
 * write tandem repeat/tandem array
 */
static void write_tandem(char *type,int pos,int len,int rep,int *raw_string)
{
  int i, textlen, restlen;
  int *s, *t, buffer[77];

  sprintf(buffer," (%d,%d,%d): ",pos+1,len,rep);
  buffer[76] = '\0';

  textlen = strlen(buffer);
  restlen = 76-textlen;
  for (s=&buffer[textlen],t=&raw_string[pos],i=0;
       i<restlen && i<len*rep; i++,s++,t++)
    *s = (isprint((int)(*t)) ? *t : '#');
  *s = '\0';
  puts(buffer);
  if(len*rep > restlen)
    puts("...");
  putc('\n',stdout);
}



       