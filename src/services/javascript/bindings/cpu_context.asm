PUBLIC js_trampoline
EXTERN on_enter_dispatch : PROC
EXTERN g_original        : QWORD

.code
js_trampoline PROC
    pushfq
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; RSP sekarang menunjuk ke AbiContext
    mov rcx, rsp
    sub rsp, 20h
    call on_enter_dispatch
    add rsp, 20h

    ; ===== CHECK skip_original =====
    mov al, BYTE PTR [rsp + 128]   ; skip_original
    test al, al
    jnz return_custom

call_original:
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    popfq

    jmp g_original

return_custom:
    ; ambil return value dari ctx->rax
    mov rax, QWORD PTR [rsp + 112] ; offset rax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    popfq

    ret
js_trampoline ENDP
END
