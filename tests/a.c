int func(int x, int y);
void foo(void);

int main(int argc) {
    argc;
    :;
    int x = 10+5;
    :;
    {
        int y = x+10;
        x = x+y;
    }
    char c = func(100, 3);
    :;
    if (0) {
        x-5;
    }
    else if (1) {
        x-10;
    }
    else if (1) {
        x-15;
    }
    else {
        x-20;
    }
    :;

    return 0;
}

int func(int x, int y) {
    return x*y;
}

void foo(void) {
    return;
}
