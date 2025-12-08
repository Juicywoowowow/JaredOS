/*
 * VBox - Simple x86 Emulator
 * ModR/M Byte Handling
 */

#include "vbox/cpu.h"
#include "vbox/memory.h"

/*============================================================================
 * ModR/M Byte Format
 *
 * 7  6   5  4  3   2  1  0
 * +----+--------+--------+
 * |mod |  reg   |  r/m   |
 * +----+--------+--------+
 *
 * mod: 00 = memory, no displacement (except [BP] = disp16)
 *      01 = memory + disp8
 *      10 = memory + disp16
 *      11 = register
 *============================================================================*/

typedef struct {
  u8 mod;
  u8 reg;
  u8 rm;
  u16 ea;      /* Effective address (for memory operands) */
  u16 disp;    /* Displacement value */
  u16 segment; /* Segment to use */
} ModRM;

/*============================================================================
 * Register Access by Index
 *============================================================================*/

/* Get pointer to 8-bit register by index (0-7: AL,CL,DL,BL,AH,CH,DH,BH) */
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
  return &cpu->a.l; /* Unreachable */
}

/* Get pointer to 16-bit register by index (0-7: AX,CX,DX,BX,SP,BP,SI,DI) */
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
  return &cpu->a.x; /* Unreachable */
}

/* Get segment register by index (0-3: ES,CS,SS,DS) */
static u16 *get_seg_reg(VBoxCPU *cpu, u8 index) {
  switch (index & 3) {
  case 0:
    return &cpu->es;
  case 1:
    return &cpu->cs;
  case 2:
    return &cpu->ss;
  case 3:
    return &cpu->ds;
  }
  return &cpu->ds;
}

/*============================================================================
 * ModR/M Decoding
 *============================================================================*/

/**
 * Decode ModR/M byte and calculate effective address
 */
static void decode_modrm(VBoxCPU *cpu, u8 *memory, ModRM *modrm) {
  u32 addr = cpu_linear_addr(cpu->cs, cpu->ip);
  u8 byte = memory[addr & (VBOX_MEMORY_SIZE - 1)];
  cpu->ip++;

  modrm->mod = (byte >> 6) & 3;
  modrm->reg = (byte >> 3) & 7;
  modrm->rm = byte & 7;
  modrm->disp = 0;
  modrm->ea = 0;

  /* Default segment is DS, except for BP-relative which uses SS */
  modrm->segment = cpu->ds;

  if (modrm->mod == 3) {
    /* Register operand, no memory access needed */
    return;
  }

  /* Calculate effective address based on r/m field */
  switch (modrm->rm) {
  case 0:
    modrm->ea = cpu->b.x + cpu->si;
    break; /* [BX+SI] */
  case 1:
    modrm->ea = cpu->b.x + cpu->di;
    break; /* [BX+DI] */
  case 2:
    modrm->ea = cpu->bp + cpu->si;
    modrm->segment = cpu->ss;
    break; /* [BP+SI] */
  case 3:
    modrm->ea = cpu->bp + cpu->di;
    modrm->segment = cpu->ss;
    break; /* [BP+DI] */
  case 4:
    modrm->ea = cpu->si;
    break; /* [SI] */
  case 5:
    modrm->ea = cpu->di;
    break; /* [DI] */
  case 6:
    if (modrm->mod == 0) {
      /* Direct address - fetch 16-bit displacement */
      addr = cpu_linear_addr(cpu->cs, cpu->ip);
      modrm->disp = memory[addr & (VBOX_MEMORY_SIZE - 1)];
      cpu->ip++;
      modrm->disp |= (u16)memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8;
      cpu->ip++;
      modrm->ea = modrm->disp;
      return;
    }
    modrm->ea = cpu->bp;
    modrm->segment = cpu->ss;
    break;
  case 7:
    modrm->ea = cpu->b.x;
    break; /* [BX] */
  }

  /* Fetch displacement */
  if (modrm->mod == 1) {
    /* 8-bit signed displacement */
    addr = cpu_linear_addr(cpu->cs, cpu->ip);
    i8 disp8 = (i8)memory[addr & (VBOX_MEMORY_SIZE - 1)];
    cpu->ip++;
    modrm->ea += (i16)disp8;
  } else if (modrm->mod == 2) {
    /* 16-bit displacement */
    addr = cpu_linear_addr(cpu->cs, cpu->ip);
    modrm->disp = memory[addr & (VBOX_MEMORY_SIZE - 1)];
    cpu->ip++;
    modrm->disp |= (u16)memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8;
    cpu->ip++;
    modrm->ea += modrm->disp;
  }

  /* Apply segment override if present */
  if (cpu->prefix_segment) {
    switch (cpu->prefix_segment) {
    case PREFIX_ES:
      modrm->segment = cpu->es;
      break;
    case PREFIX_CS:
      modrm->segment = cpu->cs;
      break;
    case PREFIX_SS:
      modrm->segment = cpu->ss;
      break;
    case PREFIX_DS:
      modrm->segment = cpu->ds;
      break;
    }
  }
}

/**
 * Read 8-bit value from ModR/M operand
 */
static u8 modrm_read8(VBoxCPU *cpu, u8 *memory, ModRM *modrm) {
  if (modrm->mod == 3) {
    return *get_reg8(cpu, modrm->rm);
  }
  u32 addr = cpu_linear_addr(modrm->segment, modrm->ea);
  return memory[addr & (VBOX_MEMORY_SIZE - 1)];
}

/**
 * Read 16-bit value from ModR/M operand
 */
static u16 modrm_read16(VBoxCPU *cpu, u8 *memory, ModRM *modrm) {
  if (modrm->mod == 3) {
    return *get_reg16(cpu, modrm->rm);
  }
  u32 addr = cpu_linear_addr(modrm->segment, modrm->ea);
  return memory[addr & (VBOX_MEMORY_SIZE - 1)] |
         ((u16)memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8);
}

/**
 * Write 8-bit value to ModR/M operand
 */
static void modrm_write8(VBoxCPU *cpu, u8 *memory, ModRM *modrm, u8 value) {
  if (modrm->mod == 3) {
    *get_reg8(cpu, modrm->rm) = value;
    return;
  }
  u32 addr = cpu_linear_addr(modrm->segment, modrm->ea);
  memory[addr & (VBOX_MEMORY_SIZE - 1)] = value;
}

/**
 * Write 16-bit value to ModR/M operand
 */
static void modrm_write16(VBoxCPU *cpu, u8 *memory, ModRM *modrm, u16 value) {
  if (modrm->mod == 3) {
    *get_reg16(cpu, modrm->rm) = value;
    return;
  }
  u32 addr = cpu_linear_addr(modrm->segment, modrm->ea);
  memory[addr & (VBOX_MEMORY_SIZE - 1)] = (u8)(value & 0xFF);
  memory[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] = (u8)(value >> 8);
}
