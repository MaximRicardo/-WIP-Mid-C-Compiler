void* malloc(int size);
void free(void *ptr);

char* func(char *x);

int main(void) {

    char *ptr = malloc(4);

    *ptr = 10;
    *func(ptr);
    :;

    free(ptr);

    return 0;
}

char* func(char *x) {
    return x;
}
