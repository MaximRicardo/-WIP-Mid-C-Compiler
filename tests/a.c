int func(int x, int y) {
    x*y;
}

int main(int argc) {
    int x = 10+5;
    :;
    {
        int y = x+10;
        x = x+y;
    }
    func(~func(100,2), func(2, 5));
    :;
}
