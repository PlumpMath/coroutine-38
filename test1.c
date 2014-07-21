#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "coroutine.h"


int mycofunction(void *coctx)
{
    int i = 0;

    for (i = 0; i < 5; i++)
    {
        printf("In mycofunction where i is: %d\n", i);
        coroutine_yield(coctx);
    }

    return 100;
}


int main()
{
    void *cocontextp;
    int *coparam;
    int ret;

    cocontextp = coroutine_create(mycofunction);

    printf("Coroutine create complete\n");
    printf("Starting coroutine call\n");

    coparam = coroutine_getparam(cocontextp);

    printf("ctx = %p\tcoparam = %p\n", cocontextp, coparam);

    do {

        ret = coroutine_call(cocontextp);
        printf("Back in main!\n");

    } while (!coroutine_hasended(cocontextp));

    printf("Coroutine returns with: %d\n", ret);

    printf("End\n");
    coroutine_destroy(cocontextp);
    return 0;
}

