/*
 * macOS / Mach-O port of SaveRegisters_arm64.S.
 *
 * Same arm64 instructions as the ELF version; only the assembler directives and
 * the symbol name differ: Mach-O prefixes C symbols with an underscore and does
 * not support the ELF .type/%function/.hidden directives.
 *
 * typedef void (*PushAllRegistersCallback)(SafePointBarrier*, ThreadState*, intptr_t*);
 * extern "C" void pushAllRegisters(SafePointBarrier*, ThreadState*, PushAllRegistersCallback)
 */

.globl _pushAllRegisters
.p2align 2
_pushAllRegisters:
        /* Save the callee-saved registers (and the return address). */
        sub sp, sp, #96
        stp x19, x20, [sp, #80]
        stp x21, x22, [sp, #64]
        stp x23, x24, [sp, #48]
        stp x25, x26, [sp, #32]
        stp x27, x28, [sp, #16]
        stp x30, x29, [sp, #0]  // x30 is lr.

        /* Pass the first two arguments through unchanged (x0, x1) and the stack
         * pointer as the third argument to the callback.
         */
        mov x3, x2
        mov x2, sp
        blr x3

        /* The callee-saved registers were not clobbered, so only the return
         * address needs restoring.
         */
        ldr x30, [sp], #96
        ret
