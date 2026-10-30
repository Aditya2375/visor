#ifndef VISOR_VM_H
#define VISOR_VM_H

#include <stddef.h>
#include <stdint.h>

/* One virtual machine: KVM fd pair, guest physical memory, one vCPU. */
struct vm {
    int     kvm_fd;
    int     vm_fd;
    int     vcpu_fd;
    void   *mem;            /* guest physical memory, host side */
    size_t  mem_size;
    void   *run;            /* struct kvm_run shared page */
    size_t  run_size;
    int     stop;           /* set by an exit handler to end vm_run */
    int     status;         /* guest-provided exit status */
};

int  vm_create(struct vm *vm, size_t mem_size);
int  vm_load(struct vm *vm, uint32_t guest_addr, const void *data, size_t len);
int  vm_init_real_mode(struct vm *vm, uint32_t entry);
int  vm_run(struct vm *vm);                     /* returns guest exit code */
void vm_destroy(struct vm *vm);

#endif
