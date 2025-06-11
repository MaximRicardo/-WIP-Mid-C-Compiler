int* func(int *x);

int main(void) {

    int n = 5;
    int *x = &n;

    if (1) {

    }
    else {

    }

    *func(x);
    :;

    return 0;
}

int* func(int *x) {
    return x;
}
