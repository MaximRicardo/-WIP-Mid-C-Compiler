[BITS 64]

extern printf

section .text
global main
func:

push rbx

push rdi

push rsi

push r12

push r13

push r14

push r15

push rbp

mov rbp, rsp

sub rsp, 0

lea eax, [rbp+64]

mov [rax+0], ebp

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rsi

pop rdi

pop rbx

ret

main:

push rbx

push rdi

push rsi

push r12

push r13

push r14

push r15

push rbp

mov rbp, rsp

sub rsp, 4

mov eax, 10

add eax, 5

mov [rbp+-4], eax

push rbp

mov rbp, rsp

sub rsp, 4

mov eax, [rbp+8]

add eax, 10

mov [rbp+-4], eax

lea eax, [rbp+8]

mov ebx, [rbp+8]

mov ecx, [rbp+8]

xchg rax, rbx
cdq
idiv ecx
mov ebx, edx

mov [rax+0], ebp

mov rbx, rsp
and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf
mov rsp, rbx

mov rsp, rbp

pop rbp

mov eax, [rbp+-4]

mov edx, 5
imul edx

mov rbx, rsp
and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf
mov rsp, rbx

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rsi

pop rdi

pop rbx

ret


section .rodata
msg: db `result = %d\n\0`
