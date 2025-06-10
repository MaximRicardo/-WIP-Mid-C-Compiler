int func(int x, int y);

int main(void) {

    char *c = func(5,2);
    c+2;
    :;

    return 0;
}

int* func(int x, int y) {
    return x*y;
}
