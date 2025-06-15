void* malloc(int size);
void free(void *ptr);

char* func(char *x);

int main(int argc, char **argv) {

    int array[3] = {1, 2, 3};

    *(array+2);
    :;

    return 0;
}

char* func(char *x) {
    return x;
}
