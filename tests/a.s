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

lea rax, [rbp+64]

mov ebx, [rbp+64]

mov ecx, [rbp+68]

add ebx, ecx

mov [rax+0], ebx

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

sub rsp, 8

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

mov eax, [rbp+-4]

mov edx, 5
imul edx

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

pop rsi

pop rdi

pop rbx

ret


section .rodata
msg: db `result = %d\n\0`
