[BITS 32]

extern memcpy
extern printf

section .text
global main
extern malloc

extern free

extern func

main:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 12

lea eax, [ebp+-12]

push dword 12

push array_lit_0$

push eax

call memcpy

lea eax, [ebp+-12]

add eax, dword 8

mov eax, [eax+0]

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

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
array_lit_0$: dd 1, 2, 3
