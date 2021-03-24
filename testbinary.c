#include <stdio.h>
#include <string.h>

int main(void)
{
    FILE *test;

    struct UserInfo {
        char *username;
        char *password;
    } userinfo;

    struct UserList {
        
    }

    test = fopen("output", "wb+");

    if (test == NULL) {
        printf("File error.\n");
        return 0;
    }

    struct UserInfo user1;

    user1.username = "shuja";
    user1.password = "testing";

    fwrite(&user1, sizeof(userinfo), 1, test);
    fclose(test);

    FILE *readtest;

    readtest = fopen("output", "rb");

    struct UserInfo user2;

    fread(&user2, sizeof(userinfo), 1, readtest);

    printf("hi\n");
    printf("%s\n", user2.username);
    printf("%s\n", user2.password);
}