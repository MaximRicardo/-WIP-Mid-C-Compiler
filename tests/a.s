[BITS 32]

extern printf

section .text
global main
main:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 8

mov eax, dword 5

mov [ebp+-4], eax

lea eax, [ebp+-4]

mov [ebp+-8], eax

mov eax, dword 1

cmp eax, dword 0

je _L0$

push ebp

mov ebp, esp

sub esp, dword 0

mov esp, ebp

pop ebp

jmp _L1$

_L0$:

push ebp

mov ebp, esp

sub esp, dword 0

mov esp, ebp

pop ebp

_L1$:

sub esp, dword 4

mov ebx, [ebp+-8]

mov [esp+0], ebx

call func

add esp, dword 4

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
