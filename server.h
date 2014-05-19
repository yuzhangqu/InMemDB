#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <queue>
#include <vector>
#include <unordered_map>
#include "TransMessage.pb.h"
#include "TransMessage.h"

#define NUM_WORKERS 8
#define NUM_PARTITION 50

enum xxop {
    GET,
    PUT,
    GETRANGE
};

