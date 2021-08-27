#ifndef _THREAD_DEF_H
#define _THREAD_DEF_H    1

#define TRUE                  (1)
#define FALSE                 (0)
                              
#define RETURN_SUCCESS        (0)
#define RETURN_FAILURE        (-1)
                              
#define OS_PAGE_SIZE          (4 * KILO) 
#define OS_PAGE_SIZE_HEX      (0x1000)
#define OS_PAGE_SIZE_MASK     (0x0000000000000FFF)
#define OS_PAGE_SIZE_BIT_POS  (12) //hex 0x1000, digit 4096

#define KILO                  (1 << 10) //2^10
#define MEGA                  (1 << 20) //2^20
#define GIGA                  (1 << 30) //2^30

//Thread capacity is 65536
#define MAX_THREAD_CNT_IN_BIT (MAX_THREAD_ARR_SIZE * GET_BIT_COUNT(gThreadStatusArr[0]))
#define MAX_THREAD_ARR_SIZE   (KILO)
#define MAX_THREAD_CNT        (MAX_THREAD_CNT_IN_BIT)

#define GET_BIT_COUNT(a) (sizeof(a) * 8)
//#define ROUND_DOWN(a, b) (((a)  & (~b)))
//#define ROUND_UP(a, b)   ((((a) & ((b) - 1))) ? ROUND_DOWN((a)+(b), (b)) : (a));
#define ROUND_DOWN(a, b) (((a) & ~((b) - 1)))
#define ROUND_UP(a, b)   (((a) + (b) - 1) & ~((b) - 1))
#define GET_BIT_NTH(a, n) (1 & ((a) >> (n)))
#define SET_BIT_NTH(a, n, t)\
    do{\
        if(t){\
            return(((unsigned long int) 1 << n) | a);\
        }\
        else\
        {\
            return(~((unsigned long int) 1 << n) & a);\
        }\
    }while(0);\

#endif  /* _THREAD_DEF_H  */
