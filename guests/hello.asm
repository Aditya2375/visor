; hello - smallest possible guest: print a string, exit 0.
[BITS 16]
[ORG 0x1000]

start:
    mov si, msg
.loop:
    lodsb
    test al, al
    jz  .done
    out 0xE9, al
    jmp .loop
.done:
    xor al, al
    out 0xF4, al

msg db "hello from the guest", 13, 10, 0
