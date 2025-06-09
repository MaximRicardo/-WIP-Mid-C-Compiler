int func(int x, int y);
void foo(void);

int main(int argc) {
    int x = 10+5;
    :;
    {
        int y = x+10;
        x = x+y;
    }
    char c = func(100, 3);
    :;
    foo();

    return 0;
}

int func(int x, int y) {
    return x*y;
}

void foo(void) {
    return;
}
