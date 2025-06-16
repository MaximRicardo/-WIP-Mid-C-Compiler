[BITS 32]

extern memcpy
extern printf

section .text
global main
extern malloc

extern free

extern putchar

extern puts

extern func

main:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 8

lea eax, [ebp+-7]

push dword 7

push array_lit_0$

push eax

call memcpy

sub esp, dword 4

lea ebx, [ebp+-7]

mov [esp+0], ebx

call puts

add esp, dword 4

mov eax, dword 0

mov esp, ebp

pop ebp

pop edi

pop esi

pop ebx

ret

mov esp, ebp

pop ebp

pop edi

pop esi

pop ebx

ret

func:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 0

mov eax, [ebp+20]

mov esp, ebp

pop ebp

pop edi

pop esi

pop ebx

ret

mov esp, ebp

pop ebp

pop edi

pop esi

pop ebx

ret


section .rodata
msg$: db `result = %d\n\0`
array_lit_0$: db 104, 101, 108, 108, 111, 10, 0
