        .file        "ia32_crt0.S"

        .section .text
        .align        4
        .globl        _start
        .type        _start,@function
_start:
        // Temporary stack(s) for INIT were created and configure by SETUP
        // Main's stack was allocated by Thread::init()
        // BSS was cleared by SETUP
        // Main's arguments are on the user-level stack, so there is nothing in the CPU registers that needed to be preserved

        call        _init

        .align      4
        .globl      __epos_app_entry
        .type   __epos_app_entry,@function
__epos_app_entry:
        call        __pre_main
        call        main
        push        %eax        // save main's return value to be used by exit()
        call        _fini
        call        _exit
