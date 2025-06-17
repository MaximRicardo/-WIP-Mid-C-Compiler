int strtol(char *str, char **end, int base);
/* a hacky way to get variadic functions working for now */
int printf();

int fibonacci(char n, char test);

int main(int argc, char **argv) {

    char *end_ptr;
    int n;

    if (argc < 2) {
        printf("give an argument containing which fibonacci number to get.\n");
        return 1;
    }

    n = strtol(argv[1], &end_ptr, 0);
    printf("fibonacci nr. %d is: %d\n", n, fibonacci(n, n));

}

int fibonacci(char n, char test) {

    int a = 0;
    int b = 1;
    int c;

    int i = 0;

    test;

    if (n < 2)
        return n;

    for (i = 0; i < n-1; i++) {
        c = a+b;
        a=b;
        b=c;
    }

    return c;

}
