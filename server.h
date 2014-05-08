#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <queue>
#include <vector>
#include <unordered_map>

#define NUM_WORKERS 8
#define NUM_HANDLES 10

enum xxop {
    GET,
    PUT,
    GETRANGE
};
