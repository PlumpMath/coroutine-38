coroutine
=========

Easy Coroutines for 32 bit Windows and Linux.

I found some coroutine libraries out there but they seemed a bit complex
to work with. I just wanted to start a routine and yield back to main.
So I started a small project to do this in x86 assembly under Win32
and then ported it to Linux.

Each coroutines is allocated its own stack which defaults to 8192 bytes.
Each stack has a guard page below its lowest address with the page protection
bits set to no access. Blowing the stack will page fault instantly.

The assembly code uses Intel style assembly but does use some gcc
style directives and is preprocessed using gcc.

Compiles under Windows with:

     gcc -Wall -mconsole -o test1.exe test1.c coroutine.c coroutine.sx
   
and similarly under Linux with:

     gcc -Wall -o test1 test1.c coroutine.c coroutine.sx

Might port to 64 bit if anyone is terribly worried.

Conor.
