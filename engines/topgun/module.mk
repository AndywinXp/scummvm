MODULE := engines/topgun

MODULE_OBJS = \
	background.o \
	background_v1_2.o \
	console.o \
	cursor_v1_2.o \
	display.o \
	events_v1_2.o \
	ini_v1_2.o \
	input.o \
	input_v1_2.o \
	libraries/audio/audio.o \
	libraries/audio/audio_v1_2.o \
	libraries/audio/cd.o \
	libraries/audio/cd_v1_2.o \
	libraries/audio/convert.o \
	libraries/audio/convert_v1_2.o \
	libraries/audio/midi.o \
	libraries/audio/mixer.o \
	libraries/audio/mixer_v1_2.o \
	libraries/audio/sound.o \
	libraries/audio/sound_v1_2.o \
	libraries/audio/wavefile.o \
	libraries/audio/waveout.o \
	libraries/resource/resource.o \
	libraries/resource/resource_v1_2.o \
	libraries/resource/resource_v1_2_codec.o \
	libraries/sprite/animation.o \
	libraries/sprite/animation_v1_2.o \
	libraries/sprite/palette.o \
	libraries/sprite/palette_v1_2.o \
	libraries/sprite/render.o \
	libraries/sprite/render_v1_2.o \
	libraries/sprite/sprite.o \
	libraries/sprite/sprite_v1_2.o \
	libraries/utils/utils.o \
	metaengine.o \
	movie_v1_2.o \
	opcode.o \
	opcode_math_v1_2.o \
	opcode_v1_2.o \
	res_loader.o \
	res_loader_v1_2.o \
	scene.o \
	scene_v1_2.o \
	script.o \
	script_v1_1.o \
	script_v1_2.o \
	text_v1_2.o \
	topgun.o \
	topgun_v1_0.o \
	topgun_v1_1.o \
	topgun_v1_2.o \
	transfer_v1_2.o \
	tunes_player.o

# This module can be built as a plugin
ifeq ($(ENABLE_TOPGUN), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
