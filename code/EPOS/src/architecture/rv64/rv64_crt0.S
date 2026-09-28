// EPOS RISC-V 64 Program Starter

        .file "rv64_crt0.S"

        .section .text
        .align  8
        .global _start
        .type   _start, function
_start:
        // Temporary stack(s) for INIT were created and configure by SETUP
        // Main's stack was allocated by Thread::init()
        // BSS was cleared by SETUP

        // Save main's argc and argv
        addi    sp, sp, -16
        sd      a0, 0(sp)
        sd      a1, 8(sp)

        call      _init

        // Restore main's argc and argv
        ld      a0, 0(sp)
        ld      a1, 8(sp)
        addi    sp, sp, 16

        .align  8
        .globl  __epos_app_entry
        .type   __epos_app_entry, function
__epos_app_entry:
        call    __pre_main
        call    main
        addi    sp, sp, -8
        sd      a0, 0(sp)   // save main's return value to be used by exit()
        call    _fini
        ld      a0, 0(sp)
        call    _exit
