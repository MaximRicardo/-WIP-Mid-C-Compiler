[BITS 32]

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

sub esp, dword 8

lea eax, [ebp+-5]

add eax, dword 0

mov byte [eax+0], 5

lea eax, [ebp+-5]

add eax, dword 1

mov byte [eax+0], 10

lea eax, [ebp+-5]

add eax, dword 2

mov byte [eax+0], 15

lea eax, [ebp+-5]

add eax, dword 2

mov al, [eax+0]

and eax, dword 255

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
