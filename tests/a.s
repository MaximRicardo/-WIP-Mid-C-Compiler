[BITS 64]

extern printf

section .text

global main
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
mov eax, 5

add eax, 8

mov ebx, 123

xchg rax, rbx
mov edx, 94
imul edx
xchg rax, rbx

imul ebx

cdq
mov esi, 4
idiv esi

mov eax, 10

add eax, 5

push rax

mov eax, [rbp+-8]

mov edx, 5
imul edx

and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf

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
