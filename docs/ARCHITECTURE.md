# visor architecture

## Model

visor is a type-2 hypervisor: the Linux kernel owns the hardware and exposes
virtualization to userspace through `/dev/kvm`. visor plays the role of a
virtual machine monitor in userspace: it creates a VM, attaches memory and a
vCPU, then drives the KVM_RUN loop and services exits.

```text
+---------------------+        ioctl         +------------------+
|   visor (ring 3)    | <------------------> |  KVM (ring 0)    |
|  - guest RAM        |                      |  - VM object      |
|  - vCPU run loop    |    guest executes    |  - vCPU object    |
|  - port I/O / MMIO  | ===================> |    on real CPU    |
+---------------------+                      +------------------+
```

## VM lifecycle

1. `open("/dev/kvm")` gives the KVM handle.
2. `KVM_CREATE_VM` creates a VM - a container for guest physical memory and
   vCPUs.
3. Guest RAM is one anonymous `mmap` registered with
   `KVM_SET_USER_MEMORY_REGION`. KVM treats userspace virtual addresses as
   backing for guest physical pages and faults them in on demand, so the
   guest sees ordinary RAM from address zero.
4. `KVM_CREATE_VCPU` creates one vCPU. Its register state is read and
   written with `KVM_GET_REGS`/`KVM_SET_REGS` and the `SREGS` variants.
5. The guest image is a flat binary copied into guest memory at `0x1000`.
   The vCPU starts in real mode with `cs.base = 0x1000`, `ip = 0`, so
   execution begins at the image's first byte, with the full boot-time
   register semantics of a real machine.

## The run loop

`KVM_RUN` descends into the guest. It returns when the guest does something
the host must service:

- **KVM_EXIT_IO** - a port I/O instruction. Two ports are defined:
  `0xE9` (console byte out, written to host stdout) and `0xF4` (guest exit
  with status). Any other port write is ignored; port reads return 0xFF,
  the value a floating bus reports on real hardware.
- **KVM_EXIT_MMIO** - an access to unmapped guest physical memory. Completed
  as a no-op: reads return zero, writes are dropped. Zero (not "leave the
  buffer alone") is deliberate: the shared run page holds hypervisor
  state, and handing stale bytes to the guest would leak host memory.
  This keeps accidental near-miss accesses diagnosable instead of fatal.
- **KVM_EXIT_HLT** - the guest executed `hlt`. visor treats this as a
  graceful stop and exits with status 0.
- **KVM_EXIT_SHUTDOWN** - a triple fault or equivalent. visor dumps the
  guest register state (GPRs, segment bases, RIP, RFLAGS) to help debug the
  guest, then exits nonzero.

Anything else is reported and treated as a bug in the guest or in visor.

## Register state on shutdown

`dump_guest_state` prints general-purpose registers, RIP, RFLAGS, and every
segment base/limit/selector. When a guest triple-faults, this state is the
only clue about where it went, so it is printed verbatim rather than
summarized.

## Design decisions

- **One vCPU, one memory slot.** SMP guests and sparse memory maps are real
  hypervisor features but noise for a teaching-grade core; the ABI leaves
  room to add both without breaking guests.
- **Flat binaries at 0x1000.** No bootloader format, no ELF parsing: the
  image is copied verbatim and entered at offset zero, which keeps the
  contract between visor and guest visible in a dozen lines of assembly.
- **Real-mode entry.** Starting in real mode (not unreal or protected mode)
  means guests exercise the same transition path as booting hardware -
  `pmode.asm` proves the GDT load and protected-mode switch work under KVM
  exactly as on bare metal.
- **Exit status as ABI.** A dedicated exit port keeps test automation
  trivial: a guest's pass/fail is the hypervisor's own exit code.

## Testing

Unit tests cover guest-image loading bounds (including wrap-around of the
guest address space) without needing KVM. Integration tests run the example
guests on real hardware and compare console output byte-for-byte; CI runs
them on every push where `/dev/kvm` exists, and they skip cleanly elsewhere.
