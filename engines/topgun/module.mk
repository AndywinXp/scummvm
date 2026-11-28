MODULE := engines/topgun

MODULE_OBJS = \
	background.o \
	console.o \
	cursor.o \
	display.o \
	events.o \
	expression.o \
	games/ace/ace1util.o \
	games/ace/dynalloc.o \
	games/ace/fileio.o \
	games/ace/pathfind.o \
	games/battle/battle.o \
	games/battle/battle_ai.o \
	games/battle/battle_geometry.o \
	games/battle/battle_hit.o \
	games/battle/battle_hooks.o \
	games/battle/battle_input.o \
	games/battle/battle_moves.o \
	games/battle/battle_scene.o \
	games/battle/battle_shared.o \
	games/battle/battle_special.o \
	games/battle/battle_stats.o \
	games/battle/battle_type0.o \
	games/battle/battle_type1.o \
	games/battle/battle_type2.o \
	games/battle/battle_type3.o \
	games/battle/battle_type4.o \
	games/battle/battle_type5.o \
	games/battle/battle_type6.o \
	games/battle/battle_weapons.o \
	games/battledemo/battledemo.o \
	games/battledemo/battledemo_ai.o \
	games/battledemo/battledemo_hit.o \
	games/battledemo/battledemo_input.o \
	games/battledemo/battledemo_moves.o \
	games/battledemo/battledemo_scene.o \
	games/battledemo/battledemo_shared.o \
	games/battledemo/battledemo_type0.o \
	games/battledemo/battledemo_type1.o \
	games/battledemo/battledemo_weapons.o \
	games/virgil/re.o \
	games/virgil/refengine.o \
	games/virgil/wordlist.o \
	ini.o \
	input.o \
	libraries/audio/audio.o \
	libraries/audio/audio_v1_2.o \
	libraries/audio/cd.o \
	libraries/audio/cd_v1_2.o \
	libraries/audio/convert.o \
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
	libraries/sprite/message.o \
	libraries/sprite/palette.o \
	libraries/sprite/render.o \
	libraries/sprite/render_v1_2.o \
	libraries/sprite/sprite.o \
	libraries/sprite/sprite_v1_2.o \
	libraries/utils/utils.o \
	metaengine.o \
	movie.o \
	opcode.o \
	opcode_v1_2.o \
	opcode_v1_2_4.o \
	opcode_v2_0.o \
	plugin.o \
	res_loader.o \
	scene.o \
	script.o \
	script_v1_1.o \
	script_v1_2.o \
	script_v1_2_4.o \
	script_v2_0.o \
	text.o \
	topgun.o \
	topgun_v1_0.o \
	topgun_v1_1.o \
	topgun_v1_2.o \
	topgun_v1_2_4.o \
	topgun_v2_0.o \
	transfer.o \
	tunes_player.o

# This module can be built as a plugin
ifeq ($(ENABLE_TOPGUN), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
