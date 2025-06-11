int func(int x, int y);

int main(void) {

    void **p = 18;
    void **q = 14;
    *(p-q);
    :;

    return 0;
}

int* func(int x, int y) {
    return x*y;
}
