#include <genSubKeys.h>
#include "desboxes.h"
#include <sacinterface.h>

void *genSubKeys( void *hnd, void *ptr_1)
{
  SACarg *sac_result;

  desboxes__genSubKeys1( &sac_result, ptr_1);

  SAC4SNet_out( hnd, 1, sac_result);

  return( hnd);
}
