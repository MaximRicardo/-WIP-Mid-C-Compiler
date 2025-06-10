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

mov eax, [ebp+20]

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

mov eax, dword 10

add eax, dword 5

mov [ebp+-4], eax

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

push ebp

mov ebp, esp

sub esp, dword 4

mov eax, [ebp+4]

add eax, dword 10

mov [ebp+-4], eax

lea eax, [ebp+4]

mov ebx, [ebp+4]

mov ecx, [ebp+-4]

add ebx, ecx

mov [eax+0], ebx

mov esp, ebp

pop ebp

sub esp, dword 8

mov ebx, dword 100

mov [esp+0], ebx

mov ebx, dword 3

mov [esp+4], ebx

call func

add esp, dword 8

mov [ebp+-8], al

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

mov eax, dword 0

cmp eax, dword 0

je _L0

push ebp

mov ebp, esp

sub esp, dword 0

mov eax, [ebp+8]

sub eax, dword 5

mov esp, ebp

pop ebp

jmp _L1

_L0:

mov eax, dword 1

cmp eax, dword 0

je _L2

push ebp

mov ebp, esp

sub esp, dword 0

mov eax, [ebp+8]

sub eax, dword 10

mov esp, ebp

pop ebp

jmp _L3

_L2:

mov eax, dword 1

cmp eax, dword 0

je _L4

push ebp

mov ebp, esp

sub esp, dword 0

mov eax, [ebp+8]

sub eax, dword 15

mov esp, ebp

pop ebp

jmp _L5

_L4:

push ebp

mov ebp, esp

sub esp, dword 0

mov eax, [ebp+8]

sub eax, dword 20

mov esp, ebp

pop ebp

_L5:

_L3:

_L1:

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

foo:

push ebx

push esi

push edi

push ebp

mov ebp, esp

sub esp, dword 0

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
