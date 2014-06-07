/*
 * Memory allocator/free functions
 */ 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "memfun.h"

void SNetMemFailed(void)
{
  fprintf(stderr,
          "\n\n** Fatal Error ** : Unable to Allocate Memory: %s.\n\n",
          strerror(errno));
  exit(1);
}

void *SNetMemAlloc( size_t size)
{
  void *ptr = NULL;
  if (size && (ptr = malloc(size)) == NULL) {
    SNetMemFailed();
  }
  return ptr;
}

/*
 //for LINUX only
#include <malloc.h>
#include <assert.h>
void *SNetMemResize( void *ptr, size_t size)
{
  void *nptr = SNetMemAlloc(size);
  assert(nptr != NULL);
  size_t old_size = malloc_usable_size(ptr);
  old_size = (old_size <= size) ? old_size : size;
  memcpy(nptr, ptr, old_size);
  free(ptr);
  return nptr;
}
*/

void *SNetMemResize( void *ptr, size_t size)
{
  if ((ptr = realloc(ptr, size)) == NULL) {
    SNetMemFailed();
  }
  return ptr;
}

void SNetMemFree( void *ptr)
{
  free(ptr);
}

void* SNetMemAlign( size_t size)
{
  void *vptr;
  int retval;
  size_t remain = size % LINE_SIZE;
  size_t request = remain ? (size + (LINE_SIZE - remain)) : size;

  if ((retval = posix_memalign(&vptr, LINE_SIZE, request)) != 0) {
    errno = retval;
    SNetMemFailed();
  }
  return vptr;
}

