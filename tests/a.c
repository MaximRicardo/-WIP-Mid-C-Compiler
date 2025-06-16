void* malloc(int size);
void free(void *ptr);
int putchar(int c);
int puts(char *str);

char* func(char *x);

int main(int argc, char **argv) {

    char string[7] = "hello\n";
    puts(string);

    return 0;
}

char* func(char *x) {
    return x;
}
