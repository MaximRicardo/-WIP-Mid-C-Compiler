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

sub esp, dword 4

mov eax, dword 18

mov [ebp+-4], eax

mov eax, [ebp+-4]

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

mov ebx, [ebp+24]

imul ebx

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
