/* Clang lowers large aggregate initialization to memset even when compiling
 * freestanding MIPS code. The ROM has no standard memset symbol. Keep this
 * implementation local to the patch ELF rather than importing a nonexistent
 * game function. Volatile stores prevent optimization back into memset.
 */
#if defined(__clang__)
void* memset(void* destination, int value, unsigned int length) {
    volatile unsigned char* bytes = (volatile unsigned char*)destination;
    while (length--) {
        *bytes++ = (unsigned char)value;
    }
    return destination;
}
#endif
