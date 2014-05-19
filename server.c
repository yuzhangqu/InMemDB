#include "server.h"

using std::unordered_map;
using std::string;
using std::queue;
using std::vector;

pthread_t workers[NUM_WORKERS];
pthread_attr_t worker_attr;
pthread_mutex_t locks[NUM_WORKERS];

unordered_map<int, string> db; //DataBase

//op is the structure of single operation in the transaction.
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

// ret is the structure of single key-value pair
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

// Init DB, for test only.
void initDB() {
    for (int i = 1; i <= NUM_PARTITION * NUM_WORKERS; i++) {
        db[i] = "Initial Data";
    }
}

// Map key to the partition #
int getParti(int key) {
    if (key < 1) {
        return 1;
    }
    if (key > NUM_WORKERS * NUM_PARTITION) {
        return NUM_WORKERS;
    }
    return (key % NUM_PARTITION == 0 ? key / NUM_PARTITION : key / NUM_PARTITION + 1);
}

// The worker.
void *worker_start(void *arg) {
    queue<struct op> *myops = (queue<struct op> *)arg;
    queue<struct ret> *tmp = new queue<struct ret>();
    op cur;
    // Worker accesses the Database.
    while (!myops->empty()) {
        cur = myops->front();
        if (cur.code == GET) {
            if (db.count(cur.key) > 0) {
                tmp->push(ret(cur.key, db[cur.key]));
            }
        }
        else if (cur.code == PUT) {
            db[cur.key] = cur.value;
        }
        else if (cur.code == GETRANGE) {
            for (int i = cur.key; i <= cur.key2; i++) {
                if (db.count(i) > 0) {
                    tmp->push(ret(i, db[i]));
                }
            }
        }
        myops->pop();
    }
    pthread_exit((void*)tmp);
}

// The coordinator
gpb *dbaccess_1_svc(gpb *buf, struct svc_req *req) {
    string tmpstr (buf->data, buf->size);
    InMemDB::TransRequest tr;
    tr.ParseFromString(tmpstr);
    queue<struct op> *myops = new queue<struct op>();
    for (int i = 0; i < tr.op_size(); i++) {
        const InMemDB::TransRequest::Op& iter = tr.op(i);
        switch (iter.code()) {
            case InMemDB::TransRequest_Op_Code_GET:
                myops->push(op(GET, iter.key()));
                break;
            case InMemDB::TransRequest_Op_Code_PUT:
                myops->push(op(PUT, iter.key(), iter.value()));
                break;
            case InMemDB::TransRequest_Op_Code_GETRANGE:
                myops->push(op(GETRANGE, iter.key(), iter.key2()));
                break;
            default:
                break;
        }
    }
    queue<struct op> *packs = new queue<struct op>[NUM_WORKERS]; //Decompose the transaction into small parts, and store into the corresponding position in packs.
    vector<int> reg; //Remember which workers shall be used.
    void *worker_result;
    queue<struct ret> *tmp;
    queue<struct ret> info; //Store the result of each worker into this variable.
    InMemDB::TransResponse trsp;
    InMemDB::TransResponse::Ret *trsp_ret;
    op cur;
    ret aret;
    while (!myops->empty()) {
        cur = myops->front();
        if (cur.code == GETRANGE) { //GetRange may involve more than one partition.
            while (getParti(cur.key) < getParti(cur.key2)) {
                packs[getParti(cur.key) - 1].push(op(GETRANGE, cur.key, getParti(cur.key) * NUM_PARTITION));
                cur.key = getParti(cur.key) * NUM_PARTITION + 1;
            }
            packs[getParti(cur.key) - 1].push(op(GETRANGE, cur.key, cur.key2));
        }
        else {
            packs[getParti(cur.key) - 1].push(op(cur.code, cur.key, cur.value));
        }
        myops->pop();
    }
    int rc;
    //Phase 1: Get all locks.
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (!packs[i].empty()) {
            pthread_mutex_lock(&locks[i]);
            reg.push_back(i);
        }
    }
    //Phase 2: Start workers.
    for (size_t i = 0; i < reg.size(); i++) {
        rc = pthread_create(&workers[reg[i]], &worker_attr, worker_start, (void *)(packs + reg[i]));
        if (rc) {
            printf("ERROR: worker init failed. %s\n", strerror(rc));
        }
    }
    //Phase 3: Collect the results.
    for (size_t i = 0; i < reg.size(); i++) {
        rc = pthread_join(workers[reg[i]], &worker_result);
        if (rc) {
            printf("Worker fail to end.\n");
        }
        pthread_mutex_unlock(&locks[reg[i]]);
        tmp = (queue<struct ret> *)worker_result;
        while (!tmp->empty()) {
            trsp_ret = trsp.add_ret();
            aret = tmp->front();
            trsp_ret->set_key(aret.key);
            trsp_ret->set_value(aret.value);
            tmp->pop();
        }
        delete tmp;
    }
    delete myops;
    delete[] packs;
    string str_trsp;
    trsp.SerializeToString(&str_trsp);
    gpb *buf_ret = new gpb();
    buf_ret->size = str_trsp.size();
    buf_ret->data = (char*)malloc(str_trsp.size());
    memcpy(buf_ret->data, str_trsp.c_str(), str_trsp.size());
    return buf_ret;
}

//int main(int argc, char *argv[]) {
//    initDB();
//    int rc; //Store any return value about pthread
//    int fifo_max_prio, fifo_min_prio;
//    struct sched_param fifo_param;
//
//    //initialize and set worker thread joinable and FIFO...
//    pthread_attr_init(&worker_attr);
//    pthread_attr_setdetachstate(&worker_attr, PTHREAD_CREATE_JOINABLE);
//
//    pthread_attr_init(&handle_attr);
//    pthread_attr_setinheritsched(&handle_attr, PTHREAD_EXPLICIT_SCHED);
//    pthread_attr_setschedpolicy(&handle_attr, SCHED_FIFO);
//    fifo_max_prio = sched_get_priority_max(SCHED_FIFO);
//    fifo_min_prio = sched_get_priority_min(SCHED_FIFO);
//    fifo_param.sched_priority = (fifo_max_prio + fifo_min_prio) / 2;
//    pthread_attr_setschedparam(&handle_attr, &fifo_param);
//
//    //Construct a transaction, for test only.
//    queue<struct op> *trans = new queue<struct op>();
//    trans->push(op(PUT, 1, "0000"));
//    trans->push(op(PUT, 51, "5151"));
//    trans->push(op(GETRANGE, 1, 51));
//    
//    //Coordinator handle this transaction.
//    rc = pthread_create(&handles[0], &handle_attr, handle_start, (void *)trans);
//    if (rc) {
//        printf("ERROR: handle create fail.\n");
//        exit(-1);
//    }
//    rc = pthread_join(handles[0], NULL);
//    pthread_exit(NULL);
//}
