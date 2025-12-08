/*
 * =============================================================================
 * kernelB.c - Core Kernel: Memory Management
 * =============================================================================
 *
 * This is the "brain" of FoxOS - Part B. It handles:
 *   1. Physical memory detection and tracking
 *   2. Simple bitmap-based page frame allocator
 *   3. Heap memory allocator (kmalloc/kfree)
 *
 * Memory Layout:
 *   0x00000000 - 0x000FFFFF : First 1MB (BIOS, VGA, Kernel)
 *   0x00100000 - heap_start : Extended kernel code/data
 *   heap_start - heap_end   : Kernel heap (dynamic allocation)
 *
 * DEBUGGING TIPS:
 *   - If kmalloc returns NULL, you're out of heap space
 *   - Memory corruption often shows as random crashes later
 *   - Use debug_print to trace allocation/free patterns
 *   - Double-free bugs can corrupt the free list
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: Physical Memory Constants
 * =============================================================================
 */

/* We assume 32MB of RAM for simplicity */
#define MEMORY_SIZE (32 * 1024 * 1024) /* 32 MB */
#define PAGE_SIZE 4096                 /* 4 KB pages */
#define NUM_PAGES (MEMORY_SIZE / PAGE_SIZE)

/* Heap configuration */
#define HEAP_START 0x00200000        /* 2 MB mark */
#define HEAP_SIZE (16 * 1024 * 1024) /* 16 MB heap */
#define HEAP_END (HEAP_START + HEAP_SIZE)

/* Bitmap to track page allocation (1 bit per page) */
static uint8_t page_bitmap[NUM_PAGES / 8];

/* =============================================================================
 * SECTION 2: Physical Page Frame Allocator
 * =============================================================================
 *
 * We use a simple bitmap allocator. Each bit represents one 4KB page.
 * Bit = 0 means page is free, Bit = 1 means page is allocated.
 *
 * This isn't the most efficient allocator, but it's simple and works.
 */

/*
 * page_frame_alloc - Allocate a single physical page
 * ugh, maybe we can allocate more pages but this can suffice.
 * Returns: Physical address of allocated page, or NULL if out of memory
 *
 * DEBUGGING TIP: If this returns NULL, check if you're leaking pages
 * somewhere. Add debug output to track allocation patterns.
 */
void *page_frame_alloc(void) {
  for (uint32_t i = 0; i < NUM_PAGES / 8; i++) {
    if (page_bitmap[i] != 0xFF) {
      /* Found a byte with a free bit */
      for (uint8_t j = 0; j < 8; j++) {
        if (!(page_bitmap[i] & (1 << j))) {
          /* Mark page as used */
          page_bitmap[i] |= (1 << j);
          uint32_t page_num = i * 8 + j;
          void *addr = (void *)(page_num * PAGE_SIZE);

          /* Zero the page before returning */
          memset(addr, 0, PAGE_SIZE);

          debug_print("[MEM] Allocated page at ");
          debug_hex((uint32_t)addr);
          debug_print("\n");

          return addr;
        }
      }
    }
  }

  debug_print("[MEM] ERROR: Out of physical pages!\n");
  return NULL;
}

/*
 * page_frame_free - Free a previously allocated page
 *
 * @param addr: Physical address of page to free (must be page-aligned)
 *
 * DEBUGGING TIP: Double-free will corrupt the bitmap. Consider adding
 * a check that the bit is actually set before clearing it.
 */
void page_frame_free(void *addr) {
  uint32_t page_num = (uint32_t)addr / PAGE_SIZE;

  if (page_num >= NUM_PAGES) {
    debug_print("[MEM] ERROR: Tried to free invalid page!\n");
    return;
  }

  uint32_t byte_idx = page_num / 8;
  uint8_t bit_idx = page_num % 8;

  /* Check for double-free */
  if (!(page_bitmap[byte_idx] & (1 << bit_idx))) {
    debug_print("[MEM] WARNING: Double-free detected at ");
    debug_hex((uint32_t)addr);
    debug_print("\n");
    return;
  }

  /* Mark page as free */
  page_bitmap[byte_idx] &= ~(1 << bit_idx);

  debug_print("[MEM] Freed page at ");
  debug_hex((uint32_t)addr);
  debug_print("\n");
}

/* =============================================================================
 * SECTION 3: Kernel Heap Allocator (kmalloc/kfree)
 * =============================================================================
 *
 * This is a simple first-fit allocator with a linked free list.
 * Each block has a header containing size and allocation status.
 *
 * Block Layout:
 *   +-------------------+
 *   | Header (8 bytes)  |  <- size, used flag, next pointer
 *   +-------------------+
 *   | User Data         |  <- returned by kmalloc
 *   +-------------------+
 */

/* Block header structure */
typedef struct block_header {
  uint32_t size;             /* Size of user data (not including header) */
  uint32_t used;             /* 1 if allocated, 0 if free */
  struct block_header *next; /* Next block in list */
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)
#define MIN_BLOCK_SIZE 16 /* Minimum allocation size */

/* Head of the free list */
static block_header_t *heap_start_ptr = NULL;

/*
 * heap_init - Initialize the kernel heap
 *
 * Creates a single large free block spanning the entire heap.
 */
static void heap_init(void) {
  heap_start_ptr = (block_header_t *)HEAP_START;
  heap_start_ptr->size = HEAP_SIZE - HEADER_SIZE;
  heap_start_ptr->used = 0;
  heap_start_ptr->next = NULL;

  debug_print("[HEAP] Initialized heap at ");
  debug_hex(HEAP_START);
  debug_print(", size ");
  debug_hex(HEAP_SIZE);
  debug_print("\n");
}

/*
 * kmalloc - Allocate memory from the kernel heap
 *
 * @param size: Number of bytes to allocate
 * Returns: Pointer to allocated memory, or NULL if out of memory
 *
 * Uses first-fit algorithm: finds the first free block that's big enough.
 */
void *kmalloc(uint32_t size) {
  if (size == 0) {
    return NULL;
  }

  /* Align size to 4 bytes */
  size = ALIGN_UP(size, 4);
  if (size < MIN_BLOCK_SIZE) {
    size = MIN_BLOCK_SIZE;
  }

  block_header_t *current = heap_start_ptr;

  while (current != NULL) {
    if (!current->used && current->size >= size) {
      /* Found a suitable block */

      /* Split block if it's significantly larger than needed */
      if (current->size >= size + HEADER_SIZE + MIN_BLOCK_SIZE) {
        /* Create new free block after allocated region */
        block_header_t *new_block =
            (block_header_t *)((uint8_t *)current + HEADER_SIZE + size);
        new_block->size = current->size - size - HEADER_SIZE;
        new_block->used = 0;
        new_block->next = current->next;

        current->size = size;
        current->next = new_block;
      }

      current->used = 1;

      void *ptr = (void *)((uint8_t *)current + HEADER_SIZE);

      /* Zero the allocated memory */
      memset(ptr, 0, size);

      debug_print("[HEAP] Allocated ");
      debug_hex(size);
      debug_print(" bytes at ");
      debug_hex((uint32_t)ptr);
      debug_print("\n");

      return ptr;
    }

    current = current->next;
  }

  debug_print("[HEAP] ERROR: Out of heap memory! Requested ");
  debug_hex(size);
  debug_print(" bytes\n");

  return NULL;
}

/*
 * kfree - Free memory allocated by kmalloc
 *
 * @param ptr: Pointer to free (must have been returned by kmalloc)
 *
 * DEBUGGING TIP: Passing a pointer not from kmalloc will corrupt the heap.
 */
void kfree(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  /* Get the block header */
  block_header_t *block = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);

  /* Sanity check: is this within the heap? */
  if ((uint32_t)block < HEAP_START || (uint32_t)block >= HEAP_END) {
    debug_print("[HEAP] ERROR: Tried to free pointer outside heap!\n");
    return;
  }

  if (!block->used) {
    debug_print("[HEAP] WARNING: Double-free detected at ");
    debug_hex((uint32_t)ptr);
    debug_print("\n");
    return;
  }

  /* Mark as free */
  block->used = 0;

  debug_print("[HEAP] Freed memory at ");
  debug_hex((uint32_t)ptr);
  debug_print("\n");

  /* Coalesce with next block if it's free */
  if (block->next != NULL && !block->next->used) {
    block->size += HEADER_SIZE + block->next->size;
    block->next = block->next->next;
    debug_print("[HEAP] Coalesced with next block\n");
  }

  /* Coalesce with previous block if possible */
  /* (Would need to maintain prev pointers for efficient implementation) */
}

/*
 * krealloc - Resize an allocated block
 *
 * @param ptr: Pointer to existing allocation (or NULL for new allocation)
 * @param size: New size in bytes
 * Returns: Pointer to resized block (may be different from input)
 */
void *krealloc(void *ptr, uint32_t size) {
  if (ptr == NULL) {
    return kmalloc(size);
  }

  if (size == 0) {
    kfree(ptr);
    return NULL;
  }

  block_header_t *block = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);

  /* If current block is big enough, just return it */
  if (block->size >= size) {
    return ptr;
  }

  /* Need to allocate new block and copy */
  void *new_ptr = kmalloc(size);
  if (new_ptr != NULL) {
    memcpy(new_ptr, ptr, block->size);
    kfree(ptr);
  }

  return new_ptr;
}

/* =============================================================================
 * SECTION 4: Memory Initialization
 * =============================================================================
 */

/*
 * memory_init - Initialize all memory subsystems
 *
 * Called early in kernel boot to set up physical and heap allocators.
 */
void memory_init(void) {
  debug_print("[MEM] Initializing memory management...\n");

  /* Clear the page bitmap - all pages start as free */
  memset(page_bitmap, 0, sizeof(page_bitmap));

  /* Mark first 2MB as used (kernel + BIOS + VGA) */
  /* This is a conservative estimate */
  for (uint32_t i = 0; i < (HEAP_START / PAGE_SIZE) / 8; i++) {
    page_bitmap[i] = 0xFF;
  }

  debug_print("[MEM] Reserved first 2MB for kernel\n");

  /* Initialize the heap */
  heap_init();

  debug_print("[MEM] Memory initialization complete\n");
  debug_print("[MEM] Total RAM: ");
  debug_hex(MEMORY_SIZE);
  debug_print(" bytes\n");
  debug_print("[MEM] Heap available: ");
  debug_hex(HEAP_SIZE);
  debug_print(" bytes\n");
}

/* =============================================================================
 * SECTION 5: Memory Statistics (for debugging)
 * =============================================================================
 */

/*
 * memory_get_free_pages - Count free physical pages
 *
 * Returns: Number of free 4KB pages
 */
uint32_t memory_get_free_pages(void) {
  uint32_t free_count = 0;

  for (uint32_t i = 0; i < NUM_PAGES / 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      if (!(page_bitmap[i] & (1 << j))) {
        free_count++;
      }
    }
  }

  return free_count;
}

/*
 * memory_get_used_heap - Calculate used heap memory
 *
 * Returns: Bytes currently allocated from heap
 */
uint32_t memory_get_used_heap(void) {
  uint32_t used = 0;
  block_header_t *current = heap_start_ptr;

  while (current != NULL) {
    if (current->used) {
      used += current->size;
    }
    current = current->next;
  }

  return used;
}

/*
 * memory_dump_stats - Print memory statistics to debug console
 */
void memory_dump_stats(void) {
  uint32_t free_pages = memory_get_free_pages();
  uint32_t used_heap = memory_get_used_heap();

  debug_print("\n=== Memory Statistics ===\n");
  debug_print("Free pages: ");
  debug_hex(free_pages);
  debug_print(" (");
  debug_hex(free_pages * PAGE_SIZE);
  debug_print(" bytes)\n");
  debug_print("Heap used: ");
  debug_hex(used_heap);
  debug_print(" / ");
  debug_hex(HEAP_SIZE);
  debug_print(" bytes\n");
  debug_print("=========================\n\n");
}
