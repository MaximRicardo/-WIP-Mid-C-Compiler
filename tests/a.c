void* malloc(int size);
void free(void *ptr);
/* a hacky way to get variadic functions working for now */
int printf();

char* func(char *x);

int main(int argc, char **argv) {

    printf("Hello, world!\n");

    return 0;
}

char* func(char *x) {
    return x;
}
