/*
 * =============================================================================
 * kernelA.c - Core Kernel: GDT, IDT, and System Initialization
 * =============================================================================
 *
 * This is the "brain" of FoxOS - Part A. It handles:
 *   1. GDT (Global Descriptor Table) setup
 *   2. IDT (Interrupt Descriptor Table) setup
 *   3. PIC (Programmable Interrupt Controller) remapping
 *   4. System initialization orchestration
 *
 * The GDT defines memory segments for protected mode.
 * The IDT defines how CPU exceptions and hardware interrupts are handled.
 *
 * DEBUGGING TIPS:
 *   - If you get a triple fault, the GDT or IDT is probably wrong
 *   - Use QEMU's -d int flag to see interrupt activity
 *   - General Protection Faults (#13) usually mean segment issues
 *   - Page Faults (#14) mean paging problems (see kernelB.c)
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: GDT Structures and Data
 * =============================================================================
 */

/*
 * GDT Entry Structure
 * Each entry defines a memory segment with base address, limit, and flags.
 * We use a flat memory model where code and data segments overlap.
 */
struct gdt_entry {
  uint16_t limit_low;  /* Lower 16 bits of segment limit */
  uint16_t base_low;   /* Lower 16 bits of base address */
  uint8_t base_middle; /* Middle 8 bits of base address */
  uint8_t access;      /* Access flags: Present, Ring, Type */
  uint8_t granularity; /* Flags and upper 4 bits of limit */
  uint8_t base_high;   /* Upper 8 bits of base address */
} __attribute__((packed));

/* GDT Pointer - passed to LGDT instruction */
struct gdt_ptr {
  uint16_t limit; /* Size of GDT minus 1 */
  uint32_t base;  /* Linear address of GDT */
} __attribute__((packed));

/* Our GDT with 5 entries: Null, Code, Data, User Code, User Data */
#define GDT_ENTRIES 5
static struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gdt_ptr;

/* Assembly function to load GDT (defined in kernel_entry.asm) */
extern void gdt_flush(void);

/*
 * gdt_set_entry - Set up a single GDT entry
 *
 * @param index: Which GDT entry to set (0-4)
 * @param base: Segment base address
 * @param limit: Segment limit
 * @param access: Access byte flags
 * @param gran: Granularity byte flags
 */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
  gdt[index].base_low = (base & 0xFFFF);
  gdt[index].base_middle = (base >> 16) & 0xFF;
  gdt[index].base_high = (base >> 24) & 0xFF;

  gdt[index].limit_low = (limit & 0xFFFF);
  gdt[index].granularity = (limit >> 16) & 0x0F;
  gdt[index].granularity |= gran & 0xF0;

  gdt[index].access = access;
}

/*
 * gdt_init - Initialize the Global Descriptor Table
 *
 * Sets up a flat memory model with overlapping segments.
 * Ring 0 (kernel) and Ring 3 (user) segments are created.
 */
void gdt_init(void) {
  debug_print("[GDT] Initializing Global Descriptor Table\n");

  gdt_ptr.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
  gdt_ptr.base = (uint32_t)&gdt;

  /* Entry 0: Null Descriptor (required by CPU) */
  gdt_set_entry(0, 0, 0, 0, 0);

  /* Entry 1: Kernel Code Segment
   * Base=0, Limit=4GB, Execute/Read, Ring 0 */
  gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  /* Entry 2: Kernel Data Segment
   * Base=0, Limit=4GB, Read/Write, Ring 0 */
  gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  /* Entry 3: User Code Segment
   * Base=0, Limit=4GB, Execute/Read, Ring 3 */
  gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

  /* Entry 4: User Data Segment
   * Base=0, Limit=4GB, Read/Write, Ring 3 */
  gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

  /* Load the new GDT */
  gdt_flush();

  debug_print("[GDT] GDT initialized successfully\n");
}

/* =============================================================================
 * SECTION 2: IDT Structures and Data
 * =============================================================================
 */

/*
 * IDT Entry Structure
 * Each entry defines how to handle an interrupt or exception.
 */
struct idt_entry {
  uint16_t base_low;  /* Lower 16 bits of handler address */
  uint16_t selector;  /* Kernel code segment selector */
  uint8_t zero;       /* Always zero */
  uint8_t flags;      /* Type and attributes */
  uint16_t base_high; /* Upper 16 bits of handler address */
} __attribute__((packed));

/* IDT Pointer - passed to LIDT instruction */
struct idt_ptr {
  uint16_t limit; /* Size of IDT minus 1 */
  uint32_t base;  /* Linear address of IDT */
} __attribute__((packed));

/* We support 256 interrupt vectors */
#define IDT_ENTRIES 256
static struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idt_ptr;

/* Assembly function to load IDT (defined in kernel_entry.asm) */
extern void idt_flush(void);

/*
 * idt_set_entry - Set up a single IDT entry
 *
 * @param index: Interrupt vector number (0-255)
 * @param base: Address of interrupt handler
 * @param selector: Code segment selector (0x08 for kernel)
 * @param flags: Type and attribute flags
 */
static void idt_set_entry(uint8_t index, uint32_t base, uint16_t selector,
                          uint8_t flags) {
  idt[index].base_low = base & 0xFFFF;
  idt[index].base_high = (base >> 16) & 0xFFFF;
  idt[index].selector = selector;
  idt[index].zero = 0;
  idt[index].flags = flags;
}

/* =============================================================================
 * SECTION 3: Exception and IRQ Handler Declarations
 * =============================================================================
 * These are defined in kernel_entry.asm
 */

/* CPU Exception handlers (ISR 0-31) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* Hardware IRQ handlers (IRQ 0-15, mapped to vectors 32-47) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* =============================================================================
 * SECTION 4: PIC (Programmable Interrupt Controller) Remapping
 * =============================================================================
 * The 8259 PIC maps IRQ 0-7 to interrupts 8-15 by default.
 * This conflicts with CPU exceptions, so we remap IRQs to 32-47.
 */

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

/*
 * pic_remap - Remap PIC to avoid conflicts with CPU exceptions
 *
 * Remaps IRQ 0-7 to vectors 32-39
 * Remaps IRQ 8-15 to vectors 40-47
 */
static void pic_remap(void) {
  uint8_t mask1, mask2;

  /* Save current masks */
  mask1 = inb(PIC1_DATA);
  mask2 = inb(PIC2_DATA);

  /* Start initialization sequence (ICW1) */
  outb(PIC1_CMD, 0x11);
  io_wait();
  outb(PIC2_CMD, 0x11);
  io_wait();

  /* Set vector offsets (ICW2) */
  outb(PIC1_DATA, 0x20); /* Master PIC: vectors 32-39 */
  io_wait();
  outb(PIC2_DATA, 0x28); /* Slave PIC: vectors 40-47 */
  io_wait();

  /* Configure cascading (ICW3) */
  outb(PIC1_DATA, 0x04); /* Slave on IRQ2 */
  io_wait();
  outb(PIC2_DATA, 0x02); /* Slave ID 2 */
  io_wait();

  /* Set 8086 mode (ICW4) */
  outb(PIC1_DATA, 0x01);
  io_wait();
  outb(PIC2_DATA, 0x01);
  io_wait();

  /* Restore masks (enable all IRQs for now) */
  outb(PIC1_DATA, 0x00); /* Enable all IRQs on master */
  outb(PIC2_DATA, 0x00); /* Enable all IRQs on slave */

  debug_print("[PIC] Remapped IRQs to vectors 32-47\n");
}

/*
 * pic_send_eoi - Send End of Interrupt signal to PIC
 *
 * @param irq: IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_CMD, 0x20); /* Send EOI to slave PIC */
  }
  outb(PIC1_CMD, 0x20); /* Send EOI to master PIC */
}

/* =============================================================================
 * SECTION 5: IDT Initialization
 * =============================================================================
 */

void idt_init(void) {
  debug_print("[IDT] Initializing Interrupt Descriptor Table\n");

  idt_ptr.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
  idt_ptr.base = (uint32_t)&idt;

  /* Clear all entries */
  memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

  /* Remap the PIC before setting up IRQ handlers */
  pic_remap();

  /* Set up exception handlers (ISR 0-31) */
  /* Flags 0x8E = Present, Ring 0, 32-bit Interrupt Gate */
  idt_set_entry(0, (uint32_t)isr0, 0x08, 0x8E);
  idt_set_entry(1, (uint32_t)isr1, 0x08, 0x8E);
  idt_set_entry(2, (uint32_t)isr2, 0x08, 0x8E);
  idt_set_entry(3, (uint32_t)isr3, 0x08, 0x8E);
  idt_set_entry(4, (uint32_t)isr4, 0x08, 0x8E);
  idt_set_entry(5, (uint32_t)isr5, 0x08, 0x8E);
  idt_set_entry(6, (uint32_t)isr6, 0x08, 0x8E);
  idt_set_entry(7, (uint32_t)isr7, 0x08, 0x8E);
  idt_set_entry(8, (uint32_t)isr8, 0x08, 0x8E);
  idt_set_entry(9, (uint32_t)isr9, 0x08, 0x8E);
  idt_set_entry(10, (uint32_t)isr10, 0x08, 0x8E);
  idt_set_entry(11, (uint32_t)isr11, 0x08, 0x8E);
  idt_set_entry(12, (uint32_t)isr12, 0x08, 0x8E);
  idt_set_entry(13, (uint32_t)isr13, 0x08, 0x8E);
  idt_set_entry(14, (uint32_t)isr14, 0x08, 0x8E);
  idt_set_entry(15, (uint32_t)isr15, 0x08, 0x8E);
  idt_set_entry(16, (uint32_t)isr16, 0x08, 0x8E);
  idt_set_entry(17, (uint32_t)isr17, 0x08, 0x8E);
  idt_set_entry(18, (uint32_t)isr18, 0x08, 0x8E);
  idt_set_entry(19, (uint32_t)isr19, 0x08, 0x8E);
  idt_set_entry(20, (uint32_t)isr20, 0x08, 0x8E);
  idt_set_entry(21, (uint32_t)isr21, 0x08, 0x8E);
  idt_set_entry(22, (uint32_t)isr22, 0x08, 0x8E);
  idt_set_entry(23, (uint32_t)isr23, 0x08, 0x8E);
  idt_set_entry(24, (uint32_t)isr24, 0x08, 0x8E);
  idt_set_entry(25, (uint32_t)isr25, 0x08, 0x8E);
  idt_set_entry(26, (uint32_t)isr26, 0x08, 0x8E);
  idt_set_entry(27, (uint32_t)isr27, 0x08, 0x8E);
  idt_set_entry(28, (uint32_t)isr28, 0x08, 0x8E);
  idt_set_entry(29, (uint32_t)isr29, 0x08, 0x8E);
  idt_set_entry(30, (uint32_t)isr30, 0x08, 0x8E);
  idt_set_entry(31, (uint32_t)isr31, 0x08, 0x8E);

  /* Set up IRQ handlers (vectors 32-47) */
  idt_set_entry(32, (uint32_t)irq0, 0x08, 0x8E);
  idt_set_entry(33, (uint32_t)irq1, 0x08, 0x8E);
  idt_set_entry(34, (uint32_t)irq2, 0x08, 0x8E);
  idt_set_entry(35, (uint32_t)irq3, 0x08, 0x8E);
  idt_set_entry(36, (uint32_t)irq4, 0x08, 0x8E);
  idt_set_entry(37, (uint32_t)irq5, 0x08, 0x8E);
  idt_set_entry(38, (uint32_t)irq6, 0x08, 0x8E);
  idt_set_entry(39, (uint32_t)irq7, 0x08, 0x8E);
  idt_set_entry(40, (uint32_t)irq8, 0x08, 0x8E);
  idt_set_entry(41, (uint32_t)irq9, 0x08, 0x8E);
  idt_set_entry(42, (uint32_t)irq10, 0x08, 0x8E);
  idt_set_entry(43, (uint32_t)irq11, 0x08, 0x8E);
  idt_set_entry(44, (uint32_t)irq12, 0x08, 0x8E);
  idt_set_entry(45, (uint32_t)irq13, 0x08, 0x8E);
  idt_set_entry(46, (uint32_t)irq14, 0x08, 0x8E);
  idt_set_entry(47, (uint32_t)irq15, 0x08, 0x8E);

  /* Load the IDT */
  idt_flush();

  debug_print("[IDT] IDT initialized successfully\n");
}

/* =============================================================================
 * SECTION 6: Interrupt Handlers (Called from Assembly)
 * =============================================================================
 */

/* Exception names for debugging */
static const char *exception_names[] = {"Division By Zero",
                                        "Debug",
                                        "Non Maskable Interrupt",
                                        "Breakpoint",
                                        "Overflow",
                                        "Bound Range Exceeded",
                                        "Invalid Opcode",
                                        "Device Not Available",
                                        "Double Fault",
                                        "Coprocessor Segment Overrun",
                                        "Invalid TSS",
                                        "Segment Not Present",
                                        "Stack Fault",
                                        "General Protection Fault",
                                        "Page Fault",
                                        "Reserved",
                                        "x87 FPU Error",
                                        "Alignment Check",
                                        "Machine Check",
                                        "SIMD FPU Exception",
                                        "Virtualization Exception",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Reserved",
                                        "Security Exception",
                                        "Reserved"};

/*
 * Registers pushed by interrupt stub
 * This mirrors the stack layout after pusha and segment pushes
 */
typedef struct {
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

/*
 * isr_handler - Handle CPU exceptions
 * Called from assembly ISR stubs
 */
void isr_handler(registers_t *regs) {
  debug_print("[ISR] Exception: ");
  if (regs->int_no < 32) {
    debug_print(exception_names[regs->int_no]);
  }
  debug_print(" (#");
  debug_hex(regs->int_no);
  debug_print(")\n");
  debug_print("  Error Code: ");
  debug_hex(regs->err_code);
  debug_print("\n  EIP: ");
  debug_hex(regs->eip);
  debug_print("\n");

  /* For now, halt on exception */
  debug_print("[ISR] System halted due to exception\n");
  cli();
  for (;;) {
    hlt();
  }
}

/* Forward declarations for device handlers */
extern void timer_handler(void);
extern void keyboard_handler(void);
extern void mouse_handler(void);

/*
 * irq_handler - Handle hardware interrupts
 * Called from assembly IRQ stubs
 */
void irq_handler(registers_t *regs) {
  uint8_t irq = regs->int_no - 32;

  /* Dispatch to appropriate handler */
  switch (irq) {
  case 0: /* Timer */
    timer_handler();
    break;
  case 1: /* Keyboard */
    keyboard_handler();
    break;
  case 12: /* Mouse */
    mouse_handler();
    break;
  default:
    /* Unhandled IRQ */
    break;
  }

  /* Send EOI to PIC */
  pic_send_eoi(irq);
}

/* =============================================================================
 * SECTION 7: Main Kernel Entry Point
 * =============================================================================
 */

/* Forward declarations */
extern void memory_init(void);
extern void timer_init(uint32_t freq);
extern void keyboard_init(void);
extern void mouse_init(void);
extern void vga_init(void);
extern void window_init(void);
extern void taskbar_init(void);
extern void pong_init(void);
extern void kernel_main_loop(void);

/*
 * kmain - Kernel main function
 * This is called from the assembly entry point after basic setup.
 */
void kmain(void) {
  /* Disable interrupts during initialization */
  cli();

  debug_print("\n");
  debug_print("===========================================\n");
  debug_print("   FoxOS v0.1 - A Simple Graphical OS\n");
  debug_print("===========================================\n\n");

  /* Initialize core kernel components (The Brain) */
  debug_print("[BOOT] Initializing GDT...\n");
  gdt_init();

  debug_print("[BOOT] Initializing IDT...\n");
  idt_init();

  debug_print("[BOOT] Initializing Memory Manager...\n");
  memory_init();

  /* Initialize drivers */
  debug_print("[BOOT] Initializing Timer (100 Hz)...\n");
  timer_init(100);

  debug_print("[BOOT] Initializing Keyboard...\n");
  keyboard_init();

  debug_print("[BOOT] Initializing Mouse...\n");
  mouse_init();

  /* Initialize graphics */
  debug_print("[BOOT] Initializing VGA Graphics...\n");
  vga_init();

  /* Initialize GUI components */
  debug_print("[BOOT] Initializing Window Manager...\n");
  window_init();

  debug_print("[BOOT] Initializing Taskbar...\n");
  taskbar_init();

  debug_print("[BOOT] Initializing Pong Game...\n");
  pong_init();

  /* Enable interrupts */
  debug_print("[BOOT] Enabling interrupts...\n");
  sti();

  debug_print("[BOOT] FoxOS initialization complete!\n");
  debug_print("[BOOT] Entering main loop...\n\n");

  /* Enter the main kernel loop */
  kernel_main_loop();

  /* Should never reach here */
  debug_print("[FATAL] Kernel main loop exited!\n");
  cli();
  for (;;) {
    hlt();
  }
}
