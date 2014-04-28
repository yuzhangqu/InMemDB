#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_WORKERS 8

void *worker_start(void *arg) {
    printf("Worker start: %d\n", *(int*)arg);
}

int main(int argc, char *argv[]) {
    int res;
    void *worker_result;
    pthread_t workers[NUM_WORKERS];
    int i;
    for (i = 0; i < NUM_WORKERS; i++) {
        res = pthread_create(&(workers[i]), NULL, worker_start, (void*)&i);
        if (res != 0) {
            perror("worker init failed.");
            exit(EXIT_FAILURE);
        }
    }
    printf("Hello World!\n");
    for (i = 0; i < NUM_WORKERS; i++) {
        res = pthread_join(workers[i], &worker_result);
        if (res == 0) {
            printf("Jobs done.\n");
        }
        else {
            printf("Worker fail to end.\n");
        }
    }
    printf("All workers done.\n");
    exit(EXIT_SUCCESS);
    return 0;
}
