# visor

A type-2 hypervisor for Linux that runs unmodified 16-bit and 32-bit x86
guest code on real hardware using the KVM kernel API. It allocates guest
memory, configures virtual CPU registers, runs the vCPU, and services every
exit - port I/O, MMIO, and halt - in userspace. No libraries beyond libc and
the kernel's KVM interface.

```text
$ visor run build/guests/hello.img
hello from the guest
```

## Quick start

Requires Linux with KVM support (`/dev/kvm`) - any machine with Intel VT-x
or AMD-V, including most cloud instances and the GitHub Actions runners.

```bash
make            # builds build/visor and the example guests
./build/visor run build/guests/hello.img
./build/visor run build/guests/pmode.img
```

## Usage

```text
visor run <guest.img> [-m MiB]

    run         load the image at guest physical address 0x1000 and execute it
    guest.img   a flat binary of 16-bit real-mode (or 32-bit, see below) code
    -m MiB      guest RAM size, 1-1024 MiB (default 16)
```

The guest starts in real mode at `cs:ip = 0x0100:0000`, which places its
first instruction at the top of the loaded image. Guests signal exit by
writing a status byte to port `0xF4`; visor prints `guest exit: <code>` and
exits with the same code.

## Guest ABI

| Resource            | Meaning                                          |
|---------------------|--------------------------------------------------|
| RAM                 | guest physical 0 - size (default 16 MiB)         |
| Entry point         | physical `0x1000`, real mode, `cs=0x100 ip=0`    |
| Console out         | `out 0xE9, al` writes one byte to the host stdout |
| Exit                | `out 0xF4, al` exits with status `al`            |
| MMIO                | unmapped accesses complete as no-ops             |

Example guests in `guests/`:

- **hello.asm** - prints a string with the console port, exits with status 0.
- **pmode.asm** - sets up a GDT, enables protected mode, far-jumps into
  32-bit code, prints, exits. Demonstrates that guests get real ring-0
  hardware semantics including segmentation and protected mode.

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the vCPU lifecycle,
exit handling, and design decisions.

## Tests

```bash
./tests/unit/run_tests.sh     # pure-logic unit tests (no KVM needed)
./tests/run-guest-tests.sh    # run guests for real under KVM (skips without /dev/kvm)
```

Guest tests execute each image on the actual hypervisor and diff its console
output against the expected file in `guests/`. They run in CI on every push,
where KVM is available.

## Layout

```text
src/       hypervisor core (vm lifecycle, vCPU run loop, exit handling)
guests/    example guest images (NASM, built to flat binaries)
tests/     host unit tests and the KVM integration test
docs/      architecture notes
```
