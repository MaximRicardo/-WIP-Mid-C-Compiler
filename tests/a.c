void* malloc(int size);
void free(void *ptr);
int puts(char *str);

char* func(char *x);

int main(int argc, char **argv) {

    char string[] = "Hello, world!\n";
    puts(string);

    return 0;
}

char* func(char *x) {
    return x;
}
