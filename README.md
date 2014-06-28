coroutine
=========

Coroutines in assembly for 32 bit Windows and Linux.

I found some coroutine libraries out there but they seemed a bit complex to work with. I just wanted to start a routine and yield back to main. So I started a small project to do this in x86 assembly under Win32 and then ported it to Linux. I started in assembly and then got slightly carried away and did the whole thing in assembly!

The assembly code uses Intel style assembly but does use some gcc style directives and is preprocessed using gcc.

Compiles under Windows with:

     gcc -Wall -mconsole -o test1.exe test1.c coroutine.sx
   
and similarly under Linux with:

     gcc -Wall -o test1 test1.c coroutine.sx

Might port to 64 bit if anyone is terribly worried.

Conor.
