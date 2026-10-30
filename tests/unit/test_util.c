#include "../harness.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    long v = 0;
    CHECK(parse_mem_mib("16", &v, 1024) == 0 && v == 16, "plain value parses");
    CHECK(parse_mem_mib("1", &v, 1024) == 0 && v == 1, "minimum accepted");
    CHECK(parse_mem_mib("1024", &v, 1024) == 0 && v == 1024, "maximum accepted");
    CHECK(parse_mem_mib("0", &v, 1024) != 0, "zero rejected");
    CHECK(parse_mem_mib("-5", &v, 1024) != 0, "negative rejected");
    CHECK(parse_mem_mib("1025", &v, 1024) != 0, "over maximum rejected");
    CHECK(parse_mem_mib("16x", &v, 1024) != 0, "trailing junk rejected");
    CHECK(parse_mem_mib("", &v, 1024) != 0, "empty rejected");
    CHECK(parse_mem_mib("99999999999999999999", &v, 1024) != 0, "overflow rejected");

    /* read_file round-trip */
    const char *path = "/tmp/visor-util-test.bin";
    FILE *f = fopen(path, "wb");
    const char payload[] = "visor-util";
    fwrite(payload, 1, sizeof(payload), f);
    fclose(f);
    size_t len = 0;
    void *buf = read_file(path, &len);
    CHECK(buf != NULL, "read_file reads an existing file");
    CHECK(len == sizeof(payload) && memcmp(buf, payload, len) == 0, "contents match");
    free(buf);
    CHECK(read_file("/tmp/visor-no-such-file.bin", &len) == NULL, "missing file fails");
    f = fopen(path, "wb"); fclose(f);   /* zero bytes */
    CHECK(read_file(path, &len) == NULL, "empty file rejected");
    remove(path);

    TEST_RESULTS();
}
