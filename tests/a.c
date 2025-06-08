char func(int x, int z);

int main(int argc) {
    int x = 10+5;
    :;
    {
        int y = x+10;
        x = x+y;
    }
    char c = func(4, 3);
    :;
}

char func(int x, int y) {
    x*y;
}
