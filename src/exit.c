#include "exit.h"
#include "vm.h"

#include <linux/kvm.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

/* Console output coalesces into lines so guest prints stay readable. */
static int console_putc(uint8_t c) {
    return putchar(c) == EOF ? -1 : 0;
}

int handle_io(struct vm *vm, struct kvm_run *run) {
    (void)vm;
    if (run->io.direction == KVM_EXIT_IO_IN) {
        /* No input devices: an unassigned port floats high, like a real
         * bus. Never leave stale shared-page bytes for the guest. */
        memset((uint8_t *)run + run->io.data_offset, 0xFF,
               (size_t)run->io.size * run->io.count);
        return 0;
    }

    const uint8_t *data = (const uint8_t *)run + run->io.data_offset;
    for (uint32_t i = 0; i < run->io.count; i++) {
        uint8_t b = 0;
        switch (run->io.size) {
        case 1: b = ((const uint8_t *)data)[i]; break;
        case 2: b = (uint8_t)((const uint16_t *)data)[i]; break;
        case 4: b = (uint8_t)((const uint32_t *)data)[i]; break;
        default:
            fprintf(stderr, "visor: unsupported io size %u\n", run->io.size);
            return -1;
        }
        if (run->io.port == PORT_CONSOLE_OUT) {
            if (console_putc(b) != 0)
                return -1;
        } else if (run->io.port == PORT_EXIT) {
            fflush(stdout);
            vm->stop = 1;
            vm->status = b;
        }
        /* other ports: ignored, like real unassigned hardware */
    }
    return 0;
}

int handle_mmio(struct vm *vm, struct kvm_run *run) {
    (void)vm;
    /* No MMIO devices yet; complete the access harmlessly instead of
     * killing the guest for touching an unassigned address. Reads return
     * zeroes: the shared page holds hypervisor state, so leaving it
     * alone would leak host bytes into the guest. */
    if (!run->mmio.is_write)
        memset(run->mmio.data, 0, 8);
    return 0;
}

void dump_guest_state(struct vm *vm, const char *why) {
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpu_fd, KVM_GET_REGS, &regs) < 0)
        return;
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
        return;
    fprintf(stderr, "visor: guest stopped: %s\n", why);
    fprintf(stderr, "  rip=%llx rsp=%llx rflags=%llx\n",
            (unsigned long long)regs.rip, (unsigned long long)regs.rsp,
            (unsigned long long)regs.rflags);
    fprintf(stderr, "  rax=%llx rbx=%llx rcx=%llx rdx=%llx\n",
            (unsigned long long)regs.rax, (unsigned long long)regs.rbx,
            (unsigned long long)regs.rcx, (unsigned long long)regs.rdx);
    fprintf(stderr, "  cs=%x/%llx ds=%x/%llx ss=%x/%llx cr0=%llx\n",
            sregs.cs.selector, (unsigned long long)sregs.cs.base,
            sregs.ds.selector, (unsigned long long)sregs.ds.base,
            sregs.ss.selector, (unsigned long long)sregs.ss.base,
            (unsigned long long)sregs.cr0);
}
