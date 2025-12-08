#ifndef VBOX_CPU_H
#define VBOX_CPU_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * CPU Register Structure
 *============================================================================*/

/* General purpose registers - can be accessed as 8-bit, 16-bit parts */
typedef union {
  u16 x; /* Full 16-bit register (AX, BX, CX, DX) */
  struct {
    u8 l; /* Low byte (AL, BL, CL, DL) */
    u8 h; /* High byte (AH, BH, CH, DH) */
  };
} GPReg16;

/* CPU State Structure */
typedef struct {
  /* General Purpose Registers */
  GPReg16 a; /* AX = AH:AL - Accumulator */
  GPReg16 b; /* BX = BH:BL - Base */
  GPReg16 c; /* CX = CH:CL - Counter */
  GPReg16 d; /* DX = DH:DL - Data */

  /* Index Registers */
  u16 si; /* Source Index */
  u16 di; /* Destination Index */

  /* Pointer Registers */
  u16 bp; /* Base Pointer */
  u16 sp; /* Stack Pointer */

  /* Program Counter */
  u16 ip; /* Instruction Pointer */

  /* Segment Registers */
  u16 cs; /* Code Segment */
  u16 ds; /* Data Segment */
  u16 es; /* Extra Segment */
  u16 ss; /* Stack Segment */

  /* Flags Register */
  u16 flags;

  /* Emulator State */
  bool halted;
  bool interrupt_pending;
  u8 pending_interrupt;

  /* Instruction Decoding State */
  u8 prefix_segment; /* Segment override prefix (0 = none) */
  bool prefix_rep;   /* REP/REPE prefix */
  bool prefix_repne; /* REPNE prefix */

  /* Cycle counter for timing */
  u64 cycles;
} VBoxCPU;

/*============================================================================
 * CPU Functions
 *============================================================================*/

/**
 * Initialize CPU to reset state
 */
void cpu_init(VBoxCPU *cpu);

/**
 * Reset CPU to initial state
 */
void cpu_reset(VBoxCPU *cpu);

/**
 * Execute a single instruction
 * Returns error code or VBOX_OK
 */
VBoxError cpu_step(VBoxCPU *cpu, u8 *memory);

/**
 * Run CPU until halted or error
 */
VBoxError cpu_run(VBoxCPU *cpu, u8 *memory);

/**
 * Trigger an interrupt
 */
void cpu_interrupt(VBoxCPU *cpu, u8 vector);

/*============================================================================
 * Flag Manipulation Macros
 *============================================================================*/

#define CPU_GET_FLAG(cpu, flag) (((cpu)->flags & (flag)) != 0)
#define CPU_SET_FLAG(cpu, flag) ((cpu)->flags |= (flag))
#define CPU_CLEAR_FLAG(cpu, flag) ((cpu)->flags &= ~(flag))
#define CPU_SET_FLAG_IF(cpu, flag, cond)                                       \
  do {                                                                         \
    if (cond)                                                                  \
      CPU_SET_FLAG(cpu, flag);                                                 \
    else                                                                       \
      CPU_CLEAR_FLAG(cpu, flag);                                               \
  } while (0)

/* Common flag checks */
#define CPU_CF(cpu) CPU_GET_FLAG(cpu, FLAG_CF)
#define CPU_ZF(cpu) CPU_GET_FLAG(cpu, FLAG_ZF)
#define CPU_SF(cpu) CPU_GET_FLAG(cpu, FLAG_SF)
#define CPU_OF(cpu) CPU_GET_FLAG(cpu, FLAG_OF)
#define CPU_PF(cpu) CPU_GET_FLAG(cpu, FLAG_PF)
#define CPU_AF(cpu) CPU_GET_FLAG(cpu, FLAG_AF)
#define CPU_DF(cpu) CPU_GET_FLAG(cpu, FLAG_DF)
#define CPU_IF(cpu) CPU_GET_FLAG(cpu, FLAG_IF)

/*============================================================================
 * Address Calculation
 *============================================================================*/

/**
 * Calculate linear address from segment:offset
 */
static inline u32 cpu_linear_addr(u16 segment, u16 offset) {
  return ((u32)segment << 4) + offset;
}

/**
 * Get effective segment (considering segment override)
 */
static inline u16 cpu_effective_segment(VBoxCPU *cpu, u16 default_seg) {
  if (cpu->prefix_segment) {
    switch (cpu->prefix_segment) {
    case PREFIX_ES:
      return cpu->es;
    case PREFIX_CS:
      return cpu->cs;
    case PREFIX_SS:
      return cpu->ss;
    case PREFIX_DS:
      return cpu->ds;
    }
  }
  return default_seg;
}

#ifdef __cplusplus
}
#endif

#endif /* VBOX_CPU_H */
