[BITS 64]

extern printf

section .text

global main
main:
push rax
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

mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf

pop rax
ret

section .rodata
msg: db `result = %d\n\0`
