[BITS 32]

extern memcpy
extern printf

section .text
global main
extern malloc

extern free

extern printf

extern func

main:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 4

mov eax, dword 10

mov [ebp+-4], eax

_L0$:

mov eax, [ebp+-4]

cmp eax, dword 0

je _L1$

push ebp

mov ebp, esp

sub esp, dword 0

sub esp, dword 8

mov ebx, array_lit_0$

mov [esp+0], ebx

mov ebx, [ebp+4]

mov [esp+4], ebx

call printf

add esp, dword 8

lea eax, [ebp+4]

mov ebx, [ebp+4]

sub ebx, dword 1

mov [eax+0], ebx

mov esp, ebp

pop ebp

jmp _L0$

_L1$:

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
array_lit_0$: db 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33, 32, 105, 32, 61, 32, 37, 100, 10, 0
