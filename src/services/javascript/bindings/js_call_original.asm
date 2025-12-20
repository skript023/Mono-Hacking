PUBLIC call_original

.code
call_original PROC
    ; RCX = fn
    ; RDX = a0
    ; R8  = a1
    ; R9  = a2
    ; [rsp+20h] = a3

    sub rsp, 20h          ; reserve shadow space FIRST

    mov rax, rcx          ; fn*
    mov rcx, rdx          ; a0
    mov rdx, r8           ; a1
    mov r8,  r9           ; a2
    mov r9,  [rsp+20h+20h] ; a3 (old rsp + 20h)

    call rax

    add rsp, 20h
    ret
call_original ENDP
END
