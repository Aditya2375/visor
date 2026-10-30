; mmio - guest physical addresses outside the assigned region must complete
; as no-ops: reads return zeroes, writes are dropped, the guest keeps going.
; Uses protected mode to reach 0x1000000, one byte past the default 16 MiB.
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

    mov eax, [0x1000000]        ; MMIO read: must come back zero, not stale
    test eax, eax
    jnz .fail
    mov dword [0x1000000], 0xDEADBEEF   ; MMIO write: must be dropped
    mov eax, [0x1000000]
    test eax, eax
    jnz .fail

    mov esi, ok
    call print
    xor al, al
    out 0xF4, al

.fail:
    mov esi, bad
    call print
    mov al, 1
    out 0xF4, al

print:
    lodsb
    test al, al
    jz  .done
    out 0xE9, al
    jmp print
.done:
    ret

ok  db "mmio reads zeroed, writes dropped, guest alive", 13, 10, 0
bad db "mmio access leaked real data", 13, 10, 0

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
