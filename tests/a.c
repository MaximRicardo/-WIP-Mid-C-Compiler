#define m_test_macro "this is a macro"

typedef unsigned u32;

int strtol(char *str, char **end, int base);
int printf(char *str, ...);

static u32 fibonacci(u32 n);

struct TestStruct;

struct TestStruct {
    int x[4];
    int y;
    int *z;
    char a;
    int b;
};

struct Struct2ElectricBoogaloo;

void func(void) {
    struct Struct2ElectricBoogaloo var;
    struct Struct2ElectricBoogaloo *var_ptr = &var;
    var_ptr->x[2] = 5;
}

int main(int argc, char **argv) {

    char *end_ptr;
    u32 n;
    struct TestStruct x;
    x.x[2] = 5;

    if (argc < 2) {
        printf("give an argument containing which fibonacci number to get.\n");
        return 1;
    }

    n = strtol(argv[1], &end_ptr, 0);
    printf("fibonacci nr. %d is: %u\n", n, fibonacci(n));

    printf("m_test_macro = %s\n", m_test_macro);
    printf("x.x[2] = %d\n", x.x[2]);

}

static u32 fibonacci(u32 n) {

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
