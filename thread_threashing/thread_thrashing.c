/**************************************************************
 * coded by jwpark82@gscdn.com/sliod@naver.com
 **************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <sched.h>
#include <sys/time.h>
#include "myAssert.h"
#include "thread_thrashing.h"

//getopt
extern char *optarg;

void printThreadRelatedInfo(size_t aAvailablePhyMem)
{
    printf("print statistics value\n"
            "total allocated memory : %ld\n"
            "total thread stacksize : %ld\n"
            "_SC_AVPHYS_PAGES       : %ld\n"
            "DELTA _SC_AVPHYS_PAGES : %ld\n\n",
            (sizeof(pthread_t) * THREAD_CNT) + (THREAD_CNT * MEMORY_SIZE ),
            THREAD_CNT * THREAD_STACK_SIZE,
            sysconf(_SC_AVPHYS_PAGES),
            aAvailablePhyMem - sysconf(_SC_AVPHYS_PAGES));
}
void printParameterAndSysconf(size_t aAvailablePhyMem)
{
    printf( "print sysconf value\n"
            "_SC_PAGESIZE         : %ld\n"
            "_SC_PHYS_PAGES       : %ld\n"
            "_SC_AVPHYS_PAGES     : %ld\n"
            "_SC_NPROCESSORS_CONF : %ld\n"
            "_SC_NPROCESSORS_ONLN : %ld\n\n",
            sysconf(_SC_PAGESIZE),
            sysconf(_SC_PHYS_PAGES),
            aAvailablePhyMem,
            sysconf(_SC_NPROCESSORS_CONF),
            sysconf(_SC_NPROCESSORS_ONLN));

    printf( "print parameter values\n"
            "THREAD_CNT        : %d\n"
            "MEMORY_SIZE       : %zu\n"
            "THREAD_STACK_SIZE : %zu\n"
            "ACCESS_DISTANCE   : %d\n"
            "ACCESS_PATTERN    : %s\n\n",
            THREAD_CNT,
            MEMORY_SIZE,
            THREAD_STACK_SIZE,
            ACCESS_DISTANCE,
            ACCESS_PATTERN_STRING[ACCESS_PATTERN]);
}

void *threadFunc(void * aThreadArg)
{
    page  *sAllocMem           = NULL;
    page  *sMemChunk           = NULL;
    int    sPageCnt            = (MEMORY_SIZE >> OS_PAGE_SIZE_BIT_POS);
    const threadArg sThreadArg = *((threadArg *)aThreadArg); 
    const int sArrIdx          = sThreadArg.mThreadID / 8;
    const int sArrIdxBitPos    = (1 << (sThreadArg.mThreadID % 8));
    int    sResult;
    int    i;
    struct timeval sBegin, sEnd;
    double diffTime1, diffTime2;
    struct timespec  begin, end;

    DASSERT(MEMORY_SIZE % OS_PAGE_SIZE == 0);

    //4K aligned memory alloc
    sResult = posix_memalign((void**)&sAllocMem ,OS_PAGE_SIZE ,MEMORY_SIZE); 
    DASSERT(sResult != RETURN_FAILURE && sAllocMem != NULL);      // is allocation sucess?
    DASSERT(((unsigned long)sAllocMem & OS_PAGE_SIZE_MASK) == 0); // is 4k adress aligned?

    while(gIsOn == FALSE)
    {
        sleep(1);
    }

    while(gIsOn == TRUE)
    {
        while(gPause == TRUE)
        {
            if((gThreadStatusArr[sArrIdx] & sArrIdxBitPos) == 0)
            {
                //set thread status bit(pause)
                __sync_fetch_and_and( &gThreadStatusArr[sArrIdx], sArrIdxBitPos );
            }

            DASSERT((gThreadStatusArr[sArrIdx] & sArrIdxBitPos) == sArrIdxBitPos);
            sleep(1);
        }

        //clear thread status bit(resume)
        __sync_fetch_and_and( &gThreadStatusArr[sArrIdx], ~sArrIdxBitPos );

        //sAllocMemIdx = gAccessPattern[i++];
        gettimeofday(&sBegin, NULL);
        clock_gettime(CLOCK_MONOTONIC, &begin);
        for(i = 0; i < sPageCnt;i++)
        {
            sMemChunk = &sAllocMem[i]; //4k memory 
            DASSERT(((long)sMemChunk & OS_PAGE_SIZE_MASK) == 0); //check 4K memory alignment

            if(ACCESS_PATTERN != ACCESS_NOACCESS)
            {
                //memory write
                sMemChunk->mPage[0]                = 'a'; //write the first byte of page
                sMemChunk->mPage[OS_PAGE_SIZE - 1] = 'a'; //write the last byte of page
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        gettimeofday(&sEnd, NULL);

        diffTime1 = (sEnd.tv_sec-sBegin.tv_sec)+((sEnd.tv_usec-sBegin.tv_usec) / 1000000.0);
        diffTime2 = (end.tv_sec-begin.tv_sec)+((end.tv_nsec-begin.tv_nsec ) / 1000000000.0);

        //not thread safe but ok.
        gMaxTime1 = diffTime1 > gMaxTime1 ? diffTime1 : gMaxTime1;
        gMaxTime2 = diffTime2 > gMaxTime2 ? diffTime2 : gMaxTime2;

        __sync_fetch_and_add( &gLoopCnt, 1 );

        //context switch
        //sleep(0);
        //sched_yield();
    }

    free(sAllocMem);
    sAllocMem = NULL;

    return 0;
}

int calcMemorySize(char *aMemSize)
{
    size_t  sScale     = 0; //KILO,MEGA,GIGA
    int     sStrLength = 0;
    int     sMemSize   = 0;
    char    sUnit; //digit,K,M,G

    sStrLength = strlen(aMemSize);

    //get unit character
    sUnit = aMemSize[sStrLength - 1];
    aMemSize[sStrLength - 1] = '\0';

    sMemSize = atoi(aMemSize);

    switch(sUnit)
    {
        case 'k':
        case 'K':
            sScale = KILO;
            break;
        case 'm':
        case 'M':
            sScale = MEGA;
            break;
        case 'g':
        case 'G':
            sScale = GIGA;
            break;
        default:
            sScale = 1; //Byte
            if(isdigit(sUnit) == FALSE)
            {
                printf("wrong input \n");
                DASSERT(1);
            }
    }

    DASSERT(sMemSize != 0);
    DASSERT(sScale   != 0);

    return (sMemSize * sScale); //MEMORY size to byte;
}

/* 
 * if ACCESS_DISTANCE == 1
 * [0,1,2,3,4,5,6,.......]
 * if ACCESS_DISTANCE == 2
 * [0,2,4,6,8,10,12,.....]
 * if ACCESS_DISTANCE == 3
 * [0,3,6,9,12,15,18.....]
 *
 * if ACCESS_DISTANCE == X
 * [0,X+0,1,X+1,2,X+2,3.....]
 */
void generateAccessPattern(int *aAccessPatternArr, int aPageCnt, int aDistance)
{
    int i = 0;
    int j = 0;

    DASSERT(aPageCnt == (MEMORY_SIZE >> OS_PAGE_SIZE_BIT_POS));
    DASSERT((0 < aDistance) && (aDistance < aPageCnt));
    DASSERT(aAccessPatternArr != NULL);

    memset((void*)aAccessPatternArr, -1, MEGA);

    for(i = 0; i < aPageCnt; i++)
    {
        //random I/O
        if(aDistance == (aPageCnt / 2))
        {
            /*
             * if ACCESS_DISTANCE == X
             * [0,X+0,1,X+1,2,X+2,3.....]
             */
            // is even or odd?
            if((i & 0x00000001) == 0)
            {
                aAccessPatternArr[i] = j++;
            }
            else
            {
                aAccessPatternArr[i] = aDistance++;
            }
        }
        /*else if() // 100% access x pattern?
          {
          }*/
        else// sequential I/O
        {
            /* 
             * if ACCESS_DISTANCE == 1 100% access
             * [0,1,2,3,4,5,6,.......]
             * if ACCESS_DISTANCE == 2 50% access
             * [0,2,4,6,8,10,12,.....]
             * if ACCESS_DISTANCE == 3 33% access
             * [0,3,6,9,12,15,18.....]
             */
            if( i % aDistance == 0)
            {
                aAccessPatternArr[i] = j;
            }

            j++;
        }
    }
}

void checkThreadStatus(int aArrSize, 
        volatile unsigned long *aThreadStatusArr, 
        threadStatus aStatus)
{
    int i;

    for(i = 0; i < aArrSize; i++)
    {
checkAgain:
        if(aThreadStatusArr[i] == aStatus)
        {
            // ok countinue
        }
        else
        {
            sched_yield();
            goto checkAgain;
        }
    }
}

int main(int argc, char **argv)
{
    pthread_t      *sThreadArr  = NULL;
    int             sOption     = 0;
    int             sResult     = 0;
    int             sPageCnt    = (MEMORY_SIZE >> OS_PAGE_SIZE_BIT_POS); //get default sPageCnt
    int             i           = 0;
    size_t          sAvailablePhyMem;
    pthread_attr_t  attr;
    threadArg       sThreadArg;
    struct timeval sBegin, sEnd;
    double diffTime;

    gettimeofday(&sBegin, NULL);

    DASSERT(OS_PAGE_SIZE   == sysconf(_SC_PAGESIZE));
    DASSERT(sizeof(threadStatus) == sizeof(unsigned long));

    while((sOption = getopt(argc, argv, "c:m:s:d:i:g:")) != EOF)
    {
        switch(sOption)
        {
            case 'c': 
                THREAD_CNT = atoi(optarg);
                THREAD_CNT = MAX_THREAD_CNT < THREAD_CNT ? MAX_THREAD_CNT : THREAD_CNT;

                //round down to set multiple of 64
                THREAD_CNT = ROUND_DOWN(THREAD_CNT, GET_BIT_COUNT(gThreadStatusArr[0]));
                DASSERT(THREAD_CNT % GET_BIT_COUNT(gThreadStatusArr[0]) == 0);
                DASSERT(THREAD_CNT != 0);
                break;
            case 'm': 
                MEMORY_SIZE = calcMemorySize(optarg);
                // round up. to set multiple of 4K
                MEMORY_SIZE = ROUND_UP(MEMORY_SIZE, OS_PAGE_SIZE);
                break;
            case 's': 
                THREAD_STACK_SIZE = calcMemorySize(optarg);
                DASSERT(THREAD_STACK_SIZE >= PTHREAD_STACK_MIN);
                break;
            case 'd': 
                // distance to next access page
                ACCESS_DISTANCE = atoi(optarg);
                sPageCnt        = (MEMORY_SIZE >> OS_PAGE_SIZE_BIT_POS);

                if((sPageCnt >> 1) <= ACCESS_DISTANCE)
                {
                    //set maximum distance between pages. really?
                    ACCESS_DISTANCE = sPageCnt >> 1; 
                    ACCESS_PATTERN  = ACCESS_RANDOM;
                }
                else if(ACCESS_DISTANCE <= 0)
                {
                    ACCESS_PATTERN = ACCESS_NOACCESS;
                    ACCESS_DISTANCE = -1;
                }
                else
                {
                    ACCESS_PATTERN = ACCESS_SEQUENTIAL;
                }
                break;
            case 'i': 
                IO_TYPE = atoi(optarg);
                DASSERT((IO_TYPE == IO_READ) || (IO_TYPE == IO_WRITE));
                break;
            case 'g': 
                GLOBAL_MEMORY = atoi(optarg);
                DASSERT((GLOBAL_MEMORY == TRUE) || (GLOBAL_MEMORY == FALSE));

                if(GLOBAL_MEMORY == TRUE)
                {
                    sResult = posix_memalign((void**)&gGlobalMem,
                            OS_PAGE_SIZE,
                            (MEMORY_SIZE * THREAD_CNT));
                    DASSERT(sResult != RETURN_FAILURE && gGlobalMem != NULL);
                    DASSERT(((unsigned long)gGlobalMem & OS_PAGE_SIZE_MASK) == 0);
                }
                else
                {
                    gGlobalMem = NULL;
                }
                break;
            default:
                break;
        }
    }

    sAvailablePhyMem = sysconf(_SC_AVPHYS_PAGES);
    printParameterAndSysconf(sAvailablePhyMem);

    memset((void*)&gThreadStatusArr, 0x00, MAX_THREAD_ARR_SIZE);
    generateAccessPattern(gAccessPattern, sPageCnt, ACCESS_DISTANCE);

    sThreadArr = (pthread_t *)malloc(sizeof(pthread_t) * THREAD_CNT);
    DASSERT(sThreadArr != NULL);

    sResult = pthread_attr_init(&attr);                                               
    DASSERT(sResult != RETURN_FAILURE);

    sResult = pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);                                   
    DASSERT(sResult != RETURN_FAILURE);

    for( i = 0; i < THREAD_CNT; i++)
    {
        sThreadArg.mThreadID = i;
        sThreadArg.mPage = (GLOBAL_MEMORY == TRUE) ? &gGlobalMem[i] : NULL;
        DASSERT((sThreadArg.mPage == NULL) || 
                (((unsigned long)sThreadArg.mPage & OS_PAGE_SIZE_MASK) == 0));

        sResult = pthread_create(&sThreadArr[i], NULL, threadFunc, (void *)&sThreadArg);
        DASSERT(sResult != RETURN_FAILURE);
    }
    printf("%d thread created\n\n", i);

    printThreadRelatedInfo(sAvailablePhyMem);

    /*printf("press enter to activate thread\n");
      (void)getchar();
      gIsOn = TRUE;*/

    sleep(10);
    printf("thread start\n");
    gIsOn = TRUE;

    /*while(TRUE)
      {
      printf("momry access distance: \n");
      scanf("%*d%d %*d%d", &sDistance, (int*)&sIOType);
      gPause = TRUE;

    //wait all threads paused
    checkThreadStatus(THREAD_CNT / sizeof(unsigned long), 
    gThreadStatusArr,
    THREAD_STATUS_PAUSE);
    //This point, all threads are paused. change the parameters.

    //now we can modify global values
    //skip worng input
    if((IO_TYPE == IO_READ) || (IO_TYPE == IO_WRITE))
    {
    IO_TYPE = sIOType;
    }

    generateAccessPattern(gAccessPattern, sPageCnt, ACCESS_DISTANCE);

    gPause = FALSE;
    //wait all thread are  resumed
    checkThreadStatus(THREAD_CNT / sizeof(unsigned long), 
    gThreadStatusArr,
    THREAD_STATUS_RESUME);
    //This point, all threads are resumed. change the parameters.
    }*/

    /*printf("press enter to terminate thread\n");
      (void)getchar();
      gIsOn = FALSE;*/

    sleep(30);
    printf("thread stop\n");
    gIsOn = FALSE;

    sleep(0);

    gettimeofday(&sEnd, NULL);
    diffTime = (sEnd.tv_sec-sBegin.tv_sec)+((sEnd.tv_usec-sBegin.tv_usec) / 1000000.0);

    printf("Total elapsed time         : %f sec\n"
            "Max elapsed time1 per loop : %f sec\n"
            "Max elapsed time2 per loop : %f sec\n"
            "Total loop count           : %d\n"
            "Average loop count         : %f\n", 
            diffTime,
            gMaxTime1,
            gMaxTime2,
            gLoopCnt,
            gLoopCnt/(float)THREAD_CNT);  

    for( i = 0; i < THREAD_CNT; i++)
    {
        pthread_join(sThreadArr[i], (void **)&sResult);
    }
    printf("\n%d thread finished\n", i);

    free(sThreadArr);
    sThreadArr = NULL;

    return RETURN_SUCCESS;
}
