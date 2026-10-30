; spin - never exits on its own; the watchdog must stop visor with a
; diagnostic. Expected file is unused: run-guest-tests.sh handles this
; guest specially because it takes WATCHDOG_SECS to finish.
[BITS 16]
[ORG 0x1000]
start:
    jmp start
