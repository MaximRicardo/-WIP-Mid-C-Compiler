typedef char i8;

int strtol(char *str, char **end, int base);
/* a hacky way to get variadic functions working for now */
int printf();

i8 fibonacci(i8 n);

int main(int argc, char **argv) {

    char *end_ptr;
    i8 n;

    if (argc < 2) {
        printf("give an argument containing which fibonacci number to get.\n");
        return 1;
    }

    n = strtol(argv[1], &end_ptr, 0);
    printf("fibonacci nr. %d is: %d\n", n, fibonacci(n));

}

i8 fibonacci(i8 n) {

    i8 a = 0;
    i8 b = 1;
    i8 c;

    i8 i = 0;

    if (n < 2)
        return n;

    for (i = 0; i < n-1; i++) {
        c = a+b;
        a=b;
        b=c;
    }

    return c;

}
