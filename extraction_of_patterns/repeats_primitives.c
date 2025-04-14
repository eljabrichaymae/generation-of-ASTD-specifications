/*
 * repeats_primitives.c
 *
 * This file contains an implementation of Crochemore's algorithm (1981) to 
 * find all occurrences of primitive tandem repeats in O(n log n) time.
 *
 * NOTES:
 *    7/98  -  Original implementation of the algorithms (Jens Stoye)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "strmat.h"
//#include "more.h"
#include "repeats_primitives.h"
#include <stdbool.h>
#include <math.h>

/*
 *
 * Forward References.
 *
 */
void pr_remove_list(pr_list *l);
void pr_remove_node(pr_node *n);



// Définition de la liste chaînée des triplets
TripletNode *triplet_list_head = NULL;
static size_t triplet_count = 0;

// Fonction pour obtenir la tête de la liste
TripletNode* get_triplet_list_head() {
    return triplet_list_head;
}



void add_triplet(int start, int length, int iteration) {
    TripletNode *new_node = (TripletNode *)malloc(sizeof(TripletNode));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_node->triplet.start = start;
    new_node->triplet.length = length;
    new_node->triplet.iteration = iteration;
    new_node->triplet.sequence = NULL;
    new_node->next = triplet_list_head;
    triplet_list_head = new_node;
    triplet_count++;
}

// Fonction pour vérifier si deux séquences sont des traductions circulaires
bool are_circular_permutations(const int* seq1, size_t length1, const int* seq2, size_t length2) {
    if (length1 != length2) return false;

    // Crée un tableau doublé pour seq1
    int* doubled_seq1 = (int*)malloc(2 * length1 * sizeof(int));
    if (doubled_seq1 == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Copie seq1 deux fois dans doubled_seq1
    memcpy(doubled_seq1, seq1, length1 * sizeof(int));
    memcpy(doubled_seq1 + length1, seq1, length1 * sizeof(int));

    bool is_permutation = false;
    for (size_t i = 0; i < length1; ++i) {
        if (memcmp(seq2, doubled_seq1 + i, length1 * sizeof(int)) == 0) {
            is_permutation = true;
            break;
        }
    }

    free(doubled_seq1);
    return is_permutation;
}


int compare_sequences(int* seq1, int len1, int* seq2, int len2) {
    if (len1 != len2) {
        return 0; // Les séquences ne sont pas de la même longueur
    }
    return memcmp(seq1, seq2, len1 * sizeof(int)) == 0;
}


// // Fonction pour afficher une séquence
// void print_sequence(const int* sequence, size_t length) {
//     for (size_t i = 0; i < length; i++) {
//         printf("%d ", sequence[i]);
//     }
//     printf("\n"); // Pour aller à la ligne après avoir imprimé la séquence
// }


void group_and_verify_tandem_repeats(const int *sequence) {
    if (triplet_list_head == NULL) return;

    TripletNode *current = triplet_list_head;

    while (current != NULL) {
        // Si la séquence n'a pas encore été assignée, on l'assigne maintenant
        if (current->triplet.sequence == NULL) {
            // Allocate memory for the sequence (no +1 needed for integers)
            current->triplet.sequence = (int *)malloc(current->triplet.length * sizeof(int));
            if (current->triplet.sequence == NULL) {
                fprintf(stderr, "Memory allocation for sequence failed\n");
                exit(EXIT_FAILURE);
            }
            // Copy the sequence using memcpy
            memcpy(current->triplet.sequence, sequence + current->triplet.start-1, current->triplet.length * sizeof(int));
        }

        TripletNode *next_node = current->next;
        TripletNode *prev_node = current;
        
        //printf("avant la boucle: %d\n",current->triplet.start + (current->triplet.length * current->triplet.iteration));
        
        int* current_sequence = current->triplet.sequence;
        
        while (next_node != NULL) {
            int* next_sequence = (int*)malloc(next_node->triplet.length * sizeof(int));
            if (next_sequence == NULL) {
                fprintf(stderr, "Memory allocation for next sequence failed\n");
                exit(EXIT_FAILURE);
            }
            memcpy(next_sequence, sequence + next_node->triplet.start-1, next_node->triplet.length * sizeof(int));
            
            
            int current_end = current->triplet.start + (current->triplet.length * current->triplet.iteration);
            int next_end = next_node->triplet.start + (next_node->triplet.length * next_node->triplet.iteration);
            int current_start = current->triplet.start;
            int next_start = next_node->triplet.start;

            //printf("current_start: %d\n", current_start);
            //printf("next_start: %d\n", next_start);
            //printf("current_sequence: \n");
            //print_sequence(current_sequence,current->triplet.length);
            //printf("next_sequence: \n");
            //print_sequence(next_sequence,next_node->triplet.length);
            
            if (current_end >= next_start && current->triplet.length == next_node->triplet.length &&
                are_circular_permutations(current_sequence, current->triplet.length, next_sequence, next_node->triplet.length)) {
                
                if (((next_end - current_end)) % current->triplet.length == 0) {
                    current->triplet.iteration += ((next_end - current_end)) / current->triplet.length;
                }
                
                // Supprime next_node
                prev_node->next = next_node->next;
                free(next_node->triplet.sequence);
                free(next_node);
                
                next_node = prev_node->next;
            } else {
                prev_node = next_node;
                next_node = next_node->next;
            }
        }
        current = current->next;
    }
}


// Helper function to check if two sequences are circular shifts of each other



// Function to merge overlapping tandem repeats
void fusion_of_overlapping_repeats() {
    if (triplet_list_head == NULL) return;

    TripletNode *current = triplet_list_head;

    while (current != NULL) {
        TripletNode *prev_next_node = current; 
        TripletNode *next_node = current->next;
        TripletNode *intermediate_node = NULL;

        while (next_node != NULL) {
            int current_end = prev_next_node->triplet.start + (prev_next_node->triplet.length * prev_next_node->triplet.iteration);
            int next_end = next_node->triplet.start + (next_node->triplet.length * next_node->triplet.iteration);
            int next_start = next_node->triplet.start;
            int current_start = prev_next_node->triplet.start;
            
                if (current_end > next_start 
                //&& is_circular_shift(prev_next_node->triplet.sequence, next_node->triplet.sequence, prev_next_node->triplet.length)
                ) {

                if (next_end - current_end <= prev_next_node->triplet.length || next_end - current_end < 0) {
                    // Merge nodes by deleting the next_node
                    if (intermediate_node == NULL) {
                        prev_next_node->next = next_node->next; // Update prev_next_node's next
                        free(next_node->triplet.sequence);
                        free(next_node);
                        next_node = prev_next_node->next; // Move to the next node
                    } else {
                        intermediate_node->next = next_node->next; // Update intermediate_node's next
                        free(next_node->triplet.sequence);
                        free(next_node);
                        next_node = intermediate_node->next; // Move to the next node
                        intermediate_node = NULL; // Reset intermediate_node
                    }
                } else if ((next_end - current_end) - (next_start - current_start) == prev_next_node->triplet.length) {
                    // Increment iteration of current and delete the next_node
                    prev_next_node->triplet.iteration += 1;
                    prev_next_node->next = next_node->next; // Update prev_next_node's next
                    free(next_node->triplet.sequence);
                    free(next_node);
                    next_node = prev_next_node->next; // Move to the next node
                } else {
                    prev_next_node = next_node;
                    next_node = next_node->next;
                }
            } else if (current_start < next_start && current_end > next_end) {
                // Set the intermediate node
                intermediate_node = next_node;
                next_node = next_node->next;
            } else {
                prev_next_node = next_node;
                next_node = next_node->next;
            }
           
            
        }
        current = current->next;
    }
}



void sort_triplets() {
    if (triplet_list_head == NULL || triplet_list_head->next == NULL) {
        return;
    }

    TripletNode *sorted = NULL;

    TripletNode *current = triplet_list_head;
    while (current != NULL) {
        TripletNode *next = current->next;

        if (sorted == NULL || sorted->triplet.start > current->triplet.start) {
            current->next = sorted;
            sorted = current;
        } else {
            TripletNode *sorted_current = sorted;
            while (sorted_current->next != NULL 
            && sorted_current->next->triplet.start <= current->triplet.start) {
                sorted_current = sorted_current->next;
            }
            current->next = sorted_current->next;
            sorted_current->next = current;
        }

        current = next;
    }

    triplet_list_head = sorted;
}




int is_within(int start1, int length1, int start2) {
    return start2 >= start1 && start2 < start1 + length1;
}


/*===========================================================================*/
/*
 * pr_append_entry
 *
 * Append entry e to list l: O(1) time.
 *
 * Parameters:  e  -  an entry
 *              l  -  a list
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_append_entry(pr_entry *e, pr_list *l)
{
  e->next = NULL;
  e->prev = l->last;
  e->inList = l;
  if(l->last != NULL)
    l->last->next = e;
  if(l->entries == NULL)
    l->entries = e;
  l->last = e;
  l->len++;
  return;
} /* pr_append_entry() */

/*===========================================================================*/
/*
 * pr_remove_entry
 *
 * Remove entry e from its list: O(1) time.
 *
 * Parameters:  e  -  an entry
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_remove_entry(pr_entry *e)
{
  if(e->inList != NULL) { /* only if entry was not removed earlier */
    if(e->prev != NULL)
      e->prev->next = e->next;
    else
      e->inList->entries = e->next;
    if(e->next != NULL)
      e->next->prev = e->prev;
    else
      e->inList->last = e->prev;
    e->inList->len--;
    if(e->inList->len == 0)
      pr_remove_list(e->inList);
    e->inList = NULL;
  }
  return;
} /* pr_remove_entry() */

/*===========================================================================*/
/*
 * pr_move_entry
 *
 * Remove entry e from its list and append it to list l: O(1) time.
 *
 * Parameters:  e  -  an entry
 *              l  -  a list
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_move_entry(pr_entry *e, pr_list *l)
{
  pr_remove_entry(e);
  pr_append_entry(e,l);
  return;
} /* pr_move_entry() */

/*===========================================================================*/
/*
 * pr_new_list
 *
 * Create new (empty) list: O(1) time.
 *
 * Parameters:  p  -  a primitives structure
 *
 * Returns:  the new list
 */
/*---------------------------------------------------------------------------*/
pr_list *pr_new_list(primitives_struct *p)
{
  pr_list *l_new;

  l_new = &p->lists[p->nextList++];
  l_new->next = l_new->prev = NULL;
  l_new->atNode = NULL;
  l_new->entries = l_new->last = NULL;
  l_new->len = 0;
  return l_new;
} /* pr_new_list() */

/*===========================================================================*/
/*
 * pr_append_list
 *
 * Append list l to the lists at node n: O(1) time.
 *
 * Parameters:  l  -  a list
 *              n  -  a node
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_append_list(pr_list *l, pr_node *n)
{
  l->next = NULL;
  l->prev = n->last;
  l->atNode = n;
  if(n->last != NULL)
    n->last->next = l;
  if(n->lists == NULL)
    n->lists = l;
  n->last = l;
  return;
} /* pr_append_list() */

/*===========================================================================*/
/*
 * pr_remove_list
 *
 * Remove list l from its node: O(1) time.
 *
 * Parameters:  l  -  a list
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_remove_list(pr_list *l)
{
  if(l->prev != NULL)
    l->prev->next = l->next;
  else
    l->atNode->lists = l->next;
  if(l->next != NULL)
    l->next->prev = l->prev;
  else
    l->atNode->last = l->prev;
  l->atNode = NULL;
  return;
} /* pr_remove_list() */

/*===========================================================================*/
/*
 * pr_replace_list
 *
 * Replace list l by a copy of itself (l_new): O(length of l) time.
 *
 * Parameters:  p  -  a primitives structure
 *              l  -  a list
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_replace_list(primitives_struct *p, pr_list *l)
{
  pr_list *l_new;
  pr_entry *e, *e_new;

  /* create new list (manually in p->lists2) */
  l_new = &p->lists2[p->nextList2++];

  /* replace l by new list */
  l_new->next = l->next;
  l_new->prev = l->prev;
  l_new->atNode = l->atNode;
  l_new->entries = l_new->last = NULL;
  l_new->len = 0;
  if(l->prev != NULL)
    l->prev->next = l_new;
  else
    l->atNode->lists = l_new;
  if(l->next != NULL)
    l->next->prev = l_new;
  else
    l->atNode->last = l_new;

  /* remove l from its node */
  l->atNode = NULL;

  /* fill new list with copies of l's entries */
  for(e=l->entries; e!=NULL; e=e->next) {
    e_new = &p->entries2[e-p->entries];
    pr_append_entry(e_new,l_new);
  }

  return;
} /* pr_replace_list() */

/*===========================================================================*/
/*
 * pr_new_node
 *
 * Create new (empty) node: O(1) time.
 *
 * Parameters:  p  -  a primitives structure
 *
 * Returns:  the new node
 */
/*---------------------------------------------------------------------------*/
pr_node *pr_new_node(primitives_struct *p)
{
  pr_node *n_new;

  n_new = &p->nodelist[p->nextNode++];
  n_new->next = n_new->prev = NULL;
  n_new->inNodelist = NULL;
  n_new->lists = n_new->last = n_new->last_source_list = NULL;
  return n_new;
} /* pr_new_node() */

/*===========================================================================*/
/*
 * pr_append_node
 *
 * Append node n to nodes of p: O(1) time.
 *
 * Parameters:  p  -  a primitives structure
 *              n  -  a node
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_append_node(primitives_struct *p, pr_node *n)
{
  n->next = NULL;
  n->prev = p->last;
  n->inNodelist = p;
  if(p->last != NULL)
    p->last->next = n;
  if(p->nodes == NULL)
    p->nodes = n;
  p->last = n;
  return;
} /* pr_append_node() */

/*===========================================================================*/
/*
 * pr_remove_node
 *
 * Remove node n from its nodelist: O(1) time.
 *
 * Parameters:  n  -  a node
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_remove_node(pr_node *n)
{
  if(n->prev != NULL)
    n->prev->next = n->next;
  else
    n->inNodelist->nodes2 = n->next;
  if(n->next != NULL)
    n->next->prev = n->prev;
  else
    n->inNodelist->last2 = n->prev;
  n->inNodelist = NULL;
  return;
} /* pr_remove_node() */

/*===========================================================================*/
/*
 * primitives_prep
 *
 * Create new (empty) primitives structure p_new: O(1) time.
 *
 * Parameters:  string         -  the string
 *              raw_string     -  the raw string
 *              length         -  the length of the string
 *
 * Returns:  a primitives structure
 */
/*---------------------------------------------------------------------------*/
primitives_struct *primitives_prep(int *string, int *raw_string, int length)
{
  primitives_struct *p_new;

  if((p_new = malloc(sizeof(primitives_struct))) == NULL)
    return NULL;
  memset(p_new, 0, sizeof(primitives_struct));

  p_new->string = string;
  p_new->raw_string = raw_string;
  p_new->length = length;

  if((p_new->entries = malloc(length * sizeof(pr_entry))) == NULL) {
    primitives_free(p_new);
    return NULL;
  }
  memset(p_new->entries, 0, length * sizeof(pr_entry));

  if((p_new->entries2 = malloc(length * sizeof(pr_entry))) == NULL) {
    primitives_free(p_new);
    return NULL;
  }
  memset(p_new->entries2, 0, length * sizeof(pr_entry));

  if((p_new->lists = malloc(2*length * sizeof(pr_list))) == NULL) {
    primitives_free(p_new);
    return NULL;
  }
  memset(p_new->lists, 0, 2*length * sizeof(pr_list));

  if((p_new->lists2 = malloc(length * sizeof(pr_list))) == NULL) {
    primitives_free(p_new);
    return NULL;
  }
  memset(p_new->lists2, 0, length * sizeof(pr_list));

  p_new->nextList = p_new->nextList2 = 0;

  if((p_new->nodelist = malloc(length * sizeof(pr_node))) == NULL) {
    primitives_free(p_new);
    return NULL;
  }
  memset(p_new->nodelist, 0, length * sizeof(pr_node));

  if((p_new->nodelist2 = malloc(length * sizeof(pr_node))) == NULL) {
    primitives_free(p_new);
    return NULL;
  }
  memset(p_new->nodelist2, 0, length * sizeof(pr_node));

  p_new->nextNode = 0;
  p_new->nodes = p_new->nodes2 = p_new->last, p_new->last2 = NULL;

  p_new->num_primitive_tandem_repeat_occs = 0;

#ifdef STATS
  p_new->num_compares = 0;
#endif


  return p_new;

} /* primitives_prep() */

/*===========================================================================*/
/*
 * pr_next_level
 *
 * Step forward to next level (i.e. swap nodelists): O(1) time.
 *
 * Parameters:  p  -  a primitives structure
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_next_level(primitives_struct *p)
{
  pr_node *n_tmp;

  /* entries and lists are not swapped: they are moved in the main procedure */

  n_tmp = p->nodelist;
  p->nodelist = p->nodelist2;
  p->nodelist2 = n_tmp;

  p->nodes2 = p->nodes;
  p->nodes = NULL;
  p->last2 = p->last;
  p->last = NULL;
  p->nextNode = p->nextList2 = 0;

  return;
} /* pr_next_level() */

/*===========================================================================*/
/*
 * primitives_free
 *
 * Delete primitives structure p: O(1) time.
 *
 * Parameters:  p  -  a primitives structure
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void primitives_free(primitives_struct *p)
{
  free(p->entries);
  free(p->entries2);
  free(p->lists);
  free(p->lists2);
  free(p->nodelist);
  free(p->nodelist2);
  free(p);
  return;
} /* primitives_free() */

/*===========================================================================*/
/*
 * pr_write
 *
 * Write a primitive tandem repeat.
 *
 * Parameters:  raw_string  -  the raw string
 *              pos         -  starting position of the repeat
 *              len         -  length of the repeat
 *              rep         -  number of repeats
 *              type        -  repeat type
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_write(int *raw_string, int pos, int len, int rep, int *type)
{
  int i, textlen, restlen;
  int *s, *t, buffer[1000];

  printf(buffer,"%s: (%d,%d,%d) ",type,pos+1,len,rep);
  //add_triplet(pos+1,len,rep);
  buffer[1000] = '\0';

  textlen = strlen(buffer);
  restlen = 1000-textlen;
  for(i=0,s=&buffer[textlen],t=&raw_string[pos];
      i<restlen && i<len*rep;
      i++,s++,t++)
    *s = isprint((int)(*t)) ? *t : '#';
  *s = '\0';
  puts(buffer);
  if(len > restlen)
    puts("...");
  putc('\n',stdout);

} /* pr_write() */







/*===========================================================================*/
/*
 * pr_report
 *
 * Report all primitive tandem repeats in this iteration.
 *
 * Parameters:  p          -  a primitives structure
 *              iteration  -  iteration
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_report(primitives_struct *p, int iteration) {
    pr_node *n;
    pr_list *l;
    pr_entry *e, *previous;

    for (n = p->nodes; n != NULL; n = n->next) {
        for (l = n->lists; l != NULL; l = l->next) {
            if (l->entries != l->last) { /* plus d'une entrée dans la liste l */
                previous = l->entries;
                for (e = l->entries->next; e != NULL; e = e->next) {
                    if (e - previous == iteration) {

                      int start = previous - p->entries;
                      int length = iteration;
                    // pr_write(p->raw_string, previous - p->entries, iteration, 2, 
                    //        "primitive tandem repeat");
                      add_triplet(start + 1, length, 2);
                      p->num_primitive_tandem_repeat_occs++;  
                    }
                    previous = e;
                }
            }
        }
    }
} /* pr_report() */



// void print_triplets() {
//     TripletNode *current = triplet_list_head;
//     while (current != NULL) {
//         printf("(%d,%d,%d, [", 
//                current->triplet.start, 
//                current->triplet.length, 
//                current->triplet.iteration);
//         if (current->triplet.sequence != NULL) {
//             for (int i = 0; i < current->triplet.length; i++) {
//                 if (i > 0) printf(", ");
//                 printf("%d", current->triplet.sequence[i]);
//             }
//         }
//         printf("])\n");
//         current = current->next;
//     }
// }



/*===========================================================================*/
/*
 * pr_create_basic_lists
 *
 * Create basic lists: O(int_MAX) space, O(length + int_MAX) time.
 *
 * Parameters:  p  -  a primitives structure
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void pr_create_basic_lists(primitives_struct *p)
{
  int i,c;
  pr_list *occ[10000+1];
  pr_node *n;

  for(i=0; i<=10000; i++)
    occ[i] = NULL;

  n = pr_new_node(p);
  pr_append_node(p,n);
  for(i=0; i<p->length ; i++) {
    c = (int)p->string[i];
    if(occ[c] == NULL)
      pr_append_list((occ[c]=pr_new_list(p)),n);
    pr_append_entry(&p->entries[i],occ[c]);
  }
  return;
} /* pr_create_basic_lists() */

/*===========================================================================*/
/*
 * primitives_find
 *
 * This is Crochemore's O(n log n) time algorithm.
 *
 * Parameters:  p  -  a primitives structure
 *
 * Returns:  nothing
 */
/*---------------------------------------------------------------------------*/
void primitives_find(primitives_struct *p)
{
  pr_node *n,*n_next,*nn,*new_n;
  pr_list *l,*l_next,*maxlist;
  pr_entry *e,*ee;
  int i, maxlistlen, pos;

  // printf("Début de primitives_find\n");

  /* create basic lists */
  // printf("Création des listes de base\n");
  pr_create_basic_lists(p);

  for(i=1; i<p->length && p->nodes!=NULL; i++) {
    // printf("Itération i = %d, longueur = %d\n", i, p->length);

    /* report primitive repeats */
    // printf("Reporting primitive repeats pour i = %d\n", i);
    pr_report(p, i);

    /* step forward to next level */
    // printf("Passage au niveau suivant\n");
    pr_next_level(p);

    /* pour chaque noeud du niveau précédent */
    for(n=p->nodes2; n!=NULL; n=n_next) {
      n_next = n->next;

      // printf("Traitement du noeud à l'adresse %p\n", n);

      /* trouver la plus grande liste */
      maxlistlen = 0;
      for(l=n->lists; l!=NULL; l=l->next) {
        if(l->len > maxlistlen) {
          maxlistlen = l->len;
          maxlist = l;
        }
      }
      // printf("Liste la plus grande trouvée avec longueur = %d\n", maxlistlen);

      /* copier les petites listes/déplacer la grande liste (sauf singletons) */
      for(l=n->lists; l!=NULL; l=l_next) {
        l_next = l->next;

        if(l == maxlist) {
          // printf("Suppression de la grande liste\n");
          pr_remove_list(l);
        } else {
          // printf("Remplacement d'une petite liste\n");
          pr_replace_list(p, l);
        }

        if(l->len == 1) {
          // printf("Marquage de l'entrée comme supprimée\n");
          l->entries->inList = NULL;
        } else {
          // printf("Création d'un nouveau noeud pour la liste\n");
          new_n = pr_new_node(p);
          pr_append_node(p, new_n);
          pr_append_list(l, new_n);
        }
      } /* for l */
    } /* for n */

    /* utiliser les listes restantes (petites) pour extraire des entrées */
    for(n=p->nodes2; n!=NULL; n=n->next) {
      for(l=n->lists; l!=NULL; l=l->next) {
        for(e=l->entries; e!=NULL; e=e->next) {
          pos = e - p->entries2;
          if(pos != 0) {
            ee = &p->entries[pos-1];
            if(ee->inList != NULL) {
              nn = ee->inList->atNode;
              if(nn->last_source_list != l) {
                //printf("Ajout d'une nouvelle liste\n");
                pr_append_list(pr_new_list(p), nn);
                nn->last_source_list = l;
              }
             // printf("Déplacement d'une entrée\n");
              pr_move_entry(ee, nn->last);
            }
          }
#ifdef STATS
          p->num_compares++;
#endif
        }
      }
    }

    /* supprimer les entrées de longueur itération */
   // printf("Suppression de l'entrée pour la longueur itération %d\n", p->length-i);
    pr_remove_entry(&p->entries[p->length-i]);

#ifdef STATS
    p->num_compares++;
#endif
  } /* for i */

 // printf("Fin de primitives_find\n");
}

/* primitives_find() */

/****** EOF (repeats_primitives.c) *******************************************/

