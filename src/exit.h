#ifndef VISOR_EXIT_H
#define VISOR_EXIT_H

struct vm;
struct kvm_run;

int  handle_io(struct vm *vm, struct kvm_run *run);
int  handle_mmio(struct vm *vm, struct kvm_run *run);
void dump_guest_state(struct vm *vm, const char *why);

/* Guest-facing I/O port map */
#define PORT_CONSOLE_OUT  0xE9    /* write: byte to the host console */
#define PORT_EXIT         0xF4    /* write: exit with this status code */

#endif
