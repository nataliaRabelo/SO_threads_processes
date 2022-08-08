//Natalia: para compilar tem que usar "gcc at1.c -lpthread -o trabalho"

#include <pthread.h>
#include <stdio.h>
void *printNameAndId(void *threadid)
{
    long tid;
    tid = (long)threadid;
    printf("Eu sou a thread_%ld e meu ID é %ld\n", tid, pthread_self());
    pthread_exit(NULL);
}
int main (int argc, char *argv[])
{
    int n;
    scanf("%d", &n);
    pthread_t threads[n];
    int rc;
    long t;
    for(t=0; t<n; t++){
    rc = pthread_create(&threads[t], NULL, printNameAndId, (void *)t);
    if (rc){
    printf("ERROR; return code from pthread_create() is %d\n", rc);
}
}
pthread_exit(NULL);
}