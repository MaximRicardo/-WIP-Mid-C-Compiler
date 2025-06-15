void* malloc(int size);
void free(void *ptr);

char* func(char *x);

int main(void) {

    char array[5] = {1, 2, 3};

    *(array+2);
    :;

    return 0;
}

char* func(char *x) {
    return x;
}
