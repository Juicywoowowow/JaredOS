/*
 * =============================================================================
 * SharkOS Memory Utilities (memory.c)
 * =============================================================================
 * Implementation of basic memory manipulation functions.
 *
 * DEBUGGING TIPS:
 *   - If memcpy causes crashes, check for buffer overflows
 *   - Null pointer access will cause triple fault (reboot)
 *   - Use memset to zero-initialize structures before use
 * =============================================================================
 */

#include "memory.h"

/*
 * memset - Fill memory with a constant byte
 * 
 * Implementation notes:
 *   - We cast val to unsigned char to ensure consistent behavior
 *   - Could be optimized with rep stosb in assembly, but this is simpler
 */
void* memset(void* dest, int val, size_t count) {
    unsigned char* ptr = (unsigned char*)dest;
    unsigned char byte = (unsigned char)val;
    
    /* Simple byte-by-byte fill */
    while (count--) {
        *ptr++ = byte;
    }
    
    return dest;
}

/*
 * memcpy - Copy memory from source to destination
 *
 * WARNING: This does NOT handle overlapping regions!
 * If src and dest overlap and src < dest, data corruption will occur.
 * For overlapping copies, implement memmove (copy backwards when needed).
 */
void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    /* Simple byte-by-byte copy */
    while (count--) {
        *d++ = *s++;
    }
    
    return dest;
}

/*
 * memcmp - Compare two memory regions
 *
 * Returns:
 *   0 if the memory regions are identical
 *   <0 if first differing byte in s1 is less than in s2
 *   >0 if first differing byte in s1 is greater than in s2
 */
int memcmp(const void* s1, const void* s2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    
    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}
