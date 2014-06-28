/*
 *  Coroutine header
 */

#ifndef _WIN32
#define __cdecl __attribute__((__cdecl__))
#endif

extern void   __cdecl coroutine_setpanic (void (*newpanicfn)(const char *msg));
extern void * __cdecl coroutine_creates  (int  (*cofunction)(void *), size_t *stacksizep);
extern int    __cdecl coroutine_destroy  (void *coctx);
extern int    __cdecl coroutine_hasended (void *coctx);
extern void * __cdecl coroutine_getparam (void *coctx);
extern int    __cdecl coroutine_call     (void *coctx);

#define coroutine_yield coroutine_call
#define coroutine_create(a) coroutine_creates((a), 0)

