#include <stdio.h>
#include <string.h>
#include <pthread.h>



typedef struct {

    int pid;

} proc_t;

 

typedef struct {

    proc_t *proc_info;

} thread_info_t; 

 

proc_t p;

thread_info_t th;

thread_info_t *thd;

 

void *thread1(void *arg) {

    printf("%d\n", thd->proc_info->pid);

    return NULL;

}

 

void *thread2(void *arg) {

    thd->proc_info = NULL;

    return NULL;

}

 

int main(int argc, char *argv[]) {                    

    thread_info_t t;

    p.pid = 100;

    t.proc_info = &p;

    thd = &t;

 

    pthread_t p1, p2;

    printf("main: begin\n");

    pthread_create(&p1, NULL, thread1, NULL); 

    pthread_create(&p2, NULL, thread2, NULL);

    

    return 0;

}
