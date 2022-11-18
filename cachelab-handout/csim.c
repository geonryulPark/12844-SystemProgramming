#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

typedef unsigned long long int mem_add;
typedef struct cache_line_t {
    bool valid;
    unsigned long long int lru;
    mem_add tag;

} cache_line_t;
typedef cache_line_t *cache_set_t;
typedef cache_set_t *cache_t;

int s = 0, S;
int b = 0, B;
int E = 0;
int verbose;
int setIndexMask;
int lruCounter = 0;

FILE *trace = NULL;
char *trace_file_name = NULL;
cache_t cache;

unsigned int hit = 0;
unsigned int miss = 0;
unsigned int evict = 0;

void setParameter(int argc, char *argv[]); // s, E, b setting
void accessCache(mem_add address, char command, int blockSize);
void initSimulate(void);

int main(int argc, char *argv[]) {
    setParameter(argc, argv);
    initSimulate();
    printSummary(hit, miss, evict);
    return 0;
}

void setParameter(int argc, char *argv[]) {
    char c;

    while ((c = getopt(argc, argv, "vh:s:E:b:t:")) != -1) {

        switch (c) {
        case 'v': verbose = 1; break;
        case 's': s = atoi(optarg); break;
        case 'E': E = atoi(optarg); break;
        case 'b': b = atoi(optarg); break;
        case 't': trace_file_name = optarg; break;
        default: break;
        }
    }


//    1111
    setIndexMask = (1 << s) - 1;

//    10000
    S = (1 << s);
    B = (1 << b);

    cache = (cache_t) malloc(sizeof(cache_set_t) * S);
    for (int i = 0; i < S; i++) {
        cache[i] = (cache_set_t) malloc(sizeof(cache_line_t) * E);
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = false;
            cache[i][j].tag = 0;
            cache[i][j].lru = 0;
        }
    }
}

void initSimulate(void) {
    trace = fopen(trace_file_name, "r");
    char command;
    mem_add address;
    int blockSize;
    while (fscanf(trace, " %c %llx,%d", &command, &address, &blockSize) != EOF) {
        switch (command) {
            case 'L':
                accessCache(address, command, blockSize);
                break;
            case 'M':
                accessCache(address, command, blockSize);
                accessCache(address, command, blockSize);
                break;
            case 'S':
                accessCache(address, command, blockSize);
                break;
            default:
                break;
        }
    }

    fclose(trace);
}

void accessCache(mem_add address, char command, int blockSize) {
    if (verbose == 0) {
        mem_add setIndex = (address >> b) & setIndexMask;
        mem_add tag = address >> (s + b);

        for (int i = 0; i < E; i++) {
            if (cache[setIndex][i].valid) {
                if (cache[setIndex][i].tag == tag) {
                    cache[setIndex][i].lru = lruCounter++;
                    hit++;
                    return;
                }
            }
        }
        // 아래부터는 miss 발생한 로직
        miss++;

        unsigned long long int minLRU = LONG_MAX;
        int replaceLine;

        for (int j = 0; j < E; j++) {
            if (cache[setIndex][j].lru < minLRU) {
                replaceLine = j;
                minLRU = cache[setIndex][j].lru;
            }
        }

        if (cache[setIndex][replaceLine].valid) {
            evict++;
        }

        cache[setIndex][replaceLine].valid = true;
        cache[setIndex][replaceLine].tag = tag;
        cache[setIndex][replaceLine].lru = lruCounter++;
    } else {
        mem_add setIndex = (address >> b) & setIndexMask;
        mem_add tag = address >> (s + b);

        for (int i = 0; i < E; i++) {
            if (cache[setIndex][i].valid) {
                if (cache[setIndex][i].tag == tag) {
                    cache[setIndex][i].lru = lruCounter++;
                    hit++;
                    printf("%c %llx, %d hit\n", command, address, blockSize);
                    return;
                }
            }
        }
        // 아래부터는 miss 발생한 로직
        miss++;
        printf("%c %llx, %d miss\n", command, address, blockSize);

        unsigned long long int minLRU = LONG_MAX;
        int replaceLine;

        for (int j = 0; j < E; j++) {
            if (cache[setIndex][j].lru < minLRU) {
                replaceLine = j;
                minLRU = cache[setIndex][j].lru;
            }
        }

        if (cache[setIndex][replaceLine].valid) {
            printf("%c %llx, %d eviction\n", command, address, blockSize);
            evict++;
        }

        cache[setIndex][replaceLine].valid = true;
        cache[setIndex][replaceLine].tag = tag;
        cache[setIndex][replaceLine].lru = lruCounter++;
    }

}
