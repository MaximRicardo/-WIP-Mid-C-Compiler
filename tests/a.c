int* func(int *x);

int main(void) {

    int n = 5;
    int *x = &n;

    *func(x);
    :;

    return 0;
}

int* func(int *x) {
    return x;
}
