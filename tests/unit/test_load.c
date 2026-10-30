#include "../harness.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>

int main(void) {
    struct vm vm;
    memset(&vm, 0, sizeof(vm));
    vm.mem = malloc(1 << 20);
    vm.mem_size = 1 << 20;

    char img[100];
    memset(img, 0xCC, sizeof(img));

    CHECK(vm_load(&vm, 0x1000, img, sizeof(img)) == 0, "in-bounds load accepted");
    CHECK(memcmp((char *)vm.mem + 0x1000, img, sizeof(img)) == 0, "bytes land at guest address");
    CHECK(vm_load(&vm, 0, img, 1) == 0, "load at guest address zero");
    CHECK(vm_load(&vm, vm.mem_size - 100, img, 100) == 0, "load ending exactly at the top");
    CHECK(vm_load(&vm, vm.mem_size - 99, img, 100) != 0, "one byte past the top rejected");
    CHECK(vm_load(&vm, vm.mem_size, img, 1) != 0, "load wholly out of bounds rejected");
    CHECK(vm_load(&vm, 0xFFFFFFFF - 10, img, 100) != 0, "wrap-around rejected");

    free(vm.mem);
    TEST_RESULTS();
}
