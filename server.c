#include "server.h"

using std::unordered_map;
using std::string;
using std::queue;

unordered_map<int, string> db;

pthread_t workers[NUM_WORKERS];
pthread_t handles[10];
pthread_attr_t worker_attr;
pthread_mutex_t locks[NUM_WORKERS];

struct op {
    xxop code;
    int key;
    int key2;
    string value;
    op() {
    }
    op(xxop c, int k, string v = "") {
        code = c;
        key = k;
        key2 = 0;
        value = v;
    }
    op(xxop c, int k, int k2) {
        code = c;
        key = k;
        key2 = k2;
        value = "";
    }
    void show() {
        printf("code = %d, key = %d, key2 = %d, value = %s\n", code, key, key2, value.c_str());
    }
};

struct ret {
    int key;
    string value;
    ret() {
    }
    ret(int k, string v) {
        key = k;
        value = v;
    }
    void show() {
        printf("key = %d, Value = %s\n", key, value.c_str());
    }
};

void initDB() {
    for (int i = 1; i <= 50 * NUM_WORKERS; i++) {
        db[i] = "Initial Data";
    }
}

int getParti(int key) {
    return (key % 50 == 0 ? key / 50 : key / 50 + 1);
}

void *worker_start(void *arg) {
    queue<struct op> *myops = (queue<struct op> *)arg;
    queue<struct ret> *tmp = new queue<struct ret>();
    op cur;
    while (!myops->empty()) {
        cur = myops->front();
        if (cur.code == GET) {
            tmp->push(ret(cur.key, db[cur.key]));
        }
        else if (cur.code == PUT) {
            db[cur.key] = cur.value;
        }
        else if (cur.code == GETRANGE) {
            for (int i = cur.key; i <= cur.key2; i++) {
                tmp->push(ret(i, db[i]));
            }
        }
        myops->pop();
    }
    pthread_exit((void*)tmp);
}

void *handle_start(void *arg) {
    queue<struct op> *packs = new queue<struct op>[NUM_WORKERS];
    queue<struct op> *myops = (queue<struct op> *)arg;
    void *worker_result;
    queue<struct ret> *tmp;
    queue<struct ret> info;
    op cur;
    ret aret;
    while (!myops->empty()) {
        cur = myops->front();
        if (cur.code == GETRANGE) {
            while (getParti(cur.key) < getParti(cur.key2)) {
                packs[getParti(cur.key) - 1].push(op(GETRANGE, cur.key, getParti(cur.key) * 50));
                cur.key = getParti(cur.key) * 50 + 1;
            }
            packs[getParti(cur.key) - 1].push(op(GETRANGE, cur.key, cur.key2));
        }
        else {
            packs[getParti(cur.key) - 1].push(op(cur.code, cur.key, cur.value));
        }
        myops->pop();
    }
    int rc;
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (!packs[i].empty()) {
            pthread_mutex_lock(&locks[i]);
        }
    }
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (!packs[i].empty()) {
            rc = pthread_create(&workers[i], &worker_attr, worker_start, (void *)(packs + i));
            if (rc) {
                printf("ERROR: worker init failed. %s\n", strerror(rc));
                pthread_mutex_unlock(&locks[i]);
                exit(-1);
            }
        }
    }
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (!packs[i].empty()) {
            rc = pthread_join(workers[i], &worker_result);
            if (rc) {
                printf("Worker fail to end.\n");
                pthread_mutex_unlock(&locks[i]);
                exit(-1);
            }
            pthread_mutex_unlock(&locks[i]);
            tmp = (queue<struct ret> *)worker_result;
            while (!tmp->empty()) {
                aret = tmp->front();
                info.push(ret(aret.key, aret.value));
                tmp->pop();
            }
        }
    }
    while (!info.empty()) {
        info.front().show();
        info.pop();
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    initDB();
    int rc; //Store any return value about pthread
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

    queue<struct op> *trans = new queue<struct op>();
    trans->push(op(PUT, 1, "0000"));
    trans->push(op(PUT, 51, "5151"));
    trans->push(op(GETRANGE, 1, 51));
    
    rc = pthread_create(&handles[0], NULL, handle_start, (void *)trans);
    if (rc) {
        printf("ERROR: handle create fail.\n");
        exit(-1);
    }
    
    printf("SUCCESS!Handle start!\n");
    rc = pthread_join(handles[0], NULL);
    pthread_exit(NULL);
}
