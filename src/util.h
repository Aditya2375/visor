#ifndef VISOR_UTIL_H
#define VISOR_UTIL_H

#include <stddef.h>

/* Read a whole file; NULL on error. Caller frees. */
void *read_file(const char *path, size_t *len);

/* Strict decimal parse with range check; 0 on success. */
int parse_mem_mib(const char *arg, long *out, long max_mib);

#endif
