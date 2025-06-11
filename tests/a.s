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

mov eax, dword 5

mov [ebp+-8], eax

sub esp, dword 8

lea ebx, [ebp+-4]

mov [esp+0], ebx

lea ebx, [ebp+-8]

mov [esp+4], ebx

call func

add esp, dword 8

mov eax, [ebp+-4]

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

sub esp, dword 8

lea ebx, [ebp+-4]

mov [esp+0], ebx

lea ebx, [ebp+-8]

mov [esp+4], ebx

call func

add esp, dword 8

mov eax, [ebp+-4]

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

sub esp, dword 8

lea ebx, [ebp+-4]

mov [esp+0], ebx

lea ebx, [ebp+-8]

mov [esp+4], ebx

call func

add esp, dword 8

mov eax, [ebp+-4]

mov ebx, esp
and esp, -16
push eax
push msg$
call printf
mov esp, ebx

sub esp, dword 8

lea ebx, [ebp+-4]

mov [esp+0], ebx

lea ebx, [ebp+-8]

mov [esp+4], ebx

call func

add esp, dword 8

mov eax, [ebp+-4]

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

mov ebx, [ebp+20]

mov ebx, [ebx+0]

mov ecx, [ebp+24]

mov ecx, [ecx+0]

add ebx, ecx

mov [eax+0], ebx

mov esp, ebp

pop ebp

pop edi

pop esi

pop ebx

ret


section .rodata
msg$: db `result = %d\n\0`
