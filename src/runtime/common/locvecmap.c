/*
 * locvecmap.c
 *
 *  Created on: Feb 24, 2014
 *      Author: thiennga
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "locvecmap.h"

static snet_lochash_t *hashMap = NULL;

void SNetLocvecMapInit(int argc, char **argv) {
	int i;
	char *fn = NULL;
	for (i = 0; i < argc; i++) {
			if(strcmp(argv[i], "-map") == 0 && i + 1 <= argc) {
				fn = argv[i+1];
				break;
			}
	}

	if (fn != NULL)
		hashMap = SNetLochashFromFile(fn);
}


void SNetLocvecGetMap(snet_locvec_t *locvec, int *loc) {
	if (hashMap != NULL)
	  	SNetLochashGetLoc(hashMap, locvec, loc);
}
