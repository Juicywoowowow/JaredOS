/*
 * VBox - Simple x86 Emulator
 * EFLAGS Register Handling
 */

#include "vbox/cpu.h"

/*============================================================================
 * Parity Lookup Table
 * Parity flag is set if the low byte has an even number of 1s
 *============================================================================*/

static const u8 parity_table[256] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};

/*============================================================================
 * Flag Update Functions
 *============================================================================*/

/**
 * Update Zero and Sign flags based on 8-bit result
 */
void flags_update_zs8(VBoxCPU *cpu, u8 result) {
  CPU_SET_FLAG_IF(cpu, FLAG_ZF, result == 0);
  CPU_SET_FLAG_IF(cpu, FLAG_SF, (result & 0x80) != 0);
  CPU_SET_FLAG_IF(cpu, FLAG_PF, parity_table[result]);
}

/**
 * Update Zero and Sign flags based on 16-bit result
 */
void flags_update_zs16(VBoxCPU *cpu, u16 result) {
  CPU_SET_FLAG_IF(cpu, FLAG_ZF, result == 0);
  CPU_SET_FLAG_IF(cpu, FLAG_SF, (result & 0x8000) != 0);
  CPU_SET_FLAG_IF(cpu, FLAG_PF, parity_table[result & 0xFF]);
}

/**
 * Update all arithmetic flags for 8-bit addition
 */
void flags_update_add8(VBoxCPU *cpu, u8 op1, u8 op2, u16 result) {
  u8 res8 = (u8)result;

  flags_update_zs8(cpu, res8);

  /* Carry: result > 0xFF */
  CPU_SET_FLAG_IF(cpu, FLAG_CF, result > 0xFF);

  /* Overflow: sign of operands same, sign of result different */
  CPU_SET_FLAG_IF(cpu, FLAG_OF, ((op1 ^ res8) & (op2 ^ res8) & 0x80) != 0);

  /* Auxiliary carry: carry from bit 3 to bit 4 */
  CPU_SET_FLAG_IF(cpu, FLAG_AF, ((op1 ^ op2 ^ res8) & 0x10) != 0);
}

/**
 * Update all arithmetic flags for 16-bit addition
 */
void flags_update_add16(VBoxCPU *cpu, u16 op1, u16 op2, u32 result) {
  u16 res16 = (u16)result;

  flags_update_zs16(cpu, res16);

  CPU_SET_FLAG_IF(cpu, FLAG_CF, result > 0xFFFF);
  CPU_SET_FLAG_IF(cpu, FLAG_OF, ((op1 ^ res16) & (op2 ^ res16) & 0x8000) != 0);
  CPU_SET_FLAG_IF(cpu, FLAG_AF, ((op1 ^ op2 ^ res16) & 0x10) != 0);
}

/**
 * Update all arithmetic flags for 8-bit subtraction
 */
void flags_update_sub8(VBoxCPU *cpu, u8 op1, u8 op2, u16 result) {
  u8 res8 = (u8)result;

  flags_update_zs8(cpu, res8);

  /* Borrow: result wrapped around */
  CPU_SET_FLAG_IF(cpu, FLAG_CF, op1 < op2);

  /* Overflow: sign of operands different, sign of result different from op1 */
  CPU_SET_FLAG_IF(cpu, FLAG_OF, ((op1 ^ op2) & (op1 ^ res8) & 0x80) != 0);

  /* Auxiliary borrow */
  CPU_SET_FLAG_IF(cpu, FLAG_AF, ((op1 ^ op2 ^ res8) & 0x10) != 0);
}

/**
 * Update all arithmetic flags for 16-bit subtraction
 */
void flags_update_sub16(VBoxCPU *cpu, u16 op1, u16 op2, u32 result) {
  u16 res16 = (u16)result;

  flags_update_zs16(cpu, res16);

  CPU_SET_FLAG_IF(cpu, FLAG_CF, op1 < op2);
  CPU_SET_FLAG_IF(cpu, FLAG_OF, ((op1 ^ op2) & (op1 ^ res16) & 0x8000) != 0);
  CPU_SET_FLAG_IF(cpu, FLAG_AF, ((op1 ^ op2 ^ res16) & 0x10) != 0);
}

/**
 * Update flags for logical operations (AND, OR, XOR)
 * CF and OF are cleared, AF is undefined
 */
void flags_update_logic8(VBoxCPU *cpu, u8 result) {
  flags_update_zs8(cpu, result);
  CPU_CLEAR_FLAG(cpu, FLAG_CF);
  CPU_CLEAR_FLAG(cpu, FLAG_OF);
}

void flags_update_logic16(VBoxCPU *cpu, u16 result) {
  flags_update_zs16(cpu, result);
  CPU_CLEAR_FLAG(cpu, FLAG_CF);
  CPU_CLEAR_FLAG(cpu, FLAG_OF);
}
