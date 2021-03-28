#include <stdio.h>
#include <string.h>

int main(void)
{
    char input[50];
    // fgets(input, 50, stdin);
    // printf("length of input: %ld\n", strlen(input));
    // input[strlen(input) - 1] = '\0';
    scanf("%s", input);

    if(strcmp(input, "hi") == 0)
    {
        printf("it matches\n");
    }
}