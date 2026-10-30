#include "vm.h"
#include "exit.h"

#include <fcntl.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096

static size_t align_up(size_t n, size_t a) { return (n + a - 1) / a * a; }

int vm_create(struct vm *vm, size_t mem_size) {
    memset(vm, 0, sizeof(*vm));
    vm->kvm_fd = vm->vm_fd = vm->vcpu_fd = -1;
    mem_size = align_up(mem_size, PAGE_SIZE);

    vm->kvm_fd = open("/dev/kvm", O_RDWR);
    if (vm->kvm_fd < 0) {
        perror("open /dev/kvm");
        return -1;
    }
    vm->vm_fd = ioctl(vm->kvm_fd, KVM_CREATE_VM, 0);
    if (vm->vm_fd < 0) {
        perror("KVM_CREATE_VM");
        goto fail;
    }

    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem == MAP_FAILED) {
        perror("mmap guest memory");
        goto fail;
    }
    vm->mem_size = mem_size;

    struct kvm_userspace_memory_region region = {
        .slot = 0,
        .guest_phys_addr = 0,
        .memory_size = mem_size,
        .userspace_addr = (uint64_t)vm->mem,
    };
    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        goto fail;
    }

    vm->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
    if (vm->vcpu_fd < 0) {
        perror("KVM_CREATE_VCPU");
        goto fail;
    }
    int run_size = ioctl(vm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_size <= 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        goto fail;
    }
    vm->run_size = (size_t)run_size;
    vm->run = mmap(NULL, vm->run_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                   vm->vcpu_fd, 0);
    if (vm->run == MAP_FAILED) {
        perror("mmap kvm_run");
        goto fail;
    }
    return 0;

fail:
    vm_destroy(vm);
    return -1;
}

int vm_load(struct vm *vm, uint32_t guest_addr, const void *data, size_t len) {
    if (!vm->mem || guest_addr + len > vm->mem_size) {
        fprintf(stderr, "visor: guest image at 0x%x (%zu bytes) does not fit in %zu bytes of guest memory\n",
                guest_addr, len, vm->mem_size);
        return -1;
    }
    memcpy((uint8_t *)vm->mem + guest_addr, data, len);
    return 0;
}

int vm_init_real_mode(struct vm *vm, uint32_t entry) {
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return -1;
    }
    /* Real mode: every segment base = selector << 4, 64 KiB limit, present. */
    struct kvm_segment seg = {
        .base = 0, .limit = 0xFFFF, .selector = 0,
        .type = 3,          /* data: accessed, read/write */
        .present = 1, .dpl = 0, .db = 0, .s = 1, .l = 0, .g = 0,
    };
    sregs.es = sregs.ds = sregs.fs = sregs.gs = sregs.ss = seg;
    seg.type = 11;          /* code: accessed, execute/read */
    sregs.cs = seg;
    sregs.cs.selector = (uint16_t)(entry >> 4);
    sregs.cs.base = entry & ~0xFull;
    sregs.cr0 = 0x60000010; /* ET | NE, paging and protection off */
    sregs.efer = 0;
    if (ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        return -1;
    }

    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rip = entry & 0xF;
    regs.rflags = 0x2;      /* reserved bit, interrupts off */
    regs.rsp = 0x8000;      /* small stack well below the load address */
    if (ioctl(vm->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        return -1;
    }
    return 0;
}

int vm_run(struct vm *vm) {
    struct kvm_run *run = vm->run;
    for (;;) {
        int rc = ioctl(vm->vcpu_fd, KVM_RUN, 0);
        if (rc < 0) {
            perror("KVM_RUN");
            return -1;
        }
        switch (run->exit_reason) {
        case KVM_EXIT_HLT:
            return 0;
        case KVM_EXIT_IO:
            if (handle_io(vm, run) != 0)
                return 1;
            if (vm->stop)
                return vm->status;
            break;
        case KVM_EXIT_MMIO:
            if (handle_mmio(vm, run) != 0)
                return 1;
            break;
        case KVM_EXIT_SHUTDOWN:
            dump_guest_state(vm, "triple fault / shutdown");
            return 1;
        case KVM_EXIT_FAIL_ENTRY:
            fprintf(stderr, "visor: KVM failed to enter the guest (reason %llu)\n",
                    (unsigned long long)run->fail_entry.hardware_entry_failure_reason);
            return -1;
        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "visor: KVM internal error %u\n", run->internal.suberror);
            return -1;
        default:
            fprintf(stderr, "visor: unhandled exit reason %d\n", run->exit_reason);
            return 1;
        }
    }
}

void vm_destroy(struct vm *vm) {
    if (vm->run && vm->run != MAP_FAILED)
        munmap(vm->run, vm->run_size);
    if (vm->mem && vm->mem != MAP_FAILED)
        munmap(vm->mem, vm->mem_size);
    if (vm->vcpu_fd >= 0)
        close(vm->vcpu_fd);
    if (vm->vm_fd >= 0)
        close(vm->vm_fd);
    if (vm->kvm_fd >= 0)
        close(vm->kvm_fd);
}
