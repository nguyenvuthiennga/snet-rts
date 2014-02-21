/*
 * locvecmap.h
 *
 *  Created on: Feb 24, 2014
 *      Author: thiennga
 */

#ifndef LOCVECMAP_H_
#define LOCVECMAP_H_
#include "locvec.h"
#include "lochash.h"

void SNetLocvecGetMap(snet_locvec_t *locvec, int *loc);
void SNetLocvecMapInit(int argc, char **argv);

#endif /* LOCVECMAP_H_ */
