void* malloc(int size);
void free(void *ptr);
/* a hacky way to get variadic functions working for now */
int printf();

char* func(char *x);

int main(int argc, char **argv) {

    int i = 0;

    while (i <= 10 && i < 20) 
        printf("Hello, world! i = %d\n", i, i = i + 1);

    return 0;
}

char* func(char *x) {
    return x;
}
