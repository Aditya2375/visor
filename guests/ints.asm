; ints - installs an int3 handler in the IVT, triggers the interrupt, and
; verifies control returns to the interrupted instruction. Exercises the
; vCPU's real-mode interrupt delivery, not just straight-line code.
[BITS 16]
[ORG 0x1000]

start:
    cli
    xor ax, ax
    mov ds, ax                  ; IVT lives at physical 0
    mov word [3*4], handler - 0x1000  ; vector 3: offset within cs=0x100
    mov word [3*4+2], 0x100     ; vector 3: segment (we are loaded at 0x1000)
    sti

    int3                        ; handler runs, sets [hit], returns here
    cmp byte [hit], 1
    jne .fail

    mov si, ok
    call print
    xor al, al
    out 0xF4, al

.fail:
    mov si, bad
    call print
    mov al, 1
    out 0xF4, al

handler:
    mov byte [cs:hit - 0x1000], 1   ; cs-relative: label minus ORG base
    iret

print:
    lodsb
    test al, al
    jz  .done
    out 0xE9, al
    jmp print
.done:
    ret

hit db 0
ok  db "int3 delivered, handler ran, iret returned", 13, 10, 0
bad db "int3 handler did not run", 13, 10, 0
