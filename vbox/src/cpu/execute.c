/*
 * VBox - Simple x86 Emulator
 * Instruction Execution
 */

#include "vbox/cpu.h"
#include "vbox/memory.h"
#include <stdio.h>

/*============================================================================
 * Forward Declarations
 *============================================================================*/

/* From flags.c */
extern void flags_update_zs8(VBoxCPU *cpu, u8 result);
extern void flags_update_zs16(VBoxCPU *cpu, u16 result);
extern void flags_update_add8(VBoxCPU *cpu, u8 op1, u8 op2, u16 result);
extern void flags_update_add16(VBoxCPU *cpu, u16 op1, u16 op2, u32 result);
extern void flags_update_sub8(VBoxCPU *cpu, u8 op1, u8 op2, u16 result);
extern void flags_update_sub16(VBoxCPU *cpu, u16 op1, u16 op2, u32 result);
extern void flags_update_logic8(VBoxCPU *cpu, u8 result);
extern void flags_update_logic16(VBoxCPU *cpu, u16 result);

/*============================================================================
 * Helper Functions
 *============================================================================*/

static inline u8 fetch_byte(VBoxCPU *cpu, u8 *memory) {
  u32 addr = cpu_linear_addr(cpu->cs, cpu->ip);
  u8 byte = memory[addr & (VBOX_MEMORY_SIZE - 1)];
  cpu->ip++;
  return byte;
}

static inline u16 fetch_word(VBoxCPU *cpu, u8 *memory) {
  u8 lo = fetch_byte(cpu, memory);
  u8 hi = fetch_byte(cpu, memory);
  return (u16)lo | ((u16)hi << 8);
}

static inline void push16(VBoxCPU *cpu, u8 *memory, u16 value) {
  cpu->sp -= 2;
  u32 addr = cpu_linear_addr(cpu->ss, cpu->sp);
  memory[addr & (VBOX_MEMORY_SIZE - 1)] = (u8)(value & 0xFF);
  memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] = (u8)(value >> 8);
}

static inline u16 pop16(VBoxCPU *cpu, u8 *memory) {
  u32 addr = cpu_linear_addr(cpu->ss, cpu->sp);
  u16 value = memory[addr & (VBOX_MEMORY_SIZE - 1)] |
              ((u16)memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8);
  cpu->sp += 2;
  return value;
}

/*============================================================================
 * Register Access by Index
 *============================================================================*/

static u8 *get_reg8(VBoxCPU *cpu, u8 index) {
  switch (index & 7) {
  case 0:
    return &cpu->a.l;
  case 1:
    return &cpu->c.l;
  case 2:
    return &cpu->d.l;
  case 3:
    return &cpu->b.l;
  case 4:
    return &cpu->a.h;
  case 5:
    return &cpu->c.h;
  case 6:
    return &cpu->d.h;
  case 7:
    return &cpu->b.h;
  }
  return &cpu->a.l;
}

static u16 *get_reg16(VBoxCPU *cpu, u8 index) {
  switch (index & 7) {
  case 0:
    return &cpu->a.x;
  case 1:
    return &cpu->c.x;
  case 2:
    return &cpu->d.x;
  case 3:
    return &cpu->b.x;
  case 4:
    return &cpu->sp;
  case 5:
    return &cpu->bp;
  case 6:
    return &cpu->si;
  case 7:
    return &cpu->di;
  }
  return &cpu->a.x;
}

/*============================================================================
 * Main Instruction Execution
 *============================================================================*/

VBoxError execute_instruction(VBoxCPU *cpu, u8 *memory, u8 opcode) {
  u8 imm8;
  u16 imm16;
  i8 rel8;
  i16 rel16;
  u32 result;

  switch (opcode) {
  /*====================================================================
   * NOP (0x90)
   *====================================================================*/
  case 0x90:
    /* No operation */
    break;

  /*====================================================================
   * HLT (0xF4)
   *====================================================================*/
  case 0xF4:
    cpu->halted = true;
    return VBOX_ERR_HALT;

  /*====================================================================
   * MOV r8, imm8 (0xB0 - 0xB7)
   *====================================================================*/
  case 0xB0:
  case 0xB1:
  case 0xB2:
  case 0xB3:
  case 0xB4:
  case 0xB5:
  case 0xB6:
  case 0xB7:
    imm8 = fetch_byte(cpu, memory);
    *get_reg8(cpu, opcode - 0xB0) = imm8;
    break;

  /*====================================================================
   * MOV r16, imm16 (0xB8 - 0xBF)
   *====================================================================*/
  case 0xB8:
  case 0xB9:
  case 0xBA:
  case 0xBB:
  case 0xBC:
  case 0xBD:
  case 0xBE:
  case 0xBF:
    imm16 = fetch_word(cpu, memory);
    *get_reg16(cpu, opcode - 0xB8) = imm16;
    break;

  /*====================================================================
   * PUSH r16 (0x50 - 0x57)
   *====================================================================*/
  case 0x50:
  case 0x51:
  case 0x52:
  case 0x53:
  case 0x54:
  case 0x55:
  case 0x56:
  case 0x57:
    push16(cpu, memory, *get_reg16(cpu, opcode - 0x50));
    break;

  /*====================================================================
   * POP r16 (0x58 - 0x5F)
   *====================================================================*/
  case 0x58:
  case 0x59:
  case 0x5A:
  case 0x5B:
  case 0x5C:
  case 0x5D:
  case 0x5E:
  case 0x5F:
    *get_reg16(cpu, opcode - 0x58) = pop16(cpu, memory);
    break;

  /*====================================================================
   * INC r16 (0x40 - 0x47)
   *====================================================================*/
  case 0x40:
  case 0x41:
  case 0x42:
  case 0x43:
  case 0x44:
  case 0x45:
  case 0x46:
  case 0x47: {
    u16 *reg = get_reg16(cpu, opcode - 0x40);
    u16 old = *reg;
    (*reg)++;
    /* INC doesn't affect CF */
    u8 old_cf = cpu->flags & FLAG_CF;
    flags_update_add16(cpu, old, 1, (u32)old + 1);
    cpu->flags = (cpu->flags & ~FLAG_CF) | old_cf;
    break;
  }

  /*====================================================================
   * DEC r16 (0x48 - 0x4F)
   *====================================================================*/
  case 0x48:
  case 0x49:
  case 0x4A:
  case 0x4B:
  case 0x4C:
  case 0x4D:
  case 0x4E:
  case 0x4F: {
    u16 *reg = get_reg16(cpu, opcode - 0x48);
    u16 old = *reg;
    (*reg)--;
    u8 old_cf = cpu->flags & FLAG_CF;
    flags_update_sub16(cpu, old, 1, (u32)old - 1);
    cpu->flags = (cpu->flags & ~FLAG_CF) | old_cf;
    break;
  }

  /*====================================================================
   * ADD AL, imm8 (0x04)
   *====================================================================*/
  case 0x04:
    imm8 = fetch_byte(cpu, memory);
    result = cpu->a.l + imm8;
    flags_update_add8(cpu, cpu->a.l, imm8, result);
    cpu->a.l = (u8)result;
    break;

  /*====================================================================
   * ADD AX, imm16 (0x05)
   *====================================================================*/
  case 0x05:
    imm16 = fetch_word(cpu, memory);
    result = cpu->a.x + imm16;
    flags_update_add16(cpu, cpu->a.x, imm16, result);
    cpu->a.x = (u16)result;
    break;

  /*====================================================================
   * SUB AL, imm8 (0x2C)
   *====================================================================*/
  case 0x2C:
    imm8 = fetch_byte(cpu, memory);
    result = cpu->a.l - imm8;
    flags_update_sub8(cpu, cpu->a.l, imm8, result);
    cpu->a.l = (u8)result;
    break;

  /*====================================================================
   * SUB AX, imm16 (0x2D)
   *====================================================================*/
  case 0x2D:
    imm16 = fetch_word(cpu, memory);
    result = cpu->a.x - imm16;
    flags_update_sub16(cpu, cpu->a.x, imm16, result);
    cpu->a.x = (u16)result;
    break;

  /*====================================================================
   * CMP AL, imm8 (0x3C)
   *====================================================================*/
  case 0x3C:
    imm8 = fetch_byte(cpu, memory);
    result = cpu->a.l - imm8;
    flags_update_sub8(cpu, cpu->a.l, imm8, result);
    break;

  /*====================================================================
   * CMP AX, imm16 (0x3D)
   *====================================================================*/
  case 0x3D:
    imm16 = fetch_word(cpu, memory);
    result = cpu->a.x - imm16;
    flags_update_sub16(cpu, cpu->a.x, imm16, result);
    break;

  /*====================================================================
   * AND AL, imm8 (0x24)
   *====================================================================*/
  case 0x24:
    imm8 = fetch_byte(cpu, memory);
    cpu->a.l &= imm8;
    flags_update_logic8(cpu, cpu->a.l);
    break;

  /*====================================================================
   * OR AL, imm8 (0x0C)
   *====================================================================*/
  case 0x0C:
    imm8 = fetch_byte(cpu, memory);
    cpu->a.l |= imm8;
    flags_update_logic8(cpu, cpu->a.l);
    break;

  /*====================================================================
   * XOR AL, imm8 (0x34)
   *====================================================================*/
  case 0x34:
    imm8 = fetch_byte(cpu, memory);
    cpu->a.l ^= imm8;
    flags_update_logic8(cpu, cpu->a.l);
    break;

  /*====================================================================
   * JMP rel8 (0xEB)
   *====================================================================*/
  case 0xEB:
    rel8 = (i8)fetch_byte(cpu, memory);
    cpu->ip += rel8;
    break;

  /*====================================================================
   * JMP rel16 (0xE9)
   *====================================================================*/
  case 0xE9:
    rel16 = (i16)fetch_word(cpu, memory);
    cpu->ip += rel16;
    break;

  /*====================================================================
   * Conditional Jumps (0x70 - 0x7F)
   *====================================================================*/
  case 0x70: /* JO */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_OF(cpu))
      cpu->ip += rel8;
    break;
  case 0x71: /* JNO */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_OF(cpu))
      cpu->ip += rel8;
    break;
  case 0x72: /* JB/JC/JNAE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_CF(cpu))
      cpu->ip += rel8;
    break;
  case 0x73: /* JNB/JNC/JAE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_CF(cpu))
      cpu->ip += rel8;
    break;
  case 0x74: /* JZ/JE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_ZF(cpu))
      cpu->ip += rel8;
    break;
  case 0x75: /* JNZ/JNE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_ZF(cpu))
      cpu->ip += rel8;
    break;
  case 0x76: /* JBE/JNA */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_CF(cpu) || CPU_ZF(cpu))
      cpu->ip += rel8;
    break;
  case 0x77: /* JNBE/JA */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_CF(cpu) && !CPU_ZF(cpu))
      cpu->ip += rel8;
    break;
  case 0x78: /* JS */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_SF(cpu))
      cpu->ip += rel8;
    break;
  case 0x79: /* JNS */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_SF(cpu))
      cpu->ip += rel8;
    break;
  case 0x7A: /* JP/JPE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_PF(cpu))
      cpu->ip += rel8;
    break;
  case 0x7B: /* JNP/JPO */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_PF(cpu))
      cpu->ip += rel8;
    break;
  case 0x7C: /* JL/JNGE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_SF(cpu) != CPU_OF(cpu))
      cpu->ip += rel8;
    break;
  case 0x7D: /* JNL/JGE */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_SF(cpu) == CPU_OF(cpu))
      cpu->ip += rel8;
    break;
  case 0x7E: /* JLE/JNG */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (CPU_ZF(cpu) || (CPU_SF(cpu) != CPU_OF(cpu)))
      cpu->ip += rel8;
    break;
  case 0x7F: /* JNLE/JG */
    rel8 = (i8)fetch_byte(cpu, memory);
    if (!CPU_ZF(cpu) && (CPU_SF(cpu) == CPU_OF(cpu)))
      cpu->ip += rel8;
    break;

  /*====================================================================
   * CALL rel16 (0xE8)
   *====================================================================*/
  case 0xE8:
    rel16 = (i16)fetch_word(cpu, memory);
    push16(cpu, memory, cpu->ip);
    cpu->ip += rel16;
    break;

  /*====================================================================
   * RET (0xC3)
   *====================================================================*/
  case 0xC3:
    cpu->ip = pop16(cpu, memory);
    break;

  /*====================================================================
   * LOOP (0xE2)
   *====================================================================*/
  case 0xE2:
    rel8 = (i8)fetch_byte(cpu, memory);
    cpu->c.x--;
    if (cpu->c.x != 0) {
      cpu->ip += rel8;
    }
    break;

  /*====================================================================
   * INT imm8 (0xCD)
   *====================================================================*/
  case 0xCD:
    imm8 = fetch_byte(cpu, memory);
    /* Save state for interrupt return */
    push16(cpu, memory, cpu->flags);
    push16(cpu, memory, cpu->cs);
    push16(cpu, memory, cpu->ip);
    /* Clear IF and TF */
    CPU_CLEAR_FLAG(cpu, FLAG_IF);
    CPU_CLEAR_FLAG(cpu, FLAG_TF);
    /* Load CS:IP from IVT */
    {
      u32 ivt_addr = (u32)imm8 * 4;
      cpu->ip = memory[ivt_addr] | ((u16)memory[ivt_addr + 1] << 8);
      cpu->cs = memory[ivt_addr + 2] | ((u16)memory[ivt_addr + 3] << 8);
    }
    break;

  /*====================================================================
   * IRET (0xCF)
   *====================================================================*/
  case 0xCF:
    cpu->ip = pop16(cpu, memory);
    cpu->cs = pop16(cpu, memory);
    cpu->flags = pop16(cpu, memory);
    break;

  /*====================================================================
   * CLI (0xFA) - Clear Interrupt Flag
   *====================================================================*/
  case 0xFA:
    CPU_CLEAR_FLAG(cpu, FLAG_IF);
    break;

  /*====================================================================
   * STI (0xFB) - Set Interrupt Flag
   *====================================================================*/
  case 0xFB:
    CPU_SET_FLAG(cpu, FLAG_IF);
    break;

  /*====================================================================
   * CLD (0xFC) - Clear Direction Flag
   *====================================================================*/
  case 0xFC:
    CPU_CLEAR_FLAG(cpu, FLAG_DF);
    break;

  /*====================================================================
   * STD (0xFD) - Set Direction Flag
   *====================================================================*/
  case 0xFD:
    CPU_SET_FLAG(cpu, FLAG_DF);
    break;

  /*====================================================================
   * CLC (0xF8) - Clear Carry Flag
   *====================================================================*/
  case 0xF8:
    CPU_CLEAR_FLAG(cpu, FLAG_CF);
    break;

  /*====================================================================
   * STC (0xF9) - Set Carry Flag
   *====================================================================*/
  case 0xF9:
    CPU_SET_FLAG(cpu, FLAG_CF);
    break;

  /*====================================================================
   * PUSHF (0x9C)
   *====================================================================*/
  case 0x9C:
    push16(cpu, memory, cpu->flags);
    break;

  /*====================================================================
   * POPF (0x9D)
   *====================================================================*/
  case 0x9D:
    cpu->flags = pop16(cpu, memory);
    break;

  /*====================================================================
   * XCHG AX, r16 (0x91 - 0x97)
   *====================================================================*/
  case 0x91:
  case 0x92:
  case 0x93:
  case 0x94:
  case 0x95:
  case 0x96:
  case 0x97: {
    u16 *reg = get_reg16(cpu, opcode - 0x90);
    u16 temp = cpu->a.x;
    cpu->a.x = *reg;
    *reg = temp;
    break;
  }

  /*====================================================================
   * MOV AX, moffs16 (0xA1)
   *====================================================================*/
  case 0xA1: {
    u16 offset = fetch_word(cpu, memory);
    u16 segment = cpu_effective_segment(cpu, cpu->ds);
    u32 addr = cpu_linear_addr(segment, offset);
    cpu->a.x = memory[addr & (VBOX_MEMORY_SIZE - 1)] |
               ((u16)memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8);
    break;
  }

  /*====================================================================
   * MOV moffs16, AX (0xA3)
   *====================================================================*/
  case 0xA3: {
    u16 offset = fetch_word(cpu, memory);
    u16 segment = cpu_effective_segment(cpu, cpu->ds);
    u32 addr = cpu_linear_addr(segment, offset);
    memory[addr & (VBOX_MEMORY_SIZE - 1)] = (u8)(cpu->a.x & 0xFF);
    memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] = (u8)(cpu->a.x >> 8);
    break;
  }

  /*====================================================================
   * Unknown opcode
   *====================================================================*/
  default:
    fprintf(stderr, "Unknown opcode: 0x%02X at CS:IP = %04X:%04X\n", opcode,
            cpu->cs, cpu->ip - 1);
    return VBOX_ERR_INVALID_OPCODE;
  }

  return VBOX_OK;
}
