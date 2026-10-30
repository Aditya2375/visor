#include "exit.h"
#include "vm.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

#define DEFAULT_MEM_MIB 16
#define MAX_MEM_MIB     1024
#define GUEST_LOAD_ADDR 0x1000
#define WATCHDOG_SECS   10      /* a guest that never stops gets this long */

static void on_alarm(int sig) {
    (void)sig;
    visor_watchdog_fired = 1;
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s run <guest.img> [-m MiB]\n", argv0);
    fprintf(stderr, "\nRuns a flat 16-bit guest binary at guest physical 0x%x.\n", GUEST_LOAD_ADDR);
    fprintf(stderr, "Guest contract:\n");
    fprintf(stderr, "  out 0xE9, al   write a byte to the host console\n");
    fprintf(stderr, "  out 0xF4, al   exit with status al (0 = success)\n");
    fprintf(stderr, "  hlt            clean stop (exit status 0)\n");
}

int main(int argc, char **argv) {
    if (argc < 3 || strcmp(argv[1], "run") != 0) {
        usage(argv[0]);
        return 2;
    }
    const char *image_path = argv[2];
    long mem_mib = DEFAULT_MEM_MIB;
    if (argc == 5 && strcmp(argv[3], "-m") == 0) {
        if (parse_mem_mib(argv[4], &mem_mib, MAX_MEM_MIB) != 0) {
            fprintf(stderr, "visor: -m expects 1-%d (MiB), got '%s'\n",
                    MAX_MEM_MIB, argv[4]);
            return 2;
        }
    } else if (argc != 3) {
        usage(argv[0]);
        return 2;
    }

    size_t len = 0;
    void *image = read_file(image_path, &len);
    if (!image) {
        fprintf(stderr, "visor: cannot read guest image '%s'\n", image_path);
        return 2;
    }

    struct vm vm;
    if (vm_create(&vm, (size_t)mem_mib * 1024 * 1024) != 0) {
        free(image);
        return 1;
    }
    int rc = vm_load(&vm, GUEST_LOAD_ADDR, image, len);
    free(image);                /* the guest owns a copy now */
    if (rc != 0) {
        vm_destroy(&vm);
        return 1;
    }
    if (vm_init_real_mode(&vm, GUEST_LOAD_ADDR) != 0) {
        vm_destroy(&vm);
        return 1;
    }

    signal(SIGALRM, on_alarm);
    alarm(WATCHDOG_SECS);
    rc = vm_run(&vm);
    alarm(0);
    fflush(stdout);
    if (rc >= 0)
        fprintf(stderr, "guest exit: %d\n", rc);
    vm_destroy(&vm);
    return rc < 0 ? 1 : rc;         /* guest status, or 1 on hypervisor error */
}
