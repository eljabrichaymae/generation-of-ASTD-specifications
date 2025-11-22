/*
 * repeats_supermax.c
 *
 * The procedure for computing the supermaximals of a string.
 *
 * NOTES
 *    ?/95  -  Original Implementation  (James Knight)
 *    8/96  -  Modularized the code  (James Knight)
 *    2/99  -  Removed memory leak bug  (Jens Stoye)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#ifdef STRMAT
#include "stree_strmat.h"
#include "stree_ukkonen.h"
//#else
//#include "stree.h"
//#endif
#include "repeats_supermax.h"


/*
 *
 *
 * The data structure to hold the left predecessors of leaves in each
 * nodes sub-tree.  Declared as an array of linked lists (indexed on
 * the suffix tree node identifiers).
 *
 *
 */

typedef struct lvlist {
  int value, count;
  struct lvlist *next;
} LVNODE, *LEFTVALS;


// Structure for right values
typedef struct RIGHTVALS {
    int value;
    int count;
    struct RIGHTVALS *next;
} RVNODE,*RIGHTVALS;

static LEFTVALS *stack;

static RIGHTVALS *rstack;

int lvals_init(int num_nodes)
{
  if ((stack = malloc(num_nodes * sizeof(LEFTVALS))) == NULL)
    return 0;

  memset(stack, 0, num_nodes * sizeof(LEFTVALS));

  return 1;
}

int rvals_init(int num_nodes)
{
  if ((stack = malloc(num_nodes * sizeof(RIGHTVALS))) == NULL)
    return 0;

  memset(stack, 0, num_nodes * sizeof(RIGHTVALS));

  return 1;
}



int lvals_add_value(int id, int value, int amount)
{
  LEFTVALS node;

  for (node=stack[id]; node != NULL; node=node->next)
    if (node->value == value) {
      node->count += amount;
      return 1;
    }

  if ((node = malloc(sizeof(LVNODE))) == NULL)
    return 0;

  node->value = value;
  node->count = amount;
  node->next = stack[id];
  stack[id] = node;
  return 1;
}


int rvals_add_value(int id, int value, int amount)
{
  RIGHTVALS node;

  for (node=rstack[id]; node != NULL; node=node->next)
    if (node->value == value) {
      node->count += amount;
      return 1;
    }

  if ((node = malloc(sizeof(RVNODE))) == NULL)
    return 0;

  node->value = value;
  node->count = amount;
  node->next = rstack[id];
  rstack[id] = node;
  return 1;
}

int lvals_get_value(int id, int value)
{
  LEFTVALS node;

  for (node=stack[id]; node != NULL; node=node->next)
    if (node->value == value)
      return node->count;

  return 0;
}

int rvals_get_value(int id, int value)
{
  RIGHTVALS node;

  for (node=rstack[id]; node != NULL; node=node->next)
    if (node->value == value)
      return node->count;

  return 0;
}


void lvals_free(int num_nodes)
{
  int i;
  LEFTVALS node, next;

  for (i=0; i < num_nodes; i++) {
    for (node=stack[i]; node != NULL; node=next) {
      next = node->next;
      free(node);
    }
  }

  free(stack);
}

// void printStack(int num_nodes) {
//     printf("Content of stack:\n");
//     for (int i = 0; i < num_nodes; i++) {
//         printf("Node %d: ", i);
//         LEFTVALS node = stack[i];
//         while (node != NULL) {
//             printf("(%d, %d) ", node->value, node->count);
//             node = node->next;
//         }
//         printf("\n");
//     }
// }

/*
 * 
 *
 *
 * The procedures for compute the supermaximals and near supermaximals.
 *
 *
 *
 */
static SUPERMAXIMALS compute_supermax(SUFFIX_TREE tree, STREE_NODE node,
                                      int min_percent, int min_length,
                                      SUPERMAXIMALS list);

static SUPERMAXIMALS compute_supermax1(SUFFIX_TREE tree, STREE_NODE node,
                                      int min_percent, int min_length,
                                      SUPERMAXIMALS list);

static SUPERMAXIMALS compute_max(SUFFIX_TREE tree, STREE_NODE node,
                                      int min_percent, SUPERMAXIMALS list);

/*
 * supermax_find
 *
 * Find the supermaximals for a given string.  Return a list of them
 * (with pointers into the string specifying what the supermaximals are).
 *
 * Parameters:  S            -  the input string
 *              M            -  the string length
 *              min_percent  -  the minimum percent for any supermaximal
 *                                 (from 0 to 100, where less than 100 allows
 *                                  partial supermaximals)
 *              min_length   -  the minimum length of any reported supermaximal
 *
 * Returns:  A list of the supermaximals, or NULL.
 */
SUPERMAXIMALS supermax_find(int *S, int M, int min_percent, int min_length)
{
  SUFFIX_TREE tree;
  SUPERMAXIMALS list;

  if (S == NULL)
    return NULL;

  /*
   * Build the suffix tree and allocate everything.
   */
//#ifdef STRMAT
  if ((tree = stree_new_tree(400, 0, SORTED_LIST, 0)) == NULL)
    return NULL;

  if (stree_ukkonen_add_string(tree, S, S, M, 1) <= 0) {
    stree_delete_tree(tree);
    return NULL;
  }
//#else
//   if ((tree = stree_new_tree(400, 0)) == NULL)
//     return NULL;

//   if (stree_ukkonen_add_string(tree, S, M, 1) <= 0) {
//     stree_delete_tree(tree);
//     return NULL;
//   }
// #endif

  if (lvals_init(stree_get_num_nodes(tree)) == 0) {
    stree_delete_tree(tree);
    return NULL;
  }

  /*
   * Compute the supermaximals.
   */

  if(min_percent==-1)
    list = compute_max(tree, stree_get_root(tree), min_length,
                        NULL);
  else
    list = compute_supermax1(tree, stree_get_root(tree), min_percent, min_length,
                          NULL);


  // printf("Num of nodes: %d: ", stree_get_num_nodes(tree));
  // printStack(stree_get_num_nodes(tree));

  /*
   * Free everything.
   */
  lvals_free(stree_get_num_nodes(tree));
  stree_delete_tree(tree);

  return list;
}


/*
 * compute_supermax
 *
 * Perform the recursive computation for finding the supermaximals.
 *
 * Parameters:  tree         -  a suffix tree
 *              node         -  a suffix tree node
 *              min_percent  -  the min percent of a reported supermaximal
 *              min_length   -  the min length of a reported supermaximal
 *              list         -  the list of supermaximals found so far
 *
 * Returns:  the list of supermaximals found in the sub-tree of `node'
 */
static SUPERMAXIMALS compute_supermax(SUFFIX_TREE tree, STREE_NODE node,
                                      int min_percent, int min_length,
                                      SUPERMAXIMALS list)
{
  static int errorflag;
  int i, id, pos, index, num_leaves, diversity, witnesses, percent;
  int *str;
  STREE_NODE child;
  LEFTVALS lvalnode;
  SUPERMAXIMALS newnode, smnode, smnext;

  if (node == stree_get_root(tree))
    errorflag = 0;

  if (errorflag)
    return NULL;

  id = stree_get_ident(tree, node);
  printf("id : %d\n", id);



  /*
   * Recurse on the children, computing the numbers of left predecessors
   * for suffixes in the sub-tree rooted at the current node.
   */
  child = stree_get_children(tree, node);
  while (child != NULL) {
    list = compute_supermax(tree, child, min_percent, min_length, list);
    if (errorflag)
      return NULL;

    /*
     * Sum up the numbers of left predecessors computed for
     * the childrens' sub-trees.
     */
    lvalnode = stack[stree_get_ident(tree, child)];
    while (lvalnode != NULL) {
      lvals_add_value(id, lvalnode->value, lvalnode->count);
      lvalnode = lvalnode->next;
    }

    child = stree_get_next(tree, child);
  }

  if (node == stree_get_root(tree))
    return list;

  /*
   * Add in the left predecessors of any leaves of the current node.
   */
  for (i=1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++)
    lvals_add_value(id, (pos == 0 ? 400 : str[pos-1]), 1);

  /*
   * Determine if the current node is a supermaximal or near supermaximal.
   *
   * First, find the total number of leaves in the sub-tree and the
   * number of different left predecessors of those leaves (i.e., the
   * diversity of the left predecessors).  Any node with a diversity
   * greater than 1 is "left diverse".
   */
  num_leaves = 0;
  diversity = 0;
  for (lvalnode=stack[id]; lvalnode != NULL; lvalnode=lvalnode->next) {
    num_leaves += lvalnode->count;
    diversity++;
  }

  if (diversity == 1)
    return list;

  /*
   * Next, find out how many of the leaves at the current node or
   * its children whose left predecessor is unique (i.e, no other leaf in
   * the subtree has the same left predecessor).  Each such leaf is a
   * witness to the supermaximality of the current node's label.
   */
  witnesses = 0;
  child = stree_get_children(tree, node);
  while (child != NULL) {
    if (stree_get_num_children(tree, child) == 0 &&
        stree_get_num_leaves(tree, child) > 0) {
      for (i=1; stree_get_leaf(tree, child, i, &str, &pos, &index); i++)
        if (lvals_get_value(id, (pos == 0 ? 400 : str[pos-1])) == 1)
          witnesses++;
    }
    child = stree_get_next(tree, child);
  }

  for (i=1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++)
    if (lvals_get_value(id, (pos == 0 ? 400 : str[pos-1])) == 1)
      witnesses++;

  if (witnesses == 0)
    return list;

  /*
   * Check whether the node is sufficiently a near supermaximal.
   */
  percent = (int) (((float) witnesses) / ((float) num_leaves) * 100.0);

  if (min_percent == 0 || (min_percent < 100 && percent >= min_percent) ||
      (min_percent == 100 && witnesses == num_leaves)) {

    if ((newnode = malloc(sizeof(STRUCT_SUPERMAX))) == NULL) {
      errorflag = 1;

      for (smnode=list; smnode != NULL; smnode=smnext) {
        smnext = smnode->next;
        free(smnode);
      }

      return NULL;
    }

    newnode->M = stree_get_labellen(tree, node);
    newnode->S = stree_get_edgestr(tree, node) +
                 (stree_get_edgelen(tree, node) - newnode->M);
    newnode->num_leaves = num_leaves;
    newnode->num_witness = witnesses;
    newnode->percent = percent;
    printf("percent : %d",percent);
    // Collect start indices of the leaves
    newnode->num_start_indices = num_leaves;
    newnode->start_indices = malloc(num_leaves * sizeof(int));
    int idx = 0;

    for (i = 1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++) {
      printf("indices : %d",pos);
      newnode->start_indices[idx++] = pos;
    }

    newnode->next = list;
    list = newnode;
  }

  return list;
}

static void collect_leaves(SUFFIX_TREE tree, STREE_NODE node, int *indices, int *count) {
    int i, pos, index;
    int *str;
    STREE_NODE child;

    // Collect local leaves
    for (i = 1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++) {
        indices[(*count)++] = pos;
    }

    // Recurse on children
    child = stree_get_children(tree, node);
    while (child != NULL) {
        collect_leaves(tree, child, indices, count);
        child = stree_get_next(tree, child);
    }
}

static SUPERMAXIMALS compute_supermax1(SUFFIX_TREE tree, STREE_NODE node,
                                      int min_percent, int min_length,
                                      SUPERMAXIMALS list)
{
    static int errorflag;
    int i, id, pos, index, num_leaves, diversity, witnesses, percent;
    int *str;
    STREE_NODE child;
    LEFTVALS lvalnode;
    SUPERMAXIMALS newnode, smnode;

    if (node == stree_get_root(tree))
        errorflag = 0;

    if (errorflag)
        return NULL;

    id = stree_get_ident(tree, node);

    // Recurse on the children
    child = stree_get_children(tree, node);
    while (child != NULL) {
        list = compute_supermax1(tree, child, min_percent, min_length, list);
        if (errorflag)
            return NULL;

        lvalnode = stack[stree_get_ident(tree, child)];
        while (lvalnode != NULL) {
            lvals_add_value(id, lvalnode->value, lvalnode->count);
            lvalnode = lvalnode->next;
        }

        child = stree_get_next(tree, child);
    }

    if (node == stree_get_root(tree))
        return list;

    // Add in the left predecessors of any leaves of the current node
    for (i = 1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++)
        lvals_add_value(id, (pos == 0 ? 400 : str[pos - 1]), 1);

    // Determine if the current node is a supermaximal or near supermaximal
    num_leaves = 0;
    diversity = 0;
    for (lvalnode = stack[id]; lvalnode != NULL; lvalnode = lvalnode->next) {
        num_leaves += lvalnode->count;
        diversity++;
    }

    if (diversity == 1)
        return list;

    // Find the number of witnesses
    witnesses = 0;
    child = stree_get_children(tree, node);
    while (child != NULL) {
        if (stree_get_num_children(tree, child) == 0 &&
            stree_get_num_leaves(tree, child) > 0) {
            for (i = 1; stree_get_leaf(tree, child, i, &str, &pos, &index); i++) {
                if (lvals_get_value(id, (pos == 0 ? 400 : str[pos - 1])) == 1) {
                    witnesses++;
                }
            }
        }
        child = stree_get_next(tree, child);
    }

    for (i = 1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++) {
        if (lvals_get_value(id, (pos == 0 ? 400 : str[pos - 1])) == 1) {
            witnesses++;
        }
    }

    if (witnesses == 0)
        return list;

    // Check whether the node is sufficiently a near supermaximal
    percent = (int)(((float)witnesses) / ((float)num_leaves) * 100.0);

    if (min_percent == 0 || (min_percent < 100 && percent >= min_percent) ||
        (min_percent == 100 && witnesses == num_leaves)) {

        newnode = (SUPERMAXIMALS)malloc(sizeof(STRUCT_SUPERMAX));
        if (newnode == NULL) {
            errorflag = 1;
            while (list != NULL) {
                smnode = list;
                list = list->next;
                free(smnode->start_indices);
                free(smnode);
            }
            return NULL;
        }

        newnode->M = stree_get_labellen(tree, node);
        newnode->S = stree_get_edgestr(tree, node) +
                      (stree_get_edgelen(tree, node) - newnode->M);
        newnode->num_leaves = num_leaves;
        //printf("num_leaves: %d\n",newnode->num_leaves);
        newnode->num_witness = witnesses;
        newnode->percent = percent;
        newnode->next = list;

        // Allocate memory for indices
        newnode->start_indices = (int *)malloc(num_leaves * sizeof(int));
        if (newnode->start_indices == NULL) {
            free(newnode);
            errorflag = 1;
            while (list != NULL) {
                smnode = list;
                list = list->next;
                free(smnode->start_indices);
                free(smnode);
            }
            return NULL;
        }
        newnode->num_start_indices = 0;

        // Collect the indices of the leaves recursively
        collect_leaves(tree, node, newnode->start_indices, &newnode->num_start_indices);

        list = newnode;
    }
    return list;
}








static SUPERMAXIMALS compute_max(SUFFIX_TREE tree, STREE_NODE node,
                                       int min_length,
                                      SUPERMAXIMALS list)
{
  static int errorflag;
  int i, id, pos, index, num_leaves, diversity, witnesses, percent;
  int *str;
  STREE_NODE child;
  LEFTVALS lvalnode;
  SUPERMAXIMALS newnode, smnode, smnext;

  if (node == stree_get_root(tree))
    errorflag = 0;

  if (errorflag)
    return NULL;

  id = stree_get_ident(tree, node);

  /*
   * Recurse on the children, computing the numbers of left predecessors
   * for suffixes in the sub-tree rooted at the current node.
   */
  child = stree_get_children(tree, node);
  while (child != NULL) {
    list = compute_max(tree, child, min_length, list);
    if (errorflag)
      return NULL;

    /*
     * Sum up the numbers of left predecessors computed for
     * the childrens' sub-trees.
     */
    lvalnode = stack[stree_get_ident(tree, child)];
    while (lvalnode != NULL) {
      lvals_add_value(id, lvalnode->value, lvalnode->count);
      lvalnode = lvalnode->next;
    }

    child = stree_get_next(tree, child);
  }

  if (node == stree_get_root(tree))
    return list;

  /*
   * Add in the left predecessors of any leaves of the current node.
   */
  for (i=1; stree_get_leaf(tree, node, i, &str, &pos, &index); i++)
    lvals_add_value(id, (pos == 0 ? 400 : str[pos-1]), 1);

  /*
   * Determine if the current node is a supermaximal or near supermaximal.
   *
   * First, find the total number of leaves in the sub-tree and the
   * number of different left predecessors of those leaves (i.e., the
   * diversity of the left predecessors).  Any node with a diversity
   * greater than 1 is "left diverse".
   */
  num_leaves = 0;
  diversity = 0;
  for (lvalnode=stack[id]; lvalnode != NULL; lvalnode=lvalnode->next) {
    num_leaves += lvalnode->count;
    diversity++;
  }

  if (diversity == 1)
    return list;

 

  if (diversity > 1) {

    if ((newnode = malloc(sizeof(STRUCT_SUPERMAX))) == NULL) {
      errorflag = 1;

      for (smnode=list; smnode != NULL; smnode=smnext) {
        smnext = smnode->next;
        free(smnode);
      }

      return NULL;
    }

    newnode->M = stree_get_labellen(tree, node);
    newnode->S = stree_get_edgestr(tree, node) +
                 (stree_get_edgelen(tree, node) - newnode->M);
    newnode->num_leaves = num_leaves;
    newnode->num_witness = witnesses;
    newnode->percent = percent;
    newnode->next = list;
    list = newnode;
  }

  return list;
}