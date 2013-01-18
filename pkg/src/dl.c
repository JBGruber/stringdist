/* This code is gratefully copied from Nick Logan's githup repository
 * https://github.com/ugexe/Text--Levenshtein--Damerau--XS/blob/master/damerau-int.c
 * 
 *
 * Changes/additions to original code:
 * - Added MAX macro
 * - Added R.h, Rdefines.h inclusion
 * - Added R interface function
 *
 * MvdL, 2013.01.17
 * mark.vanderloo@gmail.com
 */

/* ugexe@cpan.org (Nick Logan)    */

#include <R.h>
#include <Rdefines.h>

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))


/* Our unsorted dictionary linked list.   */
/* Note we use character ints, not chars. */

struct dictionary{
  unsigned int key;
  unsigned int value;
  struct dictionary* next;
};
typedef struct dictionary item;


static __inline item* push(unsigned int key,item* curr){
  item* head;
  head = malloc(sizeof(item));   
  head->key = key;
  head->value = 0;
  head->next = curr;
  return head;
}


static __inline item* find(item* head,unsigned int key){
  item* iterator = head;
  while(iterator){
    if(iterator->key == key){
      return iterator;
    }
    iterator = iterator->next;
  }
 
  return NULL;
}

static __inline item* uniquePush(item* head,unsigned int key){
  item* iterator = head;

  while(iterator){
    if(iterator->key == key){
      return head;
    }
    iterator = iterator->next;
  }
 
  return push(key,head); 
}

static void dict_free(item* head){
  item* iterator = head;
  while(iterator){
    item* temp = iterator;
    iterator = iterator->next;
    free(temp);
  }

  head = NULL;
}

/* End of Dictionary Stuff */



 
/* All calculations/work are done here */

static double distance(unsigned int src[],unsigned int tgt[],unsigned int x,unsigned int y, double *weight, double maxDistance){
  item *head = NULL;
  unsigned int swapCount,swapScore,targetCharCount,i,j;
  double *scores = malloc( (x + 2) * (y + 2) * sizeof(double) );
  double score_ceil = (double) x + y;
 
  /* intialize matrix start values */
  scores[0] = score_ceil;  
  scores[1 * (y + 2) + 0] = score_ceil;
  scores[0 * (y + 2) + 1] = score_ceil;
  scores[1 * (y + 2) + 1] = 0;
  head = uniquePush(uniquePush(head,src[0]),tgt[0]);

  double del, ins, sub;

  /* work loops    */
  /* i = src index */
  /* j = tgt index */
  for(i=1;i<=x;i++){ 
    head = uniquePush(head,src[i]);
    scores[(i+1) * (y + 2) + 1] = i;
    scores[(i+1) * (y + 2) + 0] = score_ceil;

    swapCount = 0;
    for(j=1;j<=y;j++){
      if(i == 1) {
          head = uniquePush(head,tgt[j]);
          scores[1 * (y + 2) + (j + 1)] = j;
          scores[0 * (y + 2) + (j + 1)] = score_ceil;
      }

      targetCharCount = find(head,tgt[j-1])->value;
      swapScore = scores[targetCharCount * (y + 2) + swapCount] + i - targetCharCount - 1 + j - swapCount;
      
Rprintf("swapscore: %g\n",weight[3]+swapScore);
Rprintf("score 1  : %g\n",scores[i * (y + 2) + j] );
Rprintf("score 2  : %g\n",scores[(i+1) * (y + 2) + j] + 1);
Rprintf("score 3  : %g\n",scores[i * (y + 2) + (j + 1)] + 1);

      if(src[i-1] != tgt[j-1]){
        del = scores[i * (y + 2) + j] + weight[0];
        ins = scores[(i+1) * (y + 2) + j] + weight[1];
        sub = scores[i * (y + 2) + (j + 1)] + weight[2];     
        scores[(i+1) * (y + 2) + (j + 1)] = MIN(weight[3]+swapScore,(MIN(del, MIN(ins, sub ))+1));
      }else{ 
        swapCount = j;
        scores[(i+1) * (y + 2) + (j + 1)] = MIN(weight[0] + scores[i * (y + 2) + j], weight[3]*swapScore);
      } 
    }

    /* We will return -1 here if the */
    /* current score > maxDistance   */
    if(maxDistance != 0 && maxDistance < scores[(i+1) * (y + 2) + (y+1)]) {
      dict_free(head);
      free(scores);
      return -1;
    }

    
    find(head,src[i-1])->value = i;
  }

  {
  unsigned int score = scores[(x+1) * (y + 2) + (y + 1)];
  dict_free(head);
  free(scores);
  return score;
  }
}

// -- interface with R 

SEXP R_dl(SEXP a, SEXP b, SEXP ncharA, SEXP ncharB, SEXP weight, SEXP maxDistance){
   PROTECT(a);
   PROTECT(b);
   PROTECT(ncharA);
   PROTECT(ncharB);
   PROTECT(weight);
   
   unsigned int i, j, k;
   unsigned int na = length(a);
   unsigned int nb = length(b);

   SEXP yy; 
   PROTECT(yy = allocVector(REALSXP, MAX(na,nb)));
   double *y = REAL(yy);

   for ( k=0; k < na; ++k ){
      i = k % na;
      j = k % nb;
      y[i] = distance(
         (unsigned int *) CHAR(STRING_ELT(a,i)),
         (unsigned int *) CHAR(STRING_ELT(b,j)),
         INTEGER(ncharA)[i],
         INTEGER(ncharB)[j],
         REAL(weight),
         1000
      );
   }

   UNPROTECT(6);
   return(yy);
} 

