#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}
#else

#include <editline/history.h>

#endif

int main() {
    puts("Connery version 0.0.1");
    puts("Press Ctrl + c to exit\n");

    while (1) {
        char* input = readline("connery> ");
        add_history(input);

        printf("Bond James, %s\n", input);

        free(input);
    }

    return 0;
}
