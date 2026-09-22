#ifndef PC_SIDE_H
#define PC_SIDE_H

/* pc_side.h — force-included (-include) at the top of every host translation
 * unit, so it is seen BEFORE hc12def.h / mcohw.h.
 *
 * Those headers define the interrupt-enable macros guarded by #ifndef INTR_ON:
 *
 *     #define INTR_ON()   asm("cli")     // clear I-mask -> ENABLE interrupts
 *     #define INTR_OFF()  asm("sei")     // set  I-mask -> DISABLE interrupts
 *
 * GCC cannot assemble the HCS12 cli/sei opcodes, so we pre-define INTR_ON/OFF
 * here (winning the #ifndef) and map them to a single interrupt-mask flag the
 * RTI simulation thread honors. That gives the firmware's existing
 * INTR_OFF()/INTR_ON() critical sections real meaning on the host: the
 * simulated ISR will not run while interrupts are "masked".
 *
 * Faithful to the hardware: the HCS12 I-bit is a single flag (not a nesting
 * counter), so a boolean matches cli/sei exactly. At reset interrupts are
 * masked; the firmware's INTR_ON() enables them.
 */

extern volatile int g_intr_masked;      /* 1 = interrupts masked (reset state) */

#ifndef INTR_ON
#define INTR_ON()   (g_intr_masked = 0)
#define INTR_OFF()  (g_intr_masked = 1)
#endif

#endif  /* PC_SIDE_H */
