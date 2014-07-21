/*
 *  Coroutine definitions include file
 *    This is pulled into the assembly as well as the C file
 */


/*  Context is an opaque pointer to this structure:

    struct coroutine_context
    {
        uint32_t magic;         [+ 4 byte align for 64 bit]

        int (*cofunction)(void *coctx);

        void    *blockptr;
        uint32_t blocksize;     [+ 4 byte align for 64 bit]

        void *vstackhigh;
        void *vstacklow;

        void *nextjump;
        void *otherstack;

        uint32_t yielded;
        uint32_t onvstack;

        int   retvalue          [+ 4 byte align for 64 bit]
        void *extraparam;

        [ 32 bit = 48 bytes, blank align to 64  ]
        [ 64 bit = 88 bytes, blank align to 128 ]
    }

*/

#define COROUTINE_MAGICVAL  0x21504854
#define COROUTINE_VSTKSIZE  0x2000

#define CTX_MAGIC       0

#define CTX_COFUNCTION  4

#define CTX_BLOCKPTR    8
#define CTX_BLOCKSIZE   12
#define CTX_VSTACKHIGH  16
#define CTX_VSTACKLOW   20

#define CTX_NEXTJUMP    24
#define CTX_OTHERSTACK  28

#define CTX_YIELDED     32
#define CTX_ONVSTACK    36

#define CTX_RETVALUE    40
#define CTX_EXTRAPARAM  44

#define CTX_STRUCTSIZE  64
