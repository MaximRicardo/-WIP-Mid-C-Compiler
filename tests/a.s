[BITS 64]

extern printf

section .text
global main
main:

push rbx

push r12

push r13

push r14

push r15

push rbp

mov rbp, rsp

sub rsp, 16

mov eax, 10

add eax, 5

mov [rbp+-4], eax

mov r15, rsp
and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf
mov rsp, r15

push rbp

mov rbp, rsp

sub rsp, 8

mov eax, [rbp+12]

add eax, 10

mov [rbp+-4], eax

lea rax, [rbp+12]

mov ebx, [rbp+12]

mov ecx, [rbp+-4]

add ebx, ecx

mov [rax+0], ebx

mov rsp, rbp

pop rbp

sub rsp, 8

mov ebx, 4

mov [rsp+0], ebx

mov ebx, 3

mov [rsp+4], ebx

call func

add rsp, 8

mov [rbp+-9], al

mov r15, rsp
and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf
mov rsp, r15

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rbx

ret

func:

push rbx

push r12

push r13

push r14

push r15

push rbp

mov rbp, rsp

sub rsp, 0

mov eax, [rbp+56]

mov ebx, [rbp+60]

imul ebx

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rbx

ret


section .rodata
msg: db `result = %d\n\0`
