typedef int i32;

int strtol(char *str, char **end, int base);
/* a hacky way to get variadic functions working for now */
int printf();

i32 fibonacci(i32 n);

int main(int argc, char **argv) {

    char *end_ptr;
    i32 n;

    if (argc < 2) {
        printf("give an argument containing which fibonacci number to get.\n");
        return 1;
    }

    n = strtol(argv[1], &end_ptr, 0);
    printf("fibonacci nr. %d is: %d\n", n, fibonacci(n));

}

i32 fibonacci(i32 n) {

    i32 a = 0;
    i32 b = 1;
    i32 c;

    i32 i = 0;

    if (n < 2)
        return n;

    for (i = 0; i < n-1; i++) {
        c = a+b;
        a=b;
        b=c;
    }

    return c;

}
