int main(int argc, char **argv) {

    int x = 5;
    int y = 42;

    if (1) {
        y = y / x;
        x = 1;
    }
    else if (0) {
        y = y - x;
        x = 2;
    }
    else {
        y = 10;
        x = 5;
    }

    return x+y;

}
