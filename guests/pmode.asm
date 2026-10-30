; pmode - real mode to protected mode transition, entirely inside the guest.
; Installs a GDT, enters 32-bit protected mode, prints, exits.
[BITS 16]
[ORG 0x1000]

start:
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:pm32

[BITS 32]
pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x7000

    mov esi, msg
.loop:
    lodsb
    test al, al
    jz  .done
    out 0xE9, al
    jmp .loop
.done:
    xor al, al
    out 0xF4, al

msg db "protected mode reached inside the guest", 13, 10, 0

align 8
gdt:
    dq 0
    dw 0xFFFF, 0                    ; 0x08: flat code
    db 0, 0x9A, 0xCF, 0
    dw 0xFFFF, 0                    ; 0x10: flat data
    db 0, 0x92, 0xCF, 0
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt
