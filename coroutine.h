/*
 *  Coroutine include header
 */

extern void (*coroutine_panic_fnptr)(const char *msg);

extern void * coroutine_creates (int (*cofunction)(void *),
                                 size_t *stacksizep, int hardfail);

extern void   coroutine_destroy      (void *coctx);
extern int    coroutine_hasended     (void *coctx);
extern void * coroutine_getparam     (void *coctx);
extern void   coroutine_checkcontext (void *coctx);


/* Asm */

#ifndef _WIN32
#define __cdecl __attribute__((__cdecl__))
#endif

extern int __cdecl coroutine_call(void *coctx);

#define coroutine_yield coroutine_call
#define coroutine_create(cofn) coroutine_creates((cofn), 0, 1)

