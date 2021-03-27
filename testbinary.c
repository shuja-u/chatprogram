#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct userinfo {
    char username[50];
    char password[100];
    struct userinfo *next;
} userinfo;

userinfo * users[100]; //hash set of users

unsigned int hash(char *username)
{
    int length = strnlen(username, 50);
    printf("here\n");
    unsigned int hash = 0;

    for (int i = 0; i < length; i++)
    {
        hash += username[i];
        hash = (hash * username[i]) % 100;
    }
    return hash;
}

void init_hash_set(userinfo **users)
{
    for (int i = 0; i < 100; i++)
    {
        //userinfo *user = malloc(sizeof(userinfo));
        //sprintf(user->username, "No. %d", i);
        //sprintf(user->password, "Pw %d", i);
        //users[i] = user;
        users[i] = NULL;
    }
}

int add(userinfo **users, userinfo *user)
{
    if (user == NULL)
    {
        printf("User is NULL\n");
        return -1;
    }

    int index = hash(user->username);

    if(users[index] == NULL)
    {
        users[index] = user;
        return 1;
    }

    userinfo * current_node = users[index];

    while (current_node != NULL)
    {
        if (strncmp(user->username, current_node->username, 50) == 0)
        {
            return 0;
        }
        current_node = current_node->next;
    }

    user->next = users[index];
    users[index] = user;
    return 1;
}

int delete(userinfo **users, userinfo *user)
{
    if (user == NULL)
    {
        printf("User is NULL\n");
        return -1;
    }

    int index = hash(user->username);

    if(users[index] == NULL)
    {
        printf("User doesn't exist.\n");
        return 0;
    }

    userinfo * current_node = users[index];
    userinfo * previous_node = NULL;

    while (current_node != NULL && strncmp(current_node->username, user->username, 50) != 0)
    {
        previous_node = current_node;
        current_node = current_node->next;        
    }

    if (previous_node == NULL)
    {
        users[index] = NULL;
    }
    else
    {
        previous_node->next = current_node->next;
    }
    return 1;
}


int main(void)
{
    init_hash_set(users);

    userinfo *user = malloc(sizeof(userinfo));
    strncpy(user->username, "shuja", 50);
    strncpy(user->password, "mypassword", 100);

    printf("main %s\n", user->username);

    if(add(users, user) == 1)
    {
        printf("Add successful\n");
    }

    for (int i = 0; i < 100; i++)
    {
        if(users[i] == NULL)
        {
            printf("EMPTY\n");
            continue;
        }
        printf("%s | %s\n", users[i]->username, users[i]->password);
    }

    if(delete(users, user))
    {
        printf("Delete successful\n");
    }

        for (int i = 0; i < 100; i++)
    {
        if(users[i] == NULL)
        {
            printf("EMPTY\n");
            continue;
        }
        printf("%s | %s\n", users[i]->username, users[i]->password);
    }

    for (int i = 0; i < 100; i++)
    {
        if(users[i] == NULL)
        {
            continue;
        }
        free(users[i]);
    }

    // FILE *test;

    // test = fopen("output", "ab+");

    // if (test == NULL) {
    //     printf("File error.\n");
    //     return 0;
    // }

    // long position = ftell(test);

    // printf("Append position: %ld\n", position);

    // fseek(test, 0, SEEK_END);
    // position = ftell(test);
    // printf("File size: %ld\n", position);

    // rewind(test);

    // position = ftell(test);
    // printf("After rewind position: %ld\n", position);

    // userinfo user;

    // while(fread(&user, sizeof(userinfo), 1, test)) {
    //     printf("%s\n", user.username);
    //     printf("%s\n", user.password);
    // }

    // position = ftell(test);
    // printf("After read position: %ld\n", position);

    // strcpy(user.username, "anotheruser");
    // strcpy(user.password, "amigonamakeit");
    // fwrite(&user, sizeof(userinfo), 1, test);

    // struct UserInfo user1;

    // // strcpy(user1.username, "shuja");
    // // strcpy(user1.password, "testing");
    // // fwrite(&user1, sizeof userinfo, 1, test);

    // fread(&user1, sizeof userinfo, 1, test);
    // printf("%s\n", user1.username);
    // printf("%s\n", user1.password);

    // struct UserInfo user2;

    // // strcpy(user2.username, "matey");
    // // strcpy(user2.password, "gugli");
    // // fwrite(&user2, sizeof userinfo, 1, test);

    // fread(&user2, sizeof userinfo, 1, test);
    // printf("%s\n", user2.username);
    // printf("%s\n", user2.password);

    // struct UserInfo user3;

    // // strcpy(user3.username, "blahblah");
    // // strcpy(user3.password, "idonthaveaname");
    // // fwrite(&user3, sizeof userinfo, 1, test);

    // fread(&user3, sizeof userinfo, 1, test);
    // printf("%s\n", user3.username);
    // printf("%s\n", user3.password);

    //fclose(test);
}