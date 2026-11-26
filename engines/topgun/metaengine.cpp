/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/translation.h"

#include "topgun/metaengine.h"
#include "topgun/detection.h"
#include "topgun/topgun_v1_0.h"
#include "topgun/topgun_v1_1.h"
#include "topgun/topgun_v1_2.h"
#include "topgun/topgun_v1_2_3.h"
#include "topgun/tunes_player.h"

namespace Topgun {

static const ADExtraGuiOptionsMap optionsList[] = {
	{
		GAMEOPTION_ORIGINAL_SAVELOAD,
		{
			_s("Use original save/load screens"),
			_s("Use the original save/load screens instead of the ScummVM ones"),
			"original_menus",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_SCREEN_800X600,
		{
			_s("Run the game at 800x600"),
			_s("Use a screen of 800x600 pixels instead of 640x480"),
			"screen_800x600",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_HIGH_BIT_DEPTH,
		{
			_s("Run the game at a higher bit depth"),
			_s("Make the screen a non-palette device, so the game uses the box wipe transitions instead of the palette fades"),
			"high_bit_depth",
			false,
			0,
			0
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

} // End of namespace Topgun

const char *TopgunMetaEngine::getName() const {
	return "topgun";
}

const ADExtraGuiOptionsMap *TopgunMetaEngine::getAdvancedExtraGuiOptions() const {
	return Topgun::optionsList;
}

Common::Error TopgunMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	Common::String gameId = desc->gameId;

	if (gameId == "tuneland") {
		if (desc->extra && Common::String(desc->extra) == "1996")
			*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_5);
		else if (desc->flags & ADGF_DEMO)
			*engine = new Topgun::TopgunEngine_v1_1(syst, desc);
		else
			*engine = new Topgun::TopgunEngine_v1_0(syst, desc);

		return Common::kNoError;
	}

	if (gameId == "pythonwaste") {
		*engine = new Topgun::TopgunEngine_v1_2(syst, desc);
		return Common::kNoError;
	}

	if (gameId == "bestshot" || gameId == "headshot" || gameId == "hotshot" || gameId == "lineshot") {
		*engine = new Topgun::TopgunEngine_v1_2(syst, desc, Topgun::kVersionV1_2_1);
		return Common::kNoError;
	}

	if (gameId == "peterwolf") {
		*engine = new Topgun::TopgunEngine_v1_2(syst, desc, Topgun::kVersionV1_2_2);
		return Common::kNoError;
	}

	if (gameId == "battlebeast") {
		*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_4);
		return Common::kNoError;
	}

	if (gameId == "arcadeamerica") {
		*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_6);
		return Common::kNoError;
	}

	if (gameId == "holygrail") {
		*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_7);
		return Common::kNoError;
	}

	if (gameId == "aceventura" && desc->language == Common::EN_ANY) {
		*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_9);
		return Common::kNoError;
	}

	if (gameId == "junglegames" && desc->language == Common::JA_JPN) {
		*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_8);
		return Common::kNoError;
	}

	if (gameId == "junglegames" || gameId == "greatword") {
		*engine = new Topgun::TopgunEngine_v1_2_3(syst, desc, Topgun::kVersionV1_2_3);
		return Common::kNoError;
	}

	if (gameId == "tunesplayer") {
		*engine = new Topgun::TunesPlayerEngine(syst, desc);
		return Common::kNoError;
	}

	return Common::kUnsupportedGameidError;
}

bool TopgunMetaEngine::hasFeature(MetaEngineFeature f) const {
	return checkExtendedSaves(f) ||
		(f == kSupportsLoadingDuringStartup);
}

#if PLUGIN_ENABLED_DYNAMIC(TOPGUN)
REGISTER_PLUGIN_DYNAMIC(TOPGUN, PLUGIN_TYPE_ENGINE, TopgunMetaEngine);
#else
REGISTER_PLUGIN_STATIC(TOPGUN, PLUGIN_TYPE_ENGINE, TopgunMetaEngine);
#endif
