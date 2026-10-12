.intel_syntax noprefix
.text

.globl JsDetourEntry
.seh_proc JsDetourEntry
JsDetourEntry:
    sub rsp, 0xa8
    .seh_stackalloc 0xa8
    .seh_endprologue

    mov [rsp+0x20], rcx
    mov [rsp+0x28], rdx
    mov [rsp+0x30], r8
    mov [rsp+0x38], r9
    movdqu [rsp+0x40], xmm0
    movdqu [rsp+0x50], xmm1
    movdqu [rsp+0x60], xmm2
    movdqu [rsp+0x70], xmm3
    lea rax, [rsp+0xa8]
    mov [rsp+0x80], rax
    mov [rsp+0x88], r10
    mov qword ptr [rsp+0x90], 0
    mov qword ptr [rsp+0x98], 0

    lea rcx, [rsp+0x20]
    mov rdx, r10
    call JsDetourDispatch

    add rsp, 0xa8
    ret
.seh_endproc

.globl JsInvokeOriginal
.seh_proc JsInvokeOriginal
JsInvokeOriginal:
    push rbp
    .seh_pushreg rbp
    mov rbp, rsp
    .seh_setframe rbp, 0
    push rbx
    .seh_pushreg rbx
    push rsi
    .seh_pushreg rsi
    push rdi
    .seh_pushreg rdi
    push r12
    .seh_pushreg r12
    push r13
    .seh_pushreg r13
    push r14
    .seh_pushreg r14
    push r15
    .seh_pushreg r15
    .seh_endprologue

    mov r12, rcx
    mov r13, rdx
    mov r14d, r8d

    mov r15d, 0x20
    cmp r14d, 4
    jbe 1f
    mov eax, r14d
    sub eax, 4
    lea r15, [rax*8+0x20]
1:
    test r15, 0xf
    jnz 2f
    add r15, 8
2:
    sub rsp, r15

    cmp r14d, 4
    jbe 4f
    mov esi, 4
    lea rdi, [rsp+0x20]
3:
    cmp esi, r14d
    jae 4f
    mov eax, esi
    sub eax, 4
    mov rbx, [r12+0x80]
    mov rax, [rbx+rax*8]
    mov [rdi], rax
    add rdi, 8
    inc esi
    jmp 3b

4:
    movdqu xmm0, [r12+0x20]
    movdqu xmm1, [r12+0x30]
    movdqu xmm2, [r12+0x40]
    movdqu xmm3, [r12+0x50]
    mov rcx, [r12]
    mov rdx, [r12+0x08]
    mov r8,  [r12+0x10]
    mov r9,  [r12+0x18]
    call r13

    add rsp, r15
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
    pop rbp
    ret
.seh_endproc
