void* malloc(int size);
void free(void *ptr);

char* func(char *x);

int main(void) {

    char array[5];

    array[0] = 5;
    array[1] = 10;
    array[2] = 15;

    *(array+2);
    :;

    return 0;
}

char* func(char *x) {
    return x;
}
