#ifndef DOA3_ISO_DISC_H
#define DOA3_ISO_DISC_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Duplicates a seekable descriptor; the caller can close its own descriptor.
   The verified USA disc has a flat file table. No disc bytes are copied. */
int recomp_iso_mount(int fd,const char *root,unsigned count,const char *const *names,
    const uint64_t *offsets,const uint64_t *sizes);
void recomp_iso_unmount(void);
uint64_t recomp_iso_read_bytes(void);
#ifdef __cplusplus
}
#endif
#endif
