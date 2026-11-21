MODULE := engines/topgun

MODULE_OBJS = \
	background.o \
	console.o \
	display.o \
	input.o \
	libraries/audio/audio.o \
	libraries/audio/cd.o \
	libraries/audio/convert.o \
	libraries/audio/midi.o \
	libraries/audio/mixer.o \
	libraries/audio/sound.o \
	libraries/audio/wavefile.o \
	libraries/audio/waveout.o \
	libraries/resource.o \
	libraries/sprite/animation.o \
	libraries/sprite/palette.o \
	libraries/sprite/render.o \
	libraries/sprite/sprite.o \
	libraries/utils.o \
	metaengine.o \
	opcode.o \
	res_loader.o \
	scene.o \
	script.o \
	script_v1.o \
	topgun.o \
	topgun_v0.o \
	topgun_v1.o

# This module can be built as a plugin
ifeq ($(ENABLE_TOPGUN), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
