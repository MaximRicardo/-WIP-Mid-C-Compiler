int func(int x, int y);
void foo(void);

int main(void) {
    int x = 10+5;
    :;
    {
        int y = x+10;
        x = x+y;
    }
    char c = func(100, 3);
    :;
    if (1-1) {
        5-10;
        :;
    }

    return 0;
}

int func(int x, int y) {
    return x*y;
}

void foo(void) {
    return;
}
