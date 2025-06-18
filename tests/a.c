typedef uint u32;

int strtol(char *str, char **end, int base);
int printf(char *str, ...);

u32 fibonacci(u32 n);

int main(int argc, char **argv) {

    char *end_ptr;
    u32 n;

    if (argc < 2) {
        printf("give an argument containing which fibonacci number to get.\n");
        return 1;
    }

    n = strtol(argv[1], &end_ptr, 0);
    printf("fibonacci nr. %d is: %d\n", n, fibonacci(n));

}

u32 fibonacci(u32 n) {

    u32 a = 0;
    u32 b = 1;
    u32 c;

    u32 i = 0;

    if (n < 2)
        return n;

    for (i = 0; i < n-1; i++) {
        c = a+b;
        a=b;
        b=c;
    }

    return c;

}
