#define m_big_num_start_capacity 128

#define true 1
#define false 0

#define NULL (void*)0

/* i haven't added sizeof yet */
#define m_sizeof_char 1

typedef unsigned u32;
typedef int bool;

int printf(char *str, ...);
int fprintf(void *file, char *str, ...);
void* calloc(unsigned long n_elems, unsigned long elem_size);
void* realloc(void *ptr, unsigned long size);
void free(void *pt);
void exit(int code);
void memcpy(void *dest, void *src, unsigned long n);
unsigned long strtoul(char *str, char **endptr, int base);

struct BigNum {

    char *digits; /* the number is stored as a raw non-terminated string */
    u32 n_digits;
    u32 capacity;

};

static void BigNum_init(struct BigNum *num) {

    num->capacity = m_big_num_start_capacity;
    num->n_digits = 0;
    num->digits = calloc(num->capacity, m_sizeof_char);

}

/* returns whether or not it failed. automatically converts the digit
 * to ASCII. */
static bool BigNum_append_digit(struct BigNum *num, int digit) {

    if (digit < 0 || digit > 9) {
        printf("invalid digit.\n");
        return false;
    }

    while (num->n_digits+1 >= num->capacity) {
        num->capacity = num->capacity*num->capacity;
        num->digits = realloc(num->digits, num->capacity*m_sizeof_char);
    }

    num->digits[num->n_digits] = '0'+digit;
    ++num->n_digits;

    return true;

}

static void BigNum_print(struct BigNum *self) {

    u32 i;

    if (self->n_digits == 0)
        return;

    for (i = self->n_digits-1; i < self->n_digits; i--)
        printf("%c", self->digits[i]);
    printf("\n");

}

static void BigNum_add(struct BigNum *result, struct BigNum *x,
        struct BigNum *y) {

    /* loop through every digit, add them together, put them into result, and
     * keep track of the carry */

    u32 i;
    u32 n;
    int carry = 0;

    /* no ternary operator yet */
    if (result->n_digits >= x->n_digits && result->n_digits >= y->n_digits)
        n = result->n_digits;
    else if (x->n_digits >= y->n_digits)
        n = x->n_digits;
    else
        n = y->n_digits;

    for (i = 0; i < n; ++i) {
        int x_digit = 0;
        int y_digit = 0;
        int sum;

        printf("i = %u.\n", i);

        if (i < x->n_digits)
            x_digit = x->digits[i]-'0';
        if (i < y->n_digits)
            y_digit = y->digits[i]-'0';

        printf("x = %d, y = %d.\n", x_digit, y_digit);

        sum = x_digit + y_digit + carry;
        carry = 0;
        if (sum > 9) {
            carry = 1;
            sum = sum - 10;
        }

        printf("sum = %d, carry = %d.\n", sum, carry);

        if (i < result->n_digits)
            result->digits[i] = '0'+sum;
        else
            BigNum_append_digit(result, sum);
    }

    if (carry > 0)
        BigNum_append_digit(result, carry);

}

static void BigNum_copy(struct BigNum *dest, struct BigNum *src) {

    if (src->n_digits >= dest->capacity) {
        dest->capacity = src->capacity;
        dest->digits = realloc(dest->digits, dest->capacity*m_sizeof_char);
    }
    dest->n_digits = src->n_digits;
    memcpy(dest->digits, src->digits, dest->n_digits*m_sizeof_char);

}

static void BigNum_free(struct BigNum *self) {

    free(self->digits);
    self->digits = NULL;
    self->n_digits = 0;
    self->capacity = 0;

}

/* assumes result, a and b have only been initialized with no characters
 * inserted. */
static void big_num_fibonacci(struct BigNum *result, struct BigNum *a,
        struct BigNum *b, u32 n);

int main(int argc, char **argv) {

    char *end_ptr;
    u32 n;

    struct BigNum a;
    struct BigNum b;
    struct BigNum result;
    BigNum_init(&a);
    BigNum_init(&b);
    BigNum_init(&result);

    printf("MCC version %u.%u\n", __MID_CC__, __MID_CC_MINOR__);
    printf("this is being printed from '%s'.\n", __FILE__);

    if (argc < 2) {
        printf("give an argument containing which fibonacci number to get.\n");
        return 1;
    }

    n = strtoul(argv[1], &end_ptr, 0);

    big_num_fibonacci(&result, &a, &b, n);

    printf("fibonacci nr. %u =\n", n);
    BigNum_print(&result);

    BigNum_free(&a);
    BigNum_free(&b);
    BigNum_free(&result);

    return 0;

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

static void big_num_fibonacci(struct BigNum *result, struct BigNum *a,
        struct BigNum *b, u32 n) {

    u32 i;

    if (n < 2) {
        BigNum_append_digit(result, n);
        return;
    }

    BigNum_append_digit(a, 0);
    BigNum_append_digit(b, 1);

    for (i = 0; i < n-1; i++) {

        printf("\n\n\nnew loop\n\n\n\n");
        BigNum_add(result, a, b);
        BigNum_copy(a, b);
        BigNum_copy(b, result);

    }

}
