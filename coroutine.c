/*
 *  Coroutine portable functions are in C
 *
 *  Distribution and use of this software are as per the terms of the
 *  Simplified BSD License (also known as the "2-Clause License")
 *
 *  Copyright 2014 Conor F. O'Rourke. All rights reserved.
 */

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else   /* Unix */
  #include <sys/mman.h>
  #include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "coroutine.inc"
#include "coroutine.h"


static const char *emsg_badcontext = "Coroutine context is not valid";
static const char *emsg_outofmem   = "Coroutine create ran out of memory";
static const char *emsg_badarg     = "Invalid argument to coroutine function";
static const char *emsg_bigstack   = "Stack size is too large (> 1MB)";
static const char *emsg_wrongstack = "Cannot destroy context in coroutine";
static const char *emsg_failfree   = "Cannot free coroutine context";


void (*coroutine_panic_fnptr)(const char *msg) = NULL;


/**
 *  coroutine_panic - Output reason string and exit
 */

static void coroutine_panic(const char *reason)
{
    if (coroutine_panic_fnptr)
        coroutine_panic_fnptr(reason);

#ifndef _WIN32
    fputs(reason, stderr);
#else
    MessageBoxA(NULL, reason, "Coroutine System Panic", MB_ICONERROR);
#endif

    exit(3);
}




/**
 *  getmempagesize - Return the underlying OS page size
 */

#ifdef _WIN32

static size_t getmempagesize(void)
{
    static size_t pgsize = 0;
    if (!pgsize)
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        pgsize = (size_t)sysinfo.dwPageSize;
    }
    return pgsize;
}

#else   /* Unix */

static size_t getmempagesize(void)
{
    static size_t pgsize = 0;
    if (!pgsize)
        pgsize = (size_t)sysconf(_SC_PAGESIZE);
    return pgsize;
}

#endif




/*
 *  stackblkalloc(size) - allocates at least size bytes
 *
 *  A minimum of one page plus one guard page is allocated. The guard page
 *  has protection bits set so that any access will page fault.
 *
 *  Returns: a pointer to the base of the allocated memory and saves the
 *  total size allocated back into *nbytesp. Pass these to stackblkfree()
 *
 *  On error returns NULL and sets errno (but does not update *nbytesp)
 */

static void *stackblkalloc(size_t *nbytesp)
{
    size_t newsize, pagesize;
    void *newptr;
#ifdef _WIN32
    DWORD oldprotect;
#endif

    if (!nbytesp)
    {
        errno = EINVAL;
        return NULL;
    }

    pagesize = getmempagesize();
    newsize = *nbytesp;

    if (newsize < pagesize)
    {
        newsize = pagesize;     /* catches 0 too */
    }
    else if (newsize > 0x100000)
    {
        errno = ERANGE;
        return NULL;
    }
    else
    {
        /* Round up to nearest page */
        newsize += pagesize - 1;
        newsize &= ~(pagesize - 1);
    }

    newsize += pagesize;     /* Add the guard page  */

#ifdef _WIN32
    newptr = VirtualAlloc(0, newsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (newptr == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }
#else
    newptr = mmap(0, newsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (newptr == MAP_FAILED)
    {
        errno = ENOMEM;
        return NULL;
    }
#endif

    /* Set access protection on the guard page */

#ifdef _WIN32
    if (!VirtualProtect(newptr, pagesize, PAGE_NOACCESS, &oldprotect))
    {
        VirtualFree(newptr, 0, MEM_RELEASE);
        errno = EFAULT;
        return NULL;
    }
#else
    if (mprotect(newptr, pagesize, PROT_NONE) != 0)
    {
        munmap(newptr, newsize);
        errno = EFAULT;
        return NULL;
    }
#endif

    *nbytesp = newsize;
    return newptr;
}




/*
 *  stackblkfree(ptr, size) - free the ptr with size bytes
 *
 *  Free the memory block allocated with stackblkalloc()
 *  In the Linux case, the free is done with munmap so the size returned
 *  by stackblkalloc() is required to know which band of memory to unmap
 *
 *  Returns: 1 on success, 0 on failure
 */

static int stackblkfree(void *memptr, size_t memsize)
{
#ifdef _WIN32
    if (!VirtualFree(memptr, 0, MEM_RELEASE))
#else
    if (munmap(memptr, memsize) != 0)
#endif
    {
        errno = EFAULT;
        return 0;
    }

    return 1;
}




/**
 *  coroutine_creates - Creates a coroutine context block
 *
 *  Parameters: cofunction - Pointer to coroutine function
 *              stacksizep - Optional pointer to stack size
 *              hardfail   - Boolean hardfail (panics on error)
 *
 *  Allocates and initialises a context block and stack memory area.
 *  The stacksize is rounded up to the nearest page size and
 *  stacksizep can be NULL in which case the default is 8192 bytes.
 *
 *  Returns: an opaque pointer to the coroutine context.
 *           Writes the usable stacksize back to stacksizep.
 *
 *  On error: sets errno, returns NULL or calls coroutine_panic
 */

void *coroutine_creates(int (*cofunction)(void *),
                        size_t *stacksizep, int hardfail)
{
    size_t costacksize = COROUTINE_VSTKSIZE;
    size_t pagesize = getmempagesize();

    unsigned char *blockptr, *contextp, *stacklap;


    if (!cofunction)
    {
        if (hardfail)
            coroutine_panic(emsg_badarg);
        errno = EINVAL;
        return NULL;
    }

    if (stacksizep && (*stacksizep > costacksize))
        costacksize = *stacksizep;

    blockptr = stackblkalloc(&costacksize);

    if (blockptr == NULL)
    {
        if (hardfail)
        {
            if (errno == ERANGE)
                coroutine_panic(emsg_bigstack);
            else
                coroutine_panic(emsg_outofmem);
        }
        return NULL;
    }

    if (stacksizep)
        *stacksizep = (costacksize - pagesize);


    /* Context structure sits on a 16 byte aligned address
       at the top of the allocated space:
                             ______Top of allocated space
          [context structure]______Stack High
                [stack frame]______Stack Low (one page above blockptr)
                 [Guard page]______Block Pointer
    */

    contextp = blockptr + ((costacksize - CTX_STRUCTSIZE) & ~15UL);
    stacklap = blockptr + pagesize;

    /* Zero out the context block. Strictly not necessary but might do
       a debug version where I fill the stack frame with guard bytes */

    memset(contextp, 0, CTX_STRUCTSIZE);

    *(uint32_t  *)(contextp + CTX_MAGIC)      = COROUTINE_MAGICVAL;
    *(uintptr_t *)(contextp + CTX_COFUNCTION) = (uintptr_t)cofunction;

    *(uintptr_t *)(contextp + CTX_BLOCKPTR)   = (uintptr_t)blockptr;
    *(uint32_t  *)(contextp + CTX_BLOCKSIZE)  = (uint32_t)costacksize;

    *(uintptr_t *)(contextp + CTX_VSTACKHIGH) = (uintptr_t)contextp;
    *(uintptr_t *)(contextp + CTX_VSTACKLOW)  = (uintptr_t)stacklap;

    /* The remaining parameters are zero initialised by memset() */

    return contextp;
}




/**
 *  coroutine_checkcontext - Validate context
 *
 *  Takes the opaque pointer to the context block and checks the magic
 *  number located in the first 4 bytes of the context. Called from
 *  the assembly routines too.
 *
 *  Returns nothing and panics on error
 */

void coroutine_checkcontext(void *coctx)
{
    if (*(uint32_t *)coctx != COROUTINE_MAGICVAL)
        coroutine_panic(emsg_badcontext);
}




/**
 *  coroutine_destroy - Frees a coroutine context block
 *
 *  Takes the opaque pointer to the allocated context block and
 *  frees the associated memory. Also overwrites the magic number.
 *
 *  Returns nothing and panics on error
 */

void coroutine_destroy(void *coctx)
{
    unsigned char *contextp = coctx;
    void *blockptr;
    size_t blocksize;

    coroutine_checkcontext(coctx);

    /* Check the vstack flag. Destroying our own stack would
       be rather catastrophic. Funny but uncontrolled! */

    if (*(uint32_t *)(contextp + CTX_ONVSTACK) != 0)
        coroutine_panic(emsg_wrongstack);


    /* Destroy this context */

    blockptr  = (void *) (*(uintptr_t *)(contextp + CTX_BLOCKPTR));
    blocksize = (size_t) (*(uint32_t  *)(contextp + CTX_BLOCKSIZE));

    *(uint32_t *)(contextp + CTX_MAGIC) = 0;    /* Delete magic */

    if (!stackblkfree(blockptr, blocksize))
        coroutine_panic(emsg_failfree);

}




/**
 *  coroutine_hasended - Check the CTX_YIELDED entry
 *
 *  Takes the opaque pointer to the allocated context block
 *  Returns: 0 if the coroutine is still yielding
 *           1 if the coroutine has ended (or hasn't started)
 *  Panics on error
 */

int coroutine_hasended(void *coctx)
{
    unsigned char *contextp = coctx;

    coroutine_checkcontext(coctx);

    if (*(uint32_t *)(contextp + CTX_YIELDED) == 0)
        return 1;

    return 0;
}




/**
 *  coroutine_getparam - Get a pointer to the CTX_EXTRAPARAM entry
 *
 *  Takes the opaque pointer to the allocated context block
 *  Returns a pointer to the context structure's extra parameter entry
 *  Panics on error
 */

void *coroutine_getparam(void *coctx)
{
    unsigned char *contextp = coctx;

    coroutine_checkcontext(coctx);

    return (void *)(contextp + CTX_EXTRAPARAM);
}

