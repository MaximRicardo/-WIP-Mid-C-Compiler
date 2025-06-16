void* malloc(int size);
void free(void *ptr);
int puts(char *str);

char* func(char *x);

int main(int argc, char **argv) {

    puts({'h', 'e', 'l', 'l', 'o', '\0'});

    return 0;
}

char* func(char *x) {
    return x;
}
