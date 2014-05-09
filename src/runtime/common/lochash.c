/*
 * lochash.c
 *
 *  Created on: Feb 20, 2014
 *      Author: thiennga
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "bool.h"
#include "lochash.h"
#include "locvec.h"

struct snet_lochash_item_t {
  char *locvec;
  int loc;
};

struct snet_lochash_t {
  int size;
  int count;
  snet_lochash_item_t **data;
};


snet_lochash_item_t *SNetCreateLochashItem(char *locvec, int loc) {
  snet_lochash_item_t *item = (snet_lochash_item_t *) malloc(sizeof(snet_lochash_item_t));
  item->locvec = (char *) malloc(sizeof(char) * (strlen(locvec) + 1));
  strcpy(item->locvec, locvec);
  item->loc = loc;
  return item;
}

snet_lochash_t *SNetCreateLochash(int size) {
  snet_lochash_t *h = (snet_lochash_t *) malloc(sizeof(snet_lochash_t));
  h->size = size;
  h->count = 0;
  h->data = (snet_lochash_item_t **) malloc(sizeof(snet_lochash_item_t*) * size);
  int i;
  for (i = 0; i < size; i++)
    h->data[i] = NULL;
  return h;
}


uint32_t jenkins_hash(char *key)
{
    uint32_t hash, i;
    int len = strlen(key);
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

void SNetLochashInsert(snet_lochash_t *h, snet_lochash_item_t *item) {
  assert(h->size > h->count);
  int pos = jenkins_hash(item->locvec) % h->size;
  while (h->data[pos] != NULL)
    pos = (pos + 1) % h->size;

  h->data[pos] = item;
  h->count++;
}

int getPosHash(snet_lochash_t *h, char *locvec) {
  int pos = jenkins_hash(locvec) % h->size;
  if (h->data[pos] == NULL)
    return -1;

  int mark = pos;
  do{
  	if (h->data[pos] == NULL) {
  		pos = (pos + 1) % h->size;
  		continue;
  	}

    if(strcmp(h->data[pos]->locvec, locvec) == 0)
      return pos;
      pos = (pos + 1) % h->size;
  } while (pos != mark);

  /* pos = mark */
   return -1;
}

snet_lochash_item_t *SNetLochashRmItem(snet_lochash_t *h, char *locvec) {
  int pos = getPosHash(h, locvec);
  if (pos < 0) return NULL;
  snet_lochash_item_t *re = h->data[pos];
  h->data[pos] = NULL;
  return re;
}

snet_lochash_item_t *SNetLochashLookup(snet_lochash_t *h, char *locvec) {
  int pos = getPosHash(h, locvec);
  if (pos < 0) return NULL;
  return h->data[pos];
}

void SNetDeleteLochash(snet_lochash_t **h_tab) {
  snet_lochash_t *h = *h_tab;
  int i;
  for (i = 0; i < h->size; i++)
    if (h->data[i] != NULL)
    	SNetDeleteLochashItem(&h->data[i]);

  free(h->data);
  free(h);
}

bool SNetLochashGetLoc(snet_lochash_t *h, snet_locvec_t *locvec, int *result){
	// get the key string first
	int len = 100;
	char *key;
	int cnt;
	while(1) {
		key = (char *) malloc(len * sizeof(char));
		cnt = SNetLocvecGetKey(key, len, locvec);
		if (cnt < len)
			break;
		else
			len = cnt + 1;
	}

	int pos = getPosHash(h, key);
	if (pos < 0)
		return false;
	*result = h->data[pos]->loc;
	return true;
}

snet_lochash_t *SNetLochashFromFile(char *fn) {
	FILE *f = fopen(fn, "r");
	if (f == NULL) {
		printf("map file does not exists!\n");
		return NULL;
	}
	char buf[1000];
	int loc;
	snet_lochash_item_t *item;
	int size;
	if (fscanf(f, "%d", &size) == EOF) {
		printf("map file is empty!\n");
		return NULL;
	}

	snet_lochash_t *hash = SNetCreateLochash(size * 2);
	while (fscanf(f, "%s", buf) != EOF && fscanf(f, "%d", &loc) != EOF) {
		item = SNetCreateLochashItem(buf, loc);
		SNetLochashInsert(hash, item);
	}
	return hash;
}

void SNetDeleteLochashItem(snet_lochash_item_t **item) {
	free((*item)->locvec);
	free(item);
}

