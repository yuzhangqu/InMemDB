#include "server.h"

using std::unordered_map;
using std::string;

struct ops {
    public:
        int n;
        xxop *op;
        int *key;
        int *key2;
        ops(int i) {
            n = i;
            op = new xxop[n];
            key = new int[n];
            key2 = new int[n];
        }
        ~ops() {
            delete[] op;
            delete[] key;
            delete[] key2;
        }
};

struct res {
    public:
        int n;
        int *key;
        string *re;
        res(int i) {
            n = i;
            key = new int[n];
            re = new string[n];
        }
        ~res() {
            delete[] key;
            delete[] re;
        }
};

unordered_map<int, string> db;

void initDB() {
    for (int i = 1; i < 50 * NUM_WORKERS; i++) {
        db[i] = "abcdefg";
    }
}

struct res *ret = new res(1);

void *worker_start(void *arg) {
    struct ops *myops = (struct ops*)arg;
    ret->key[0] = myops->key[0];
    ret->re[0] = db[myops->key[0]];
    pthread_exit((void*)ret);
}

int main(int argc, char *argv[]) {
    initDB();
    int rc; //Store any return value about pthread
    void *worker_result;
    pthread_t workers[NUM_WORKERS];
    //pthread_t handles[NUM_HANDLES];
    pthread_attr_t worker_attr;
    //pthread_attr_t handle_attr;
    //int fifo_max_prio, fifo_min_prio;
    //struct sched_param fifo_param;

    //initialize and set worker thread joinable and FIFO...
    pthread_attr_init(&worker_attr);
    //pthread_attr_setinheritsched(&worker_attr, PTHREAD_EXPLICIT_SCHED);
    //pthread_attr_setschedpolicy(&worker_attr, SCHED_FIFO);
    //fifo_max_prio = sched_get_priority_max(SCHED_FIFO);
    //fifo_min_prio = sched_get_priority_min(SCHED_FIFO);
    //fifo_param.sched_priority = (fifo_max_prio + fifo_min_prio) / 2;
    //pthread_attr_setschedparam(&worker_attr, &fifo_param);
    pthread_attr_setdetachstate(&worker_attr, PTHREAD_CREATE_JOINABLE);

    struct ops *test = new ops(1);
    test->op[0] = GET;
    test->key[0] = 2;

    rc = pthread_create(&workers[0], &worker_attr, worker_start, (void *)test);
    if (rc) {
        printf("ERROR: worker init failed. %s\n", strerror(rc));
        exit(-1);
    }
    pthread_attr_destroy(&worker_attr);
    rc = pthread_join(workers[0], &worker_result);
    if (rc) {
        printf("Worker fail to end.\n");
        exit(-1);
    }
    printf("SUCCESS! The content is: %s\n", (((struct res*)worker_result)->re[0]).c_str());
    delete test;
    delete ret;
    pthread_exit(NULL);
}
