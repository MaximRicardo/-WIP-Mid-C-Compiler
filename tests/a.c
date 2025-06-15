void* malloc(int size);
void free(void *ptr);
int putchar(int c);

char* func(char *x);

int main(int argc, char **argv) {

    char string[6] = {'h', 'e', 'l', 'l', 'o', '\0'};
    putchar(string[0]);
    putchar(string[1]);
    putchar(string[2]);
    putchar(string[3]);
    putchar(string[4]);
    putchar(string[5]);
    putchar('\n');

    return 0;
}

char* func(char *x) {
    return x;
}
