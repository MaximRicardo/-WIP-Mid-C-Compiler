[BITS 32]

extern memcpy
extern printf

section .text
global main
extern malloc

extern free

extern puts

extern func

main:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 16

lea eax, [ebp+-15]

push dword 15

push array_lit_0$

push eax

call memcpy

sub esp, dword 4

lea ebx, [ebp+-15]

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
array_lit_0$: db 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33, 10, 0
