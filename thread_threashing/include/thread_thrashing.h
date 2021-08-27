#ifndef _THREAD_THRASHING_H
#define _THREAD_THRASHING_H    1

#include "thread_def.h"

typedef struct page {
    char mPage[OS_PAGE_SIZE];
} page;

typedef struct threadArg {
    unsigned int  mThreadID;
    page        * mPage;
} threadArg;

typedef enum IOType{
    IO_READ  = 0,
    IO_WRITE = 1,
    IO_NONE  = 2,
} IOType;

typedef enum accessPattern{
    ACCESS_SEQUENTIAL = 0,
    ACCESS_RANDOM     = 1,
    ACCESS_INTERVAL   = 2,
    ACCESS_NOACCESS   = 3,
} accessPattern;

//64 threads cluster status
//It dosen`t check individual thread status.
//It checks 64 threads are all paused or resumed at once
typedef enum threadStatus{
    THREAD_STATUS_RESUME = 0,
    THREAD_STATUS_PAUSE  = 0xFFFFFFFFFFFFFFFF, //ULONG_MAX
} threadStatus;

//global variable
volatile int           gIsOn           = FALSE;
volatile int           gPause          = FALSE;
volatile double        gMaxTime1       = 0;
volatile double        gMaxTime2       = 0;
volatile unsigned long gThreadStatusArr[MAX_THREAD_ARR_SIZE];
int                    gAccessPattern[MEGA];
page                 * gGlobalMem = NULL;
int                    gLoopCnt   = 0;

//arguments 
int           THREAD_CNT        = GET_BIT_COUNT(gThreadStatusArr[0]); 
size_t        MEMORY_SIZE       = MEGA;
size_t        THREAD_STACK_SIZE = 8 * MEGA;
int           ACCESS_RATE       = 0;
accessPattern ACCESS_PATTERN    = 0;
int           ACCESS_DISTANCE   = 1;
int           GLOBAL_MEMORY     = FALSE;
IOType        IO_TYPE           = IO_READ;

const char ACCESS_PATTERN_STRING[ACCESS_NOACCESS+1][12] = {"SEQUENTIAL\0",
                                                           "RANDOM\0",
                                                           "INTERVAL\0",
                                                           "NOACCESS\0"};

void *threadFunc(void * aThreadArg);
int calcMemorySize(char * aMemSize);
void generateAccessPattern(int * aAccessPatternArr, 
                           int   aPageCnt, 
                           int   aDistance);
void checkThreadStatus(int                      aArrSize,
                       volatile unsigned long * aThreadStatusArr,
                                                threadStatus aStatus);
void printParameterAndSysconf();
void printThreadRelatedInfo();

#endif  /* _THREAD_THRASHING_H  */
