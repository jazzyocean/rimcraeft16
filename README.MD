## Rīmcræft
-is a 16-bit virtual computer/fantasy console with a fully-fledged custom RISC instruction set, with a complete unique assembly language, and a work-in-progess higher level language based on C.

![Image of Rimcraeft16 printing text](https://github.com/snowbifx/rimcraeft16/blob/master/hello_github.gif?raw=true)

The files in this repository are as follows:
```
    src                   Directory         Contains all source files of the project
      emu                 Directory         Contains all source files for the emulator
        include           Directory         External header files required to compile
           raylib.h       C header file     Header file for the Raylib project, used for graphics
        lib               Directory         External .a files required to compile
           libraylib.a    Static library    Library file for the Raylib project
        consts.h          C header file     Contains all constants used by the project
        emu.c             C source file     Emulator mainloop, threads, and interrupt handler(TODO)
        emu.h             C header file     Emulator struct definition, function prototypes
        fileio.c          C source file     Function for loading a dynamic number of sectors of ROM into buffer
        fileio.h          C header file     Function prototypes
        helpers.c         C source file     Various miscellaneous helper functions
        helpers.h         C header file     Function prototypes
        main.c            C header file     Main function, initializes everything and displays info before and after execution.
```

Rīmcræft *(alt. rimcraeft)* is the Old English word for "mathematics," coming literally from *rīm—number* plus *cræft—knowledge, science, skill*.

Docs for Rimcraeft16 can be found on [Read the Docs](https://google.com)