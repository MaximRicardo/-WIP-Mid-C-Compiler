int func(int x) {
    x+5;
}

int main(int argc) {
    int x = 10+5;
    {
        int y = x+10;
        y/x;
        :;
    }
    x*5;
    :;
    return 0;
}
