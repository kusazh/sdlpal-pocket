APP = pal
SDK_DIR = ../sdk
ROOT = $(realpath ../..)
OBJ_DIR = $(ROOT)/.obj/pal
BUILD_DIR = $(OBJ_DIR)
EXCLUDED = audio aviplay glslp midi midi_timidity midi_tsf mini_glloader mp3play oggplay opusplay video_glsl
SRCS = $(filter-out $(addprefix engine/,$(addsuffix .c,$(EXCLUDED))),$(wildcard engine/*.c)) engine/sdl_compat/sdl_compat.c platform_io.c platform_audio.c platform_avi.c platform_smoke.c
CFLAGS = -std=gnu99 -MMD -MP -fno-strict-aliasing -DPAL_POCKET -I. -Iengine -Iengine/sdl_compat -I$(SDK_DIR)/include/SDL2 -include platform_io.h
.DEFAULT_GOAL := all
SRCS += platform_video.c
include $(SDK_DIR)/sdk.mk
all: $(BUILD_DIR)/app.elf
package: all resources
	ruby package.rb
$(APP_OBJS): pal_config.h platform_io.h
GAME_DIR ?=
resources: pal.pak pal-fm.pcm
pal.pak: pack.rb sdlpal.cfg
	@test -n "$(GAME_DIR)" || { echo "Set GAME_DIR to your own DOS PAL directory"; exit 1; }
	ruby pack.rb "$(GAME_DIR)" $@
SDL2_CONFIG ?= /opt/homebrew/opt/sdl2-compat/bin/sdl2-config
PC_FLAGS = -DOF_PC -DPAL_POCKET -std=gnu99 -MMD -MP -O2 -g -fno-strict-aliasing -I. -Iengine -Iengine/sdl_compat -I$(SDK_DIR)/include -include platform_io.h $(shell $(SDL2_CONFIG) --cflags) -I$(shell $(SDL2_CONFIG) --prefix)/include
PC_SRCS = $(SRCS) $(SDK_DIR)/pc/of_sdl2.c
PC_OBJS = $(patsubst %.c,$(ROOT)/.obj/desktop/pal/%.o,$(PC_SRCS))
$(PC_OBJS): pal_config.h platform_io.h
$(ROOT)/.obj/desktop/pal/%.o: %.c
	@mkdir -p $(dir $@)
	cc $(PC_FLAGS) -c $< -o $@
desktop: $(PC_OBJS) pal.pak pal-fm.pcm
	cc $(PC_OBJS) $(shell $(SDL2_CONFIG) --libs) -lm -o pal-desktop
.PHONY: all resources desktop package
-include $(APP_OBJS:.o=.d)
-include $(PC_OBJS:.o=.d)

HOST_CXX ?= c++
RIX_SRCS = tools/render_rix.cpp $(addprefix engine/adplug/,$(addsuffix .cpp,rix player fprovide binio binfile dosbox_opls))
tools/render-rix: $(RIX_SRCS) tools/rix_compat.h
	$(HOST_CXX) -std=c++11 -O2 -Iengine -include tools/rix_compat.h $(RIX_SRCS) -o $@
pal-fm.pcm: tools/render-rix
	@test -n "$(GAME_DIR)" || { echo "Set GAME_DIR to your own DOS PAL directory"; exit 1; }
	@set -e; tmp=$@.tmp.$$$$; trap 'rm -f "$$tmp"' EXIT; ./tools/render-rix "$(GAME_DIR)/MUS.MKF" "$$tmp"; mv "$$tmp" $@

# Public release never depends on proprietary game resources.
release: all
	python3 release.py
.PHONY: release
