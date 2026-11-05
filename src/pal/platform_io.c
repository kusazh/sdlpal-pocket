/* SPDX-License-Identifier: GPL-3.0-or-later */
#undef fopen
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#ifndef OF_PC
#include "of_file.h"
#endif

/* PALPAK1: magic[8], count:u32le, entries {name[24], offset:u32le,
 * size:u32le}, then original file bytes. Streams have independent cursors.
 * This prototype keeps compressed assets resident until process exit. */
static uint8_t *archive;
static uint32_t count;
static uint32_t le32(const uint8_t *p) {
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int load_archive(void) {
    if (archive) return 0;
#ifndef OF_PC
    of_file_slot_register(4, "pal.pak");
#endif
    FILE *f = fopen("pal.pak", "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return -1; }
    long size = ftell(f);
    if (size < 12 || size > 32 * 1024 * 1024 || fseek(f, 0, SEEK_SET)) {
        fclose(f); errno = EINVAL; return -1;
    }
    uint8_t *data = malloc((size_t)size);
    if (!data) { fclose(f); return -1; }
    int ok = fread(data, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    uint32_t n = ok ? le32(data + 8) : 0;
    if (!ok || memcmp(data, "PALPAK1\0", 8) || n > 128 || 12 + n * 32 > (size_t)size) {
        free(data); errno = EINVAL; return -1;
    }
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *e = data + 12 + i * 32;
        uint32_t off = le32(e + 24), len = le32(e + 28);
        if (!memchr(e, 0, 24) || off < 12 + n * 32 || off > (size_t)size || len > (size_t)size - off) {
            free(data); errno = EINVAL; return -1;
        }
    }
    archive = data; count = n;
    printf("PAL: loaded %u resources (%ld bytes)\n", count, size);
    return 0;
}
FILE *PAL_fopen(const char *path, const char *mode) {
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    /* User slots 1-5 use APF slots 10-14, never the original 0.RPG. */
    if (strlen(name) == 5 && name[0] >= '1' && name[0] <= '5' && !strcasecmp(name + 1, ".rpg")) {
        char save[24];
        snprintf(save, sizeof save, "pal-%c.sav", name[0]);
#ifndef OF_PC
        of_file_slot_register(10 + name[0] - '1', save);
#endif
        return fopen(save, mode);
    }
    if (strcmp(mode, "r") && strcmp(mode, "rb")) { errno = EROFS; return NULL; }
    if (load_archive()) return NULL;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t *e = archive + 12 + i * 32;
        if (!strcasecmp(name, (const char *)e))
            return fmemopen(archive + le32(e + 24), le32(e + 28), "rb");
    }
    errno = ENOENT;
    return NULL;
}
