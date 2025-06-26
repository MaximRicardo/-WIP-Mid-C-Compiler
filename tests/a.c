int main(int argc, char **argv) {

    int x = 5;
    int y = 42;

    if (0) {
        y = y + x;
        x = 1;
    }
    else {
        y = y - x;
        x = 2;
    }

    return y+x;

}
