/*
 * =============================================================================
 * SharkOS Kernel Header (kernel.h)
 * =============================================================================
 * Kernel initialization and core functions.
 * =============================================================================
 */

#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * Kernel Panic
 * 
 * Called when an unrecoverable error occurs. Prints message and halts.
 * Use sparingly - only for truly fatal errors.
 * ---------------------------------------------------------------------------- */
void kernel_panic(const char* message);

#endif /* KERNEL_H */
