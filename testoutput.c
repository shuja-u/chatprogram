#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *mythread(void *arg) {
    sleep(5);
    //FILE *testfile;
    //testfile = stdin;
    
    //fflush(stdin);
    printf("\r\nINCOMING MESSAGE\nhi, i have characters\nEnter message: ");
    fflush(stdout);
    return NULL;
}

int main(void)
{
    //char buffer[BUFSIZ];
    //if (setvbuf(stdin, buffer, _IONBF, sizeof(buffer)) != 0) {
    //    printf("buffer error");
    //}
    pthread_t p1;
    char message[BUFSIZ];
    printf("Enter message: ");
    fflush(stdout);
    //sleep(5);
    if((pthread_create(&p1, NULL, mythread, "A")) != 0) {
        printf("ERROR: pthread\n");
    }
    fgets(message, BUFSIZ, stdin);
    pthread_join(p1, NULL);
    printf("%s", message);
}