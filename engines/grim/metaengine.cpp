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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/grim.h"
#include "engines/grim/legacy_savegames/legacy_savegames.h"

#include "common/endian.h"
#include "common/system.h"
#include "common/savefile.h"
#include "common/translation.h"
#include "common/config-manager.h"

#include "graphics/surface.h"

namespace Grim {

static const int kSaveImageWidth = 250;
static const int kSaveImageHeight = 188;
static const int kLauncherThumbnailWidth = 160;
static const int kLauncherThumbnailHeight = 120;

// Same gamma curve as SputRender::buildGammaTable applies to the save image in the game menu
static void correctSaveImageGamma(Common::Array<byte> &pixels, float gamma) {
	if (!(gamma >= 0.0f) || gamma == 1.0f)
		return;

	for (uint i = 0; i + 1 < pixels.size(); i += 2) {
		uint16 value = READ_LE_UINT16(&pixels[i]);
		float red = (float)pow((double)((value >> 11) & 0x1F) / 31.0, (double)gamma);
		float green = (float)pow((double)((value >> 5) & 0x3F) / 63.0, (double)gamma);
		float blue = (float)pow((double)(value & 0x1F) / 31.0, (double)gamma);

		value = (uint16)(((uint)(31.0 * red) << 11) | ((uint)(63.0 * green) << 5) | (uint)(31.0 * blue));
		WRITE_LE_UINT16(&pixels[i], value);
	}
}

static bool readSaveStrings(Common::SeekableReadStream *pStream, Common::Array<Common::String> &strings) {
	if (LegacySaveFile::isLegacySave(pStream))
		return legacyReadSaveStrings(pStream, strings);

	strings.clear();

	int32 totalBytes = pStream->readSint32LE();

	if (pStream->err() || pStream->eos() || totalBytes < 0 || totalBytes > pStream->size() - 4)
		return false;

	while (totalBytes > 0) {
		int32 length = pStream->readSint32LE();

		if (pStream->err() || pStream->eos() || length < 1 || length > totalBytes - 4)
			return false;

		Common::Array<char> text(length);

		if (pStream->read(text.data(), length) != (uint32)length)
			return false;

		text[length - 1] = '\0';
		strings.push_back(Common::String(text.data()));
		totalBytes -= length + 4;
	}

	return true;
}

static bool readSaveThumbnail(Common::SeekableReadStream *pStream, Common::Array<byte> &pixels) {
	if (LegacySaveFile::isLegacySave(pStream))
		return legacyReadThumbnail(pStream, pixels);

	int32 blockSize = pStream->readSint32LE();

	if (pStream->err() || pStream->eos() || blockSize < 0 || blockSize > pStream->size() - 8)
		return false;

	pStream->seek(blockSize, SEEK_CUR);

	int32 imageSize = pStream->readSint32LE();

	if (pStream->err() || pStream->eos() || imageSize != kSaveImageWidth * kSaveImageHeight * 2)
		return false;

	pixels.resize(imageSize);

	return pStream->read(pixels.data(), imageSize) == (uint32)imageSize;
}

static const ADExtraGuiOptionsMap gameGuiOptions[] = {
	{
		GAMEOPTION_LOAD_DATAUSR,
		{
			_s("Load user patch (unsupported)"),
			_s("Load an user patch. Please note that the ScummVM team doesn't provide support for using such patches."),
			"datausr_load",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_SHOW_FPS,
		{
			_s("Show FPS"),
			_s("Show the current FPS-rate, while you play."),
			"show_fps",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_3D_ACCELERATION,
		{
			_s("Enable 3D acceleration"),
			_s("Use the hardware accelerated 3D renderer. It can also be changed from the game settings."),
			"grim_3d_acceleration",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_REMASTER_BUGFIXES,
		{
			_s("Apply Remastered bug fixes"),
			_s("Apply the fixes to the game engine bugs that were corrected in the Remastered version."),
			"grim_remaster_bugfixes",
			true,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_JOYSTICK,
		{
			_s("Enable joystick/gamepad"),
			_s("Control the game with the joystick or gamepad."),
			"joystick_enabled",
			true,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_ANALOG_JOYSTICK,
		{
			_s("Analog joystick mode"),
			_s("Use the analog stick of the joystick or gamepad to move Guybrush."),
			"analog_joystick_mode",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_CAMERA_RELATIVE,
		{
			_s("Camera-relative movement"),
			_s("Move Guybrush relative to the camera instead of relative to the character."),
			"camera_relative_mode",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_MOVIE_SUBTITLES,
		{
			_s("Movie subtitles"),
			_s("Show the subtitles during the movies."),
			"movie_subtitles",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_AUDIO_EFFECTS,
		{
			_s("Audio effects"),
			_s("Apply the audio effects to the voices."),
			"audio_effects",
			true,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_SHADOW_EFFECTS,
		{
			_s("Maximal shadow effects"),
			_s("Use the maximal level of shadow effects instead of the minimal one."),
			"shadow_effects",
			true,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_MOVIE_QUALITY,
		{
			_s("Reduced movie quality"),
			_s("Play the movies at the minimal quality instead of the maximal one."),
			"movie_quality",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_EMI_MUSIC_QUALITY,
		{
			_s("Reduced music quality"),
			_s("Play the music at the minimal quality instead of the maximal one."),
			"music_quality",
			false,
			0,
			0
		}
	},

	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

class GrimMetaEngine : public AdvancedMetaEngine<Grim::GrimGameDescription> {
public:
	const char *getName() const override {
		return "grim";
	}

	const ADExtraGuiOptionsMap *getAdvancedExtraGuiOptions() const override {
		return gameGuiOptions;
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const Grim::GrimGameDescription *desc) const override;

	bool hasFeature(MetaEngineFeature f) const override;

	Common::KeymapArray initKeymaps(const char *target) const override;

	SaveStateList listSaves(const char *target) const override;

	SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const override;

	Common::String getSavegameFile(int saveGameIdx, const char *target = nullptr) const override;

};

Common::Error GrimMetaEngine::createInstance(OSystem *syst, Engine **engine, const Grim::GrimGameDescription *desc) const {
	const GrimGameDescription *gd = (const GrimGameDescription *)desc;

	if (gd->gameType == GType_MONKEY4) {
#ifdef ENABLE_MONKEY4
		if (gd->desc.flags & ADGF_DEMO)
			return Common::Error(Common::kUnsupportedGameidError, _s("The demo of Escape from Monkey Island is not supported by this version of the Grim engine"));

		*engine = new EmiEngine(syst, gd->desc.flags, gd->gameType, gd->desc.platform, gd->desc.language);

		return Common::kNoError;
#else
		return Common::Error(Common::kUnsupportedGameidError, _s("Escape from Monkey Island support is not compiled in"));
#endif
	}

	if (gd->desc.flags & ADGF_REMASTERED)
		return Common::Error(Common::kUnsupportedGameidError, _s("Grim Fandango Remastered is not supported by this version of the Grim engine"));

	*engine = new GrimEngine(syst, gd->desc.flags, gd->gameType, gd->desc.platform, gd->desc.language);

	return Common::kNoError;
}

bool GrimMetaEngine::hasFeature(MetaEngineFeature f) const {
	return
		(f == kSupportsListSaves) ||
		(f == kSupportsDeleteSave) ||
		(f == kSavesSupportMetaInfo) ||
		(f == kSavesSupportThumbnail) ||
		(f == kSupportsLoadingDuringStartup);
}

Common::String GrimMetaEngine::getSavegameFile(int saveGameIdx, const char *target) const {
	Common::String gameId = target ? ConfMan.get("gameid", target) : Common::String();
	bool isEmi = gameId == "monkey4";
	bool isPs2 = target && Common::parsePlatform(ConfMan.get("platform", target)) == Common::kPlatformPS2;
	const char *extension = isPs2 ? "ps2" : "gsv";

	if (saveGameIdx == kSavegameFilePattern)
		return isEmi ? Common::String::format("efmi###.%s", extension) : Common::String("grim##.gsv");

	if (isEmi)
		return Common::String::format("efmi%03d.%s", saveGameIdx, extension);

	return Common::String::format("grim%02d.gsv", saveGameIdx);
}

Common::KeymapArray GrimMetaEngine::initKeymaps(const char *target) const {
	Common::String gameId = ConfMan.get("gameid", target);

#ifdef ENABLE_GRIM
	if (gameId == "grim") {
		return Grim::GrimEngine::initKeymapsGrim(target);
	}
#endif
#ifdef ENABLE_MONKEY4
	if (gameId == "monkey4") {
		return Grim::EmiEngine::initKeymapsEmi(target);
	}
#endif

	return AdvancedMetaEngine::initKeymaps(target);
}

SaveStateList GrimMetaEngine::listSaves(const char *target) const {
	Common::String gameId = ConfMan.get("gameid", target);
	Common::String extra = ConfMan.get("extra", target);
	const bool isDemo = extra.contains("Demo");
	Common::Platform platform = Common::parsePlatform(ConfMan.get("platform", target));
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::StringArray filenames;
	Common::String pattern = gameId == "monkey4" ? "efmi###.gsv" : "grim##.gsv";
	SaveStateList saveList;

	if (platform == Common::kPlatformPS2)
		pattern = "efmi###.ps2";

	if (isDemo)
		return saveList; // Demos do not support saving

	filenames = saveFileMan->listSavefiles(pattern);

	for (Common::StringArray::const_iterator file = filenames.begin(); file != filenames.end(); ++file) {
		// Obtain the last digits of the filename, since they correspond to the save slot
		int slotNum = atoi(file->c_str() + 4);

		if (slotNum >= 0) {
			Common::SeekableReadStream *pFile = saveFileMan->openForLoading(*file);
			Common::Array<Common::String> strings;

			if (pFile && gameId == "monkey4") {
				if (EmiEngine::readSaveStrings(pFile, strings))
					saveList.push_back(SaveStateDescriptor(this, slotNum, EmiEngine::getSaveDescription(strings, slotNum)));
			} else if (pFile && readSaveStrings(pFile, strings)) {
				saveList.push_back(SaveStateDescriptor(this, slotNum, strings.empty() ? Common::String::format("Slot %d", slotNum) : strings[0]));
			}

			delete pFile;
		}
	}

	Common::sort(saveList.begin(), saveList.end(), SaveStateDescriptorSlotComparator());
	return saveList;
}

SaveStateDescriptor GrimMetaEngine::querySaveMetaInfos(const char *target, int slot) const {
	Common::String gameId = ConfMan.get("gameid", target);
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::String fileName = getSavegameFile(slot, target);

	Common::SeekableReadStream *pFile = saveFileMan->openForLoading(fileName);
	Common::Array<Common::String> strings;

	if (!pFile)
		return SaveStateDescriptor();

	if (gameId == "monkey4") {
		if (!EmiEngine::readSaveStrings(pFile, strings)) {
			delete pFile;
			return SaveStateDescriptor();
		}

		SaveStateDescriptor emiDesc(this, slot, EmiEngine::getSaveDescription(strings, slot));
		Graphics::Surface *emiThumbnail = EmiEngine::readSaveThumbnail(pFile);

		if (emiThumbnail)
			emiDesc.setThumbnail(emiThumbnail);

		delete pFile;
		return emiDesc;
	}

	if (!readSaveStrings(pFile, strings)) {
		delete pFile;
		return SaveStateDescriptor();
	}

	SaveStateDescriptor desc(this, slot, strings.empty() ? Common::String::format("Slot %d", slot) : strings[0]);
	Common::Array<byte> pixels;

	pFile->seek(0);

	if (readSaveThumbnail(pFile, pixels)) {
		if (ConfMan.hasKey("grim_gamma", target))
			correctSaveImageGamma(pixels, ConfMan.getFloat("grim_gamma", target));

		Graphics::Surface *pThumbnail = new Graphics::Surface();

		pThumbnail->create(kSaveImageWidth, kSaveImageHeight, Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
		memcpy(pThumbnail->getPixels(), pixels.data(), pixels.size());

		Graphics::Surface *pScaled = pThumbnail->scale(kLauncherThumbnailWidth, kLauncherThumbnailHeight, true);

		pThumbnail->free();
		delete pThumbnail;
		desc.setThumbnail(pScaled);
	}

	delete pFile;
	return desc;
}

} // End of namespace Grim

#if PLUGIN_ENABLED_DYNAMIC(GRIM)
	REGISTER_PLUGIN_DYNAMIC(GRIM, PLUGIN_TYPE_ENGINE, Grim::GrimMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(GRIM, PLUGIN_TYPE_ENGINE, Grim::GrimMetaEngine);
#endif
