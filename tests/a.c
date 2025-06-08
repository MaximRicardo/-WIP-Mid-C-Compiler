int func(int x, int z);

int main(int argc) {
    int x = 10+5;
    :;
    {
        int y = x+10;
        x = x+y;
    }
    char c = func(100, 3);
    :;

    return 0;
}

int func(int x, int y) {
    return x*y;
}
