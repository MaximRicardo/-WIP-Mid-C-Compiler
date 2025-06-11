void func(int *x, int *y);

int main(void) {

    int x = 5;
    int y = 5;

    func(&x, &y);
    x;
    :;
    func(&x, &y);
    x;
    :;
    func(&x, &y);
    x;
    :;
    func(&x, &y);
    x;
    :;

    return 0;
}

void func(int *x, int *y) {
    *x = *x + *y;
}
