/* DOS resources use RNG animations; Windows AVI playback is not built. */
#include "aviplay.h"
void PAL_AVIInit(void) {}
void PAL_AVIShutdown(void) {}
BOOL PAL_PlayAVI(const char *path) { (void)path; return FALSE; }
void AVI_FillAudioBuffer(void *u, uint8_t *s, int n) { (void)u; (void)s; (void)n; }
void *AVI_GetPlayState(void) { return NULL; }
