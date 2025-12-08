/*
 * VBox - Simple x86 Emulator
 * CPU State Management
 */

#include "vbox/cpu.h"
#include "vbox/memory.h"
#include <string.h>

/*============================================================================
 * CPU Initialization
 *============================================================================*/

void cpu_init(VBoxCPU *cpu) { cpu_reset(cpu); }

void cpu_reset(VBoxCPU *cpu) {
  /* Clear all state */
  memset(cpu, 0, sizeof(VBoxCPU));

  /* Set default register values */
  cpu->a.x = 0;
  cpu->b.x = 0;
  cpu->c.x = 0;
  cpu->d.x = 0;
  cpu->si = 0;
  cpu->di = 0;
  cpu->bp = VBOX_DEFAULT_BP;
  cpu->sp = VBOX_DEFAULT_SP;

  /* Segment registers - start at 0 or boot sector location */
  cpu->cs = VBOX_DEFAULT_CS;
  cpu->ds = VBOX_DEFAULT_DS;
  cpu->es = VBOX_DEFAULT_ES;
  cpu->ss = VBOX_DEFAULT_SS;

  /* For boot sector, IP starts at 0x7C00 */
  cpu->ip = 0x7C00;

  /* Default flags: interrupts disabled initially */
  cpu->flags = 0x0002; /* Reserved bit 1 always set */

  cpu->halted = false;
  cpu->interrupt_pending = false;
  cpu->pending_interrupt = 0;
  cpu->cycles = 0;
}

/*============================================================================
 * Instruction Decoding Helpers
 *============================================================================*/

/* Fetch byte at CS:IP and increment IP */
static inline u8 fetch_byte(VBoxCPU *cpu, u8 *memory) {
  u32 addr = cpu_linear_addr(cpu->cs, cpu->ip);
  u8 byte = memory[addr & (VBOX_MEMORY_SIZE - 1)];
  cpu->ip++;
  return byte;
}

/* Fetch word at CS:IP and increment IP by 2 */
static inline u16 fetch_word(VBoxCPU *cpu, u8 *memory) {
  u8 lo = fetch_byte(cpu, memory);
  u8 hi = fetch_byte(cpu, memory);
  return (u16)lo | ((u16)hi << 8);
}

/* Forward declaration - implemented in execute.c */
extern VBoxError execute_instruction(VBoxCPU *cpu, u8 *memory, u8 opcode);

/*============================================================================
 * CPU Execution
 *============================================================================*/

VBoxError cpu_step(VBoxCPU *cpu, u8 *memory) {
  if (cpu->halted) {
    return VBOX_ERR_HALT;
  }

  /* Clear prefix state */
  cpu->prefix_segment = 0;
  cpu->prefix_rep = false;
  cpu->prefix_repne = false;

  /* Handle pending interrupt if interrupts enabled */
  if (cpu->interrupt_pending && CPU_GET_FLAG(cpu, FLAG_IF)) {
    cpu_interrupt(cpu, cpu->pending_interrupt);
    cpu->interrupt_pending = false;
  }

  /* Fetch opcode */
  u8 opcode = fetch_byte(cpu, memory);

  /* Handle prefixes */
  while (1) {
    switch (opcode) {
    case PREFIX_ES:
    case PREFIX_CS:
    case PREFIX_SS:
    case PREFIX_DS:
      cpu->prefix_segment = opcode;
      opcode = fetch_byte(cpu, memory);
      continue;

    case PREFIX_REP:
      cpu->prefix_rep = true;
      opcode = fetch_byte(cpu, memory);
      continue;

    case PREFIX_REPNE:
      cpu->prefix_repne = true;
      opcode = fetch_byte(cpu, memory);
      continue;

    case PREFIX_LOCK:
      /* Ignore LOCK prefix for now */
      opcode = fetch_byte(cpu, memory);
      continue;

    default:
      break;
    }
    break;
  }

  /* Execute instruction */
  VBoxError err = execute_instruction(cpu, memory, opcode);
  if (err != VBOX_OK) {
    return err;
  }

  cpu->cycles++;
  return VBOX_OK;
}

VBoxError cpu_run(VBoxCPU *cpu, u8 *memory) {
  VBoxError err;

  while (!cpu->halted) {
    err = cpu_step(cpu, memory);
    if (err != VBOX_OK && err != VBOX_ERR_HALT) {
      return err;
    }
  }

  return VBOX_OK;
}

/*============================================================================
 * Interrupt Handling
 *============================================================================*/

void cpu_interrupt(VBoxCPU *cpu, u8 vector) {
  /* Push FLAGS, CS, IP onto stack */
  cpu->sp -= 2;
  /* Note: This would write to memory, but we need the memory pointer */
  /* This is a stub - actual implementation in execute.c */
}
