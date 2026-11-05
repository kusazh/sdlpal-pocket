/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The offline converter needs only these host facilities, not SDL/common.h. */
#ifndef PAL_RIX_COMPAT_H
#define PAL_RIX_COMPAT_H
#define _COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
#define alloca _alloca
#define SDL_strcasecmp _stricmp
#else
#include <alloca.h>
#include <strings.h>
#define SDL_strcasecmp strcasecmp
#endif
#endif
