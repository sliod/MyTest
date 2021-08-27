#ifndef _MYASSERT_H
#define _MYASSERT_H    1

#include <assert.h>

#define UNUSED(x) (void)(x)

// ASSERT(condition) checks if the condition is met, and if not, calls
// ABORT with an error message indicating the module and line where
// the error occurred.        
#ifndef ASSERT                
#define ASSERT(x)                                               \
    if (!(x)) {                                                 \
        printf("Assertion failed in \"%s\", line %d\n" \
                "\tProbable bug in software.\n",                \
                __FILE__, __LINE__);                            \
        assert(0);                                              \
    }                                                           \
else   // This 'else' exists to catch the user's following semicolon
#endif

// DASSERT(condition) is just like ASSERT, except that it only is 
// functional in DEBUG mode, but does nothing when in a non-DEBUG
// (optimized, shipping) build.
#ifdef DEBUG                  
# define DASSERT(x) ASSERT(x) 
#else                         
# define DASSERT(x) UNUSED(x)  /* DASSERT does nothing when not debugging */
#endif 

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif  /* _MYASSERT_H  */
