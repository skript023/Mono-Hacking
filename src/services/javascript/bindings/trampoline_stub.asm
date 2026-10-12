OPTION CASEMAP:NONE

EXTERN JsDetourDispatch:PROC

.code

PUBLIC JsDetourEntry
JsDetourEntry PROC
    sub rsp, 0A8h

    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    movdqu [rsp+40h], xmm0
    movdqu [rsp+50h], xmm1
    movdqu [rsp+60h], xmm2
    movdqu [rsp+70h], xmm3
    lea rax, [rsp+0A8h]
    mov [rsp+80h], rax
    mov [rsp+88h], r10
    mov qword ptr [rsp+90h], 0
    mov qword ptr [rsp+98h], 0

    lea rcx, [rsp+20h]
    mov rdx, r10
    call JsDetourDispatch

    add rsp, 0A8h
    ret
JsDetourEntry ENDP

PUBLIC JsInvokeOriginal
JsInvokeOriginal PROC
    push rbp
    mov rbp, rsp
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    mov r12, rcx
    mov r13, rdx
    mov r14d, r8d

    mov r15d, 20h
    cmp r14d, 4
    jbe invoke_stack_ready
    mov eax, r14d
    sub eax, 4
    lea r15, [rax*8+20h]
invoke_stack_ready:
    test r15, 0Fh
    jnz invoke_stack_aligned
    add r15, 8
invoke_stack_aligned:
    sub rsp, r15

    cmp r14d, 4
    jbe invoke_load_registers
    mov esi, 4
    lea rdi, [rsp+20h]
invoke_copy_stack:
    cmp esi, r14d
    jae invoke_load_registers
    mov eax, esi
    sub eax, 4
    mov rbx, [r12+80h]
    mov rax, [rbx+rax*8]
    mov [rdi], rax
    add rdi, 8
    inc esi
    jmp invoke_copy_stack

invoke_load_registers:
    movdqu xmm0, [r12+20h]
    movdqu xmm1, [r12+30h]
    movdqu xmm2, [r12+40h]
    movdqu xmm3, [r12+50h]
    mov rcx, [r12]
    mov rdx, [r12+8]
    mov r8,  [r12+10h]
    mov r9,  [r12+18h]
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
JsInvokeOriginal ENDP

END
