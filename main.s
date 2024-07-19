.global sym3, sym4
.extern sym5, sym8, sym9

.section code
.equ sym6, -sym1 + 0xFFFFFEFE
.equ sym7, sym2 + 10000 + sym6 # this is comment
.word sym1, 12345, sym2, 0x12345, 54321
sym3:
    ld $sym6, %sp
    ld $1000, %r1
    beq %r4, %r10, 0x2000
    ld [%sp + sym6], %r1
    xor %r2, %pc
    .skip 10
    bgt %r15, %sp, 0x1000
    jmp sym1
    push %r1
    ld [%sp], %r12
    pop %r1
    xchg %r13, %r2
    add %r13, %r2
    bne %r11, %r2, sym7
    csrwr %r2, %status
    csrrd %cause, %r1
    csrrd %handler, %r3
    call 10000
    sub %r12, %r2
    mul %r5, %r6
    st %sp, [%sp + 1234]
    div %r13, %r12
    .ascii "this is string"
    not %r2
    and %r2, %r9
    or %r5, %r2
    xor %r5, %r8
    shl %r14, %r4
    shr %r13, %r2
    ret
    iret
    halt
.end