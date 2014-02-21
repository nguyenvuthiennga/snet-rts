/*
 * lochash.h
 *
 *  Created on: Feb 20, 2014
 *      Author: thiennga
 */

#ifndef __LOCHASH_H_
#define __LOCHASH_H_
#include "bool.h"
#include "locvec.h"

typedef struct snet_lochash_item_t snet_lochash_item_t;
typedef struct snet_lochash_t snet_lochash_t;

void SNetDeleteLochashItem(snet_lochash_item_t **item);
snet_lochash_item_t *SNetCreateLochashItem(char *locvec, int loc);

snet_lochash_t *SNetLochashFromFile(char *fn);
snet_lochash_t *SNetCreateLochash(int size);
void SNetLochashInsert(snet_lochash_t *h, snet_lochash_item_t *item);
snet_lochash_item_t *SNetLochashRmItem(snet_lochash_t *h, char *locvec);
snet_lochash_item_t *SNetLochashLookup(snet_lochash_t *h, char *locvec);
void SNetDeleteLochash(snet_lochash_t **h_tab);

bool SNetLochashGetLoc(snet_lochash_t *h, snet_locvec_t *locvec, int *result);

#endif /* LOCHASH_H_ */
