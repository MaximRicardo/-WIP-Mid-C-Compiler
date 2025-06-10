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

sub rsp, qword 16

mov eax, dword 10

add eax, dword 5

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

sub rsp, qword 8

mov eax, [rbp+12]

add eax, dword 10

mov [rbp+-4], eax

lea rax, [rbp+12]

mov ebx, [rbp+12]

mov ecx, [rbp+-4]

add ebx, ecx

mov [rax+0], ebx

mov rsp, rbp

pop rbp

sub rsp, qword 8

mov ebx, dword 100

mov [rsp+0], ebx

mov ebx, dword 3

mov [rsp+4], ebx

call func

add rsp, qword 8

mov [rbp+-9], al

mov r15, rsp
and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf
mov rsp, r15

mov eax, dword 0

cmp eax, dword 0

je _L0

push rbp

mov rbp, rsp

sub rsp, qword 0

mov eax, [rbp+20]

sub eax, dword 5

mov rsp, rbp

pop rbp

jmp _L1

_L0:

mov eax, dword 1

cmp eax, dword 0

je _L2

push rbp

mov rbp, rsp

sub rsp, qword 0

mov eax, [rbp+20]

sub eax, dword 10

mov rsp, rbp

pop rbp

jmp _L3

_L2:

mov eax, dword 1

cmp eax, dword 0

je _L4

push rbp

mov rbp, rsp

sub rsp, qword 0

mov eax, [rbp+20]

sub eax, dword 15

mov rsp, rbp

pop rbp

jmp _L5

_L4:

push rbp

mov rbp, rsp

sub rsp, qword 0

mov eax, [rbp+20]

sub eax, dword 20

mov rsp, rbp

pop rbp

_L5:

_L3:

_L1:

mov r15, rsp
and rsp, -16
mov rbx, rax
mov al, 0
mov rdi, msg
mov rsi, rbx
call printf
mov rsp, r15

mov eax, dword 0

and eax, dword -1

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rbx

ret

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

sub rsp, qword 0

mov eax, [rbp+56]

mov ebx, [rbp+60]

imul ebx

and eax, dword -1

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rbx

ret

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rbx

ret

foo:

push rbx

push r12

push r13

push r14

push r15

push rbp

mov rbp, rsp

sub rsp, qword 0

mov rsp, rbp

pop rbp

pop r15

pop r14

pop r13

pop r12

pop rbx

ret

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
