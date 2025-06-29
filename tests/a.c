int main(int argc, char **argv) {

    int x = 5;
    int y = 42;
    int z = 10;

    if (1) {
        int z = 5;
        y = y / x;
        x = z;
    }
    else if (1) {
        int z = 5;
        y = y - x;
        x = 2;
    }
    else {
        y = 10;
        x = 5;
    }

    return x+y;

}
