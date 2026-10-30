#include "../harness.h"
#include "exit.h"
#include "vm.h"

#include <linux/kvm.h>
#include <stdlib.h>
#include <string.h>

#define IO_DATA(f) ((uint8_t *)&(f)->run + (f)->run.io.data_offset)

/* Build a fake kvm_run page in a plain buffer and drive the exit
 * dispatchers through it. */

struct fakerun {
    struct kvm_run run;
    uint8_t data[64];
};

static void mkio(struct fakerun *f, uint32_t port, uint8_t dir,
                 uint8_t size, uint32_t count) {
    memset(f, 0, sizeof(*f));
    f->run.io.port = port;
    f->run.io.direction = dir;
    f->run.io.size = size;
    f->run.io.count = count;
    f->run.io.data_offset = sizeof(struct kvm_run);
}

int main(void) {
    struct vm vm;
    memset(&vm, 0, sizeof(vm));
    struct fakerun f;

    /* IN from an unassigned port floats high */
    mkio(&f, 0x3F8, KVM_EXIT_IO_IN, 1, 4);
    memset(IO_DATA(&f), 0x00, 4);
    CHECK(handle_io(&vm, &f.run) == 0, "IN accepted");
    CHECK(IO_DATA(&f)[0] == 0xFF && IO_DATA(&f)[3] == 0xFF,
          "unassigned port reads return 0xFF");

    /* OUT to the exit port stops the VM with the guest's status */
    mkio(&f, PORT_EXIT, KVM_EXIT_IO_OUT, 1, 1);
    IO_DATA(&f)[0] = 42;
    CHECK(handle_io(&vm, &f.run) == 0, "exit write accepted");
    CHECK(vm.stop == 1 && vm.status == 42, "exit port sets stop and status");

    /* OUT to an unknown port is ignored */
    mkio(&f, 0x1234, KVM_EXIT_IO_OUT, 1, 1);
    vm.stop = 0;
    CHECK(handle_io(&vm, &f.run) == 0 && vm.stop == 0, "unknown OUT ignored");

    /* odd io sizes are rejected, not silently truncated */
    mkio(&f, PORT_CONSOLE_OUT, KVM_EXIT_IO_OUT, 3, 1);
    CHECK(handle_io(&vm, &f.run) != 0, "size-3 io rejected");

    /* MMIO reads return zeroes, never stale shared-page bytes */
    memset(&f, 0, sizeof(f));
    f.run.mmio.is_write = 0;
    memset(f.run.mmio.data, 0x5A, 8);
    CHECK(handle_mmio(&vm, &f.run) == 0, "MMIO read accepted");
    CHECK(f.run.mmio.data[0] == 0 && f.run.mmio.data[7] == 0,
          "MMIO read returns zeroes");

    /* MMIO writes are harmlessly dropped */
    f.run.mmio.is_write = 1;
    CHECK(handle_mmio(&vm, &f.run) == 0, "MMIO write dropped");

    TEST_RESULTS();
}
