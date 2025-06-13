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

sub esp, dword 20

lea eax, [ebp+-20]

add eax, dword 0

mov dword [eax+0], 5

lea eax, [ebp+-20]

add eax, dword 4

mov dword [eax+0], 10

lea eax, [ebp+-20]

add eax, dword 8

mov dword [eax+0], 15

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
