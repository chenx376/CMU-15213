/* Yue Zhao (yzhao6)
 * Implementation description:
 * I use the recommended data structure from recitation for simulation.
 * I set up missCounter, hitCounter, evictCounter to store results.
 * In the main methods, I first get parameters from input, then allocate memory
 * to initialize a cache for simulation. Then open a trace file, and deal with
 * the opeartions line by line. First, I try to find if it's a 'miss', if not,
 * change the LRU counter, update hit number; if so, update miss number, try to
 * judge if it is a eviction, if there is empty place in cache, set the cache
 * line; if not, find the mininum in LRU counter, evict that cache line. 
 * Finally, close the trace file, print summary and free the memory allocation.
 *
 * Language: C 
 */ 
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/* Structure for each cache line (reference: recitation slides). 
 * tag -  tag bits in each cache line
 * validBit -  one bit for the status of the cache line
 * counterLRU - the counter using least recently used policy
 */
typedef struct {
unsigned long tag;
    int validBit;
    int counterLRU;
} CacheLine;

/* Global variables. 
 * missCounter: store the number of misses
 * hitCounter: store the number of hits
 * evictCounter: store the number of evicts
 */
int missCounter, hitCounter, evictCounter;

/* 
 * init - initialize a two dimentinal array with type CacheLine as a cache
 *
 * s - number of set index bits
 * E - number of lines per set
 */
CacheLine** init(int s, int E) {
    unsigned long setNum = 1 << s;
    CacheLine** c;
    c = (CacheLine**) calloc(setNum, sizeof(CacheLine*));
    if (c == NULL) {
        printf("Calloc fails in memmory allocation.");
        exit(1);
    }
    unsigned i;

    for (i = 0; i < setNum; i++) {
        c[i] = (CacheLine*) calloc(E, sizeof(CacheLine));
        if (c[i] == NULL) {
            printf("Calloc fails in memmory allocation.");
            exit(1);
        }
    }
    return c;
}

/* 
 * freeCache - free memory usage after simulation.
 *
 * c - the cache for simulation
 * s - number of set index bits
 * E - number of lines per set
 */
void freeCache(CacheLine** c, int s, int E) {
    unsigned long setNum = 1 << s;
    unsigned i;
    for (i = 0; i < setNum; i++) {
        if (c[i] != NULL) {
            free(c[i]);
        }
    }
    if (c != NULL) {
        free(c);
    }
}

/*
 * findMax - Return current max number of counterLRU in the same cache set
 *
 * c - the cache for simulation
 * setNumber - Index of current set
 * E - number of lines per set 
 */ 
int findMax (CacheLine** c, unsigned long setNumber, int E) {
    int i;
    int max = 0;
    for (i = 0; i < E; i++) {
        if (c[setNumber][i].counterLRU > max) {
            max = c[setNumber][i].counterLRU;
        }
    }
    return max;
}

/*
 * isMiss - Return 1 if current access is a Miss,
 *  return 0 if it's a hit and update the cache line's counterLRU
 *
 * c - the cache for simulation
 * setNumber - Index of current set
 * tagNumber - tag bits of current data
 * E - number of lines per set  
 */ 
int isMiss(CacheLine** c, unsigned long setNumber,
        unsigned long tagNumber, int E) {
    int i;
    for (i = 0; i < E; i++) {
        if (c[setNumber][i].validBit == 1 
                && c[setNumber][i].tag == tagNumber) {
            c[setNumber][i].counterLRU = findMax(c, setNumber, E) + 1;
            return 0;
        }
    }
    return 1;
}

/*
 * ifFull - return 1 is current set is full,
 *  return 0 if there are enough space and update the cache line.
 *
 * c - the cache for simulation
 * setNumber - Index of current set
 * tagNumber - tag bits of current data
 * E - number of lines per set  
 */
int isFull(CacheLine** c, unsigned long setNumber,
        unsigned long tagNumber, int E) {
    int i;
    for (i = 0; i < E; i++) {
        if (c[setNumber][i].validBit == 0) {
            c[setNumber][i].validBit = 1;
            c[setNumber][i].tag = tagNumber;
            c[setNumber][i].counterLRU = findMax(c, setNumber, E) + 1;
            return 0;
        }
    }
    return 1;
}

/*
 * evictData -  when there in no empty cache line in current set,
 * use LRU to find a cache line to store the new data.
 *
 * c - the cache for simulation
 * setNumber - Index of current set
 * tagNumber - tag bits of current data
 * E - number of lines per set  
 */
void evictData(CacheLine** c, unsigned long setNumber,
        unsigned long tagNumber, int E) {
    int minAccess = c[setNumber][0].counterLRU;
    int minIndex = 0;
    int i;
    for (i = 1; i < E; i++) {
        if (c[setNumber][i].counterLRU < minAccess) {
            minAccess = c[setNumber][i].counterLRU;
            minIndex = i;
        }
    }
    c[setNumber][minIndex].tag = tagNumber;
    c[setNumber][minIndex].counterLRU = findMax(c, setNumber, E) + 1;
}

/*
 * simulator - deal with 'L' and 'S' operation. 
 * Check the whlole set to match the tag number, if after the operation, it is
 * a miss, increment 'missCounter'. Then judge if the set is full, if not,
 * put the data in the next empty cache line, change the cache line's 
 * counterLRU to the max value; if the set is full, by checking all values in 
 * countLRU in the set, evict the least recently used data with the minimum 
 * countLRU, change the cache line's counterLRU to the max value, increment
 * 'evictCounter'. 
 * If after the operation, it is not a miss, change the cacheline's counterLRU
 * to the max value, increment 'hitCounter'.
 *
 * c - the cache for simulation
 * setNumber - Index of current set
 * tagNumber - tag bits of current data
 * E - number of lines per set
 * verbose - flag for verbose mode
 */
void simulator(CacheLine** c, unsigned long setNumber,
        unsigned long tagNumber, int E, int verbose) {
    if (isMiss(c, setNumber, tagNumber, E) == 1) {
        if (verbose == 1) {
            printf("miss");
        }
        missCounter++;
        if (isFull(c, setNumber, tagNumber, E) == 1) {
            if (verbose == 1) {
                printf(" eviction");
            }
            evictData(c, setNumber, tagNumber, E);
            evictCounter++;
        }
        if (verbose == 1) {
            printf("\n"); 
        }
    } else {        
        if (verbose == 1) {
            printf("hit\n");
        }
        hitCounter++;
    }
}

/* 
 * help - helper message for 'h'.
 */ 
void help() {
    printf("This is the helper documentation.\n");
    printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("-h: Optional help flag that prints usage info\n");
    printf("-v: Optional verbose flag that displays trace info\n");
    printf("-s <s>: Number of set index bits (S = 2^s, the number of sets)\n");
    printf("-E <E>: Associativity (number of lines per set)\n");
    printf("-b <b>: Number of block bits (B = 2^b is the block size)\n");
    printf("-t <tracefile>: Name of the memory trace to replay\n");
}

/* isValid - helper function to check if input parameter starts with a nagative
 * sign, parameter s, E, b should all bigger than zero. if so, print error 
 * message and exit.
 *
 * optarg - an input parameter for operation
 */
void isValid(char* optarg) {
    if (optarg[0] == '-') {
        printf("Invalid input!\n");
        exit(1);
    }
}

/* 
 * getParameters - get useful arguments from input
 * 
 * argc - number of input parameters
 * argv - input parameters
 * s - number of set index bits
 * E - number of lines per set
 * b - number of block bits
 */
char* getParameters(int argc, char** argv, int* s, int* E, int* b,
        int* verbose) {
    int opt;
    char* traceFile;
    while(-1 != (opt = getopt(argc, argv, "s:E:b:t:vh"))) {
        switch(opt) {
            case 's':
                isValid(optarg);
                *s = atoi(optarg);
                break;
            case 'E':
                isValid(optarg);
                *E = atoi(optarg);
                break;
            case 'b':
                isValid(optarg);
                *b = atoi(optarg);
                break;
            case 't':
                traceFile = optarg;
                break;
            case 'v':
                *verbose = 1;
                break;
            case 'h':
                help();
                break;
            default:
                break; 
        }
    }
    return traceFile;
}

/*
 * main - Main funtion for Cache Simulation.
 * 
 * argc - number of input parameters
 * argv - input parameters
 */
int main(int argc, char** argv) {
    int s, E, b, verbose = 0;
    char* traceFile;
    missCounter = 0; 
    hitCounter = 0;
    evictCounter = 0;
    traceFile = getParameters(argc, argv, &s, &E, &b, &verbose);
    CacheLine** cache = init(s, E);
    FILE *f;
    f = fopen(traceFile, "r");

    if (s == 0 || E == 0 || b == 0 ||f == NULL) {
        printf("Invalid input!\n");
        exit(1);
    }

    char identifier;
    unsigned long address;
    int size;

  /* Read from trace file line by line, deal with each operation. */
    while (fscanf(f, "%c %lx,%d", &identifier, &address, &size) > 0) {
        unsigned long setNumber = address << (64 - s - b) >> (64 - s);
        unsigned long tagNumber = address >> (s + b);
        switch(identifier) {
            case 'L':
                if (verbose == 1) {
                    printf("%c %lx,%d ", identifier, address, size);
                }
                simulator(cache, setNumber, tagNumber, E, verbose);
                break;
            case 'S':
                if (verbose == 1) {
                    printf("%c %lx,%d ", identifier, address, size);
                }
                simulator(cache, setNumber, tagNumber, E, verbose);
                break;
            default:
                break;
        }
    }
    fclose(f);
    printSummary(hitCounter, missCounter, evictCounter);
    freeCache(cache, s, E);
    return 0;
}

