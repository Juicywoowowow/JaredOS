/**
 * jaredOS - Main Include Header
 */

#ifndef JAREDOS_H
#define JAREDOS_H

#include "kernel/types.h"

/* Version info */
#define JAREDOS_VERSION_MAJOR 0
#define JAREDOS_VERSION_MINOR 1
#define JAREDOS_VERSION_PATCH 0
#define JAREDOS_NAME "jaredOS"

/* Include all headers */
#include "kernel/core/gdt.h"
#include "kernel/core/idt.h"
#include "kernel/core/isr.h"
#include "kernel/core/irq.h"
#include "kernel/memory/pmm.h"
#include "kernel/memory/vmm.h"
#include "kernel/memory/heap.h"
#include "kernel/drivers/vga.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/serial.h"
#include "kernel/lib/string.h"
#include "kernel/lib/stdlib.h"
#include "kernel/lib/printf.h"
#include "kernel/shell/shell.h"

#endif /* JAREDOS_H */
