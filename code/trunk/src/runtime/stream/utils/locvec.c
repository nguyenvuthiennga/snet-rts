
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "locvec.h"

#define LOCVEC_INFO_TAG 493


#define LOCVEC_CAPACITY_DELTA 1



void SNetLocvecAppend(snet_locvec_t *vec, snet_loctype_t type, int num);
void SNetLocvecPop(snet_locvec_t *vec);
int SNetLocvecToptype(snet_locvec_t *vec);
void SNetLocvecTopinc(snet_locvec_t *vec);
void SNetLocvecTopdec(snet_locvec_t *vec);
int SNetLocvecTopval(snet_locvec_t *vec);
bool SNetLocvecEqual(snet_locvec_t *u, snet_locvec_t *v);
bool SNetLocvecEqualParent(snet_locvec_t *u, snet_locvec_t *v);




struct snet_locvec_t {
  int size;
  int capacity;
  snet_locitem_t *arr;
};

snet_locvec_t *SNetLocvecCreate(void)
{
  snet_locvec_t *vec = malloc(sizeof(snet_locvec_t));

  vec->size = 0;
  vec->capacity = LOCVEC_CAPACITY_DELTA;
  vec->arr = malloc(LOCVEC_CAPACITY_DELTA * sizeof(snet_locitem_t));
  return vec;
}

void SNetLocvecDestroy(snet_locvec_t *vec)
{
  free( vec->arr );
  free( vec );
}

snet_locvec_t *SNetLocvecCopy(snet_locvec_t *vec)
{
  snet_locvec_t *newvec = malloc(sizeof(snet_locvec_t));
  /* shallow copy of fields */
  *newvec = *vec;
  /* copy arr */
  newvec->arr = malloc(newvec->capacity * sizeof(snet_locitem_t));
  (void) memcpy(newvec->arr, vec->arr, newvec->size * sizeof(snet_locitem_t));
  return newvec;
}

void SNetLocvecAppend(snet_locvec_t *vec, snet_loctype_t type, int num)
{
  if (vec->size == vec->capacity) {
    /* grow */
    snet_locitem_t *newarr = malloc(
        (vec->capacity + LOCVEC_CAPACITY_DELTA) * sizeof(snet_locitem_t)
        );
    vec->capacity += LOCVEC_CAPACITY_DELTA;
    (void) memcpy(newarr, vec->arr, vec->size * sizeof(snet_locitem_t));
    free(vec->arr);
    vec->arr = newarr;
  }

  snet_locitem_t *item = &vec->arr[vec->size];
  /* pack item into a 64 bit integer */
  *item = (num << 8) | type ;
  vec->size++;
}


void SNetLocvecPop(snet_locvec_t *vec)
{
  vec->size--;
}


int SNetLocvecToptype(snet_locvec_t *vec)
{
  return (int)(vec->arr[vec->size-1] & 0xff);
}

int SNetLocvecTopval(snet_locvec_t *vec)
{
  return (int)(vec->arr[vec->size-1] >> 8);
}


void SNetLocvecTopinc(snet_locvec_t *vec)
{
  snet_locitem_t *item = &vec->arr[vec->size-1];
  int num = (int)(*item >> 8);

  *item = ((num+1) << 8) | (int)(*item & 0xff);
}

void SNetLocvecTopdec(snet_locvec_t *vec)
{
  snet_locitem_t *item = &vec->arr[vec->size-1];
  int num = (int)(*item >> 8);

  *item = ((num-1) << 8) | (int)(*item & 0xff);
}


bool SNetLocvecEqual(snet_locvec_t *u, snet_locvec_t *v)
{
  int i;

  if (u==v) return true;
  if (u->size != v->size) return false;

  for (i=0; i<u->size; i++) {
    if (u->arr[i] != v->arr[i]) { return false; }
  }
  return true;
}

bool SNetLocvecEqualParent(snet_locvec_t *u, snet_locvec_t *v)
{
  int i;

  if (u==v) return true;
  if (u->size != v->size) return false;

  for (i=0; i<u->size-1; i++) {
    if (u->arr[i] != v->arr[i]) { return false; }
  }

  i = u->size-1;
  if ( (u->arr[i] & 0xff) != (v->arr[i] & 0xff) ) {
    return false;
  }

  return true;
}






/* for serial combinator */

bool SNetLocvecSerialEnter(snet_locvec_t *vec)
{
  if (SNetLocvecToptype(vec) != LOC_SERIAL) {
    SNetLocvecAppend(vec, LOC_SERIAL, 1);
    return true;
  }
  return false;
}

void SNetLocvecSerialNext(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_SERIAL );
  SNetLocvecTopinc(vec);
}


void SNetLocvecSerialLeave(snet_locvec_t *vec, bool enter)
{
  assert( SNetLocvecToptype(vec) == LOC_SERIAL );
  if (enter) {
    SNetLocvecPop(vec);
  } else {
  //TODO clear leaf
  }
}

/* for parallel combinator */
void SNetLocvecParallelEnter(snet_locvec_t *vec)
{
  SNetLocvecAppend(vec, LOC_PARALLEL, -1);
}


void SNetLocvecParallelNext(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_PARALLEL );
  SNetLocvecTopinc(vec);
}


void SNetLocvecParallelLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_PARALLEL );
  SNetLocvecPop(vec);
}


/* for split combinator */
void SNetLocvecSplitEnter(snet_locvec_t *vec)
{
  SNetLocvecAppend(vec, LOC_SPLIT, -1);
}

void SNetLocvecSplitLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_SPLIT );
  SNetLocvecPop(vec);
}

snet_locvec_t *SNetLocvecSplitSpawn(snet_locvec_t *vec, int i)
{
  assert( SNetLocvecToptype(vec) == LOC_SPLIT );
  //TODO assert vec is disp
  snet_locvec_t *copy = SNetLocvecCopy(vec);
  SNetLocvecPop(copy);
  SNetLocvecAppend(copy, LOC_SPLIT, i);
  return copy;
}



/* for star combinator */
bool SNetLocvecStarWithin(snet_locvec_t *vec)
{
  return ( SNetLocvecToptype(vec) == LOC_STAR );
}

void SNetLocvecStarEnter(snet_locvec_t *vec)
{
  if( SNetLocvecToptype(vec) != LOC_STAR ) {
    SNetLocvecAppend(vec, LOC_STAR, 0);
  } else {
    SNetLocvecTopinc(vec);
  }
}

snet_locvec_t *SNetLocvecStarSpawn(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_STAR );
  return SNetLocvecCopy(vec);
}

void SNetLocvecStarLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_STAR );
  if (SNetLocvecTopval(vec) == 0) {
    SNetLocvecPop(vec);
  } else {
    SNetLocvecTopdec(vec);
    //TODO clear leaf/coll/disp
  }
}



/* for feedback combinator */
void SNetLocvecFeedbackEnter(snet_locvec_t *vec)
{
  SNetLocvecAppend(vec, LOC_FEEDBACK, -1);

}


void SNetLocvecFeedbackLeave(snet_locvec_t *vec)
{
  assert( SNetLocvecToptype(vec) == LOC_FEEDBACK );
  SNetLocvecPop(vec);
}


/**
 * INFO STRUCTURE SETTING/RETRIEVAL
 */

snet_locvec_t *SNetLocvecGet(snet_info_t *info)
{
  return SNetInfoGetTag(info, LOCVEC_INFO_TAG);
}


void SNetLocvecSet(snet_info_t *info, snet_locvec_t *vec)
{
  SNetInfoSetTag(info, LOCVEC_INFO_TAG, vec);
}







void SNetLocvecPrint(FILE *file, snet_locvec_t *vec)
{
  int i;
  for (i=0; i<vec->size; i++) {
    snet_locitem_t *item = &vec->arr[i];
    snet_loctype_t type = ((int)*item) & 0xff;
    int num = ((int)*item) >> 8;
    if (num >= 0) {
      fprintf(file, ":%c%d", type, num);
    } else {
      fprintf(file, ":%c", type);
    }
  }
  fprintf(file, "\n");
}

#if 0
int main(void)
{
  snet_locvec_t *v, *u;

  v = SNetLocvecCreate();

  SNetLocvecAppend(v,LOC_SERIAL,0);
  SNetLocvecAppend(v,LOC_PARALLEL,2);
  SNetLocvecAppend(v,LOC_SPLIT,4345);
  printf("equals %d\n", SNetLocvecEqual(v,v));

  u = SNetLocvecCopy(v);
  SNetLocvecAppend(u,LOC_SERIAL,1);

  SNetLocvecPrint(u);
  SNetLocvecPrint(v);
  printf("equals %d\n", SNetLocvecEqual(u,v));


  printf("toptype %c\n", SNetLocvecToptype(v));
  SNetLocvecTopinc(v);
  SNetLocvecPrint(v);

  SNetLocvecPop(u);
  SNetLocvecPrint(u);
  printf("equals %d\n", SNetLocvecEqual(u,v));

  SNetLocvecPop(u);
  SNetLocvecPrint(u);
  SNetLocvecPop(v);
  SNetLocvecPrint(v);
  printf("equals %d\n", SNetLocvecEqual(u,v));

  SNetLocvecDestroy(u);
  SNetLocvecDestroy(v);
}
#endif
