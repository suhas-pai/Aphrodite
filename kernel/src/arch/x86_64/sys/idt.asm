section .text

extern isr_handle_interrupt
extern lapic_eoi

%macro push_all 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro pop_all 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro idt_func 1
align 16
idt_func_%1:
    cld
    %if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 30
        push qword 0
    %endif
    push_all
    mov rdi, %1
    mov rsi, rsp
    call isr_handle_interrupt
    call lapic_eoi
    pop_all
    add rsp, 8
    iretq
%endmacro

%assign i 0
%rep 256
    idt_func i
%assign i i+1
%endrep

%macro idt_func_ref 1
    dq idt_func_%1
%endmacro

section .data
global idt_thunks
idt_thunks:

%assign i 0
%rep 256
    idt_func_ref i
%assign i i+1
%endrep

