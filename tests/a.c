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
    func(5, 10);
    :;
}
