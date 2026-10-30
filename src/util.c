#include "util.h"

#include <stdio.h>
#include <stdlib.h>

void *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return NULL;
    }
    void *buf = malloc((size_t)size);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *len = (size_t)size;
    return buf;
}

int parse_mem_mib(const char *arg, long *out, long max_mib) {
    char *end = NULL;
    long v = strtol(arg, &end, 10);
    if (!end || *end || v < 1 || v > max_mib)
        return -1;
    *out = v;
    return 0;
}
