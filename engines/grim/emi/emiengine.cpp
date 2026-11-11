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

#include "common/archive.h"
#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/ustr.h"

#include "gui/message.h"

#include "engines/grim/debug.h"
#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/emidebugger.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/movie.h"
#include "engines/grim/emi/polyglot_v2.h"
#include "engines/grim/emi/primitivequeue_v2.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/sputcontrol_v2.h"
#include "engines/grim/emi/textevent.h"
#include "engines/grim/inputdialog.h"
#include "engines/grim/lab.h"
#include "engines/grim/update/update.h"

namespace Grim {

static const uint kMaxHardCap = 0x4000000;
static const uint kReservedMemory = 0x800000;

bool EmiEngine::_modulesStarted = false;
bool EmiEngine::_gameviewOpen = false;
Lua_V2 *EmiEngine::_lua = nullptr;
KeyGenerator *EmiEngine::_keyGenerator = nullptr;
int EmiEngine::_screenshotCount = 0;
int EmiEngine::_frameCount = 0;
float EmiEngine::_rawFrameTime = 0.0f;
double EmiEngine::_lastStamp = 0.0;
bool EmiEngine::_firstFrame = true;
bool EmiEngine::_backspaceHeld = false;
bool EmiEngine::_controlHeld = false;
TextEvent *EmiEngine::_statsEvents[EmiEngine::kNumStatsEvents] = { nullptr, nullptr, nullptr, nullptr, nullptr };
int EmiEngine::_mode = 0;
int EmiEngine::_pausedMode = 0;
int EmiEngine::_discNumber = 0;
int EmiEngine::_frameDumpCount = 0;
LString EmiEngine::_frameDumpPath;
int EmiEngine::_savedMode = 0;
int EmiEngine::_cpuSpeed = 1000;
LString EmiEngine::_island;
float EmiEngine::_frameTime = 0.0f;
bool EmiEngine::_dumpingFrames = false;
bool EmiEngine::_saveLoadIdle = true;
int EmiEngine::_frontCopyCount = 0;
int EmiEngine::_pendingMusicState = -1;
Polyglot_v2 *EmiEngine::_polyglot = nullptr;

EmiEngine::EmiEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language) :
		GrimEngine(syst, gameFlags, gameType, platform, language) {
	setDebugger(new EmiDebugger());

	_polyglot = new Polyglot_v2();
}

EmiEngine::~EmiEngine() {
	delete _polyglot;
	_polyglot = nullptr;
}

Polyglot_v2 *EmiEngine::getPolyglot() {
	return _polyglot;
}

const char *EmiEngine::getUpdateFilename() {
	if (getGamePlatform() != Common::kPlatformWindows || (getGameFlags() & ADGF_DEMO))
		return nullptr;

	if (SearchMan.hasFile("patch.m4b"))
		return nullptr;

	switch (getGameLanguage()) {
	case Common::FR_FRA:
		return "MonkeyUpdate_FRA.exe";
	case Common::DE_DEU:
		return "MonkeyUpdate_DEU.exe";
	case Common::IT_ITA:
		return "MonkeyUpdate_ITA.exe";
	case Common::PT_BRA:
		return "MonkeyUpdate_BRZ.exe";
	case Common::ES_ESP:
		return "MonkeyUpdate_ESP.exe";
	default:
		return "MonkeyUpdate.exe";
	}
}

void EmiEngine::mountEmiPatchArchives() {
	const char *updateFilename = getUpdateFilename();

	if (updateFilename && !SearchMan.hasArchive("update")) {
		Common::File *updateStream = new Common::File();

		if (updateStream->open(updateFilename)) {
			Common::Archive *update = loadUpdateArchive(updateStream);

			if (update)
				SearchMan.add("update", update, 1);
		} else {
			delete updateStream;
		}

		if (!SearchMan.hasArchive("update")) {
			Common::String urlForRequiredDataFiles = "https://wiki.scummvm.org/index.php?title=Escape_from_Monkey_Island#Required_data_files";
			Common::U32String errorMessage = Common::U32String::format(_("The original patch of Escape from Monkey Island\n"
					"is missing. Please put the file %s\n"
					"in the game data files directory.\n"
					"Go to %s to see a list\n"
					"of the needed files."), updateFilename, urlForRequiredDataFiles.c_str());

			GUIErrorMessageWithURL(errorMessage, urlForRequiredDataFiles.c_str());
			error("%s not found", updateFilename);
		}
	}

	if (!SearchMan.hasFile("monkey4-patch.m4b"))
		error("%s", "monkey4-patch.m4b not found");

	if (!SearchMan.hasArchive("monkey4-patch.m4b")) {
		Common::ArchiveMemberList files;

		SearchMan.listMatchingMembers(files, "monkey4-patch.m4b");

		if (!files.empty()) {
			Lab *lab = new Lab();

			if (lab->open(files.front()->getPathInArchive(), false))
				SearchMan.add("monkey4-patch.m4b", lab, 0, true);
			else
				delete lab;
		}
	}

	Common::ArchiveMemberList patches;

	Debug::debug(Debug::Patchr, "%d patch files available", SearchMan.listMatchingMembers(patches, "*.patchr"));
}

void EmiEngine::setPause(bool pause) {
	if (!_mode)
		return;

	if (pause) {
		if (_mode == kModePaused)
			return;

		_pausedMode = _mode;
		_mode = kModePaused;
		Sounds::pauseMusicStreams();
		Movie::pauseCurrent();
	} else if (_mode == kModePaused) {
		_mode = _pausedMode;
		Sounds::resumeMusicStreams();
		Movie::resumeCurrent();
	}
}

int EmiEngine::getDiscNumber() {
	return _discNumber;
}

bool EmiEngine::findDisc(int disc) {
	if (disc != 1 && disc != 2)
		return false;

	// TODO the search of the CD is not ported, the disc is always found
	_discNumber = disc;

	return true;
}

uint EmiEngine::getTotalPhysMem() {
	// TODO the original asks the system, this is a constant
	return 0x40000000;
}

int EmiEngine::messageBox(const char *caption, const char *text, uint flags) {
	Common::String message;

	if (caption && *caption) {
		message = caption;
		message += "\n\n";
	}

	if (text)
		message += text;

	Common::U32String dialogText(message);

	if (flags == kMessageBoxYesNo) {
		GUI::MessageDialog dialog(dialogText, _("Yes"), _("No"));

		if (dialog.runModal() == GUI::kMessageOK)
			return kMessageBoxYes;

		return kMessageBoxNo;
	}

	if (flags == kMessageBoxYesNoCancel) {
		Common::U32StringArray altButtons;

		altButtons.push_back(_("No"));
		altButtons.push_back(_("Cancel"));

		GUI::MessageDialog dialog(dialogText, _("Yes"), altButtons);
		int result = dialog.runModal();

		if (result == GUI::kMessageOK)
			return kMessageBoxYes;

		if (result == GUI::kMessageAlt)
			return kMessageBoxNo;

		return kMessageBoxCancel;
	}

	GUI::MessageDialog dialog(dialogText);

	dialog.runModal();

	return kMessageBoxOk;
}

void EmiEngine::showMessage(const char *text, const char *caption) {
	if (CDrawContext::context)
		CDrawContext::context->prepareDialog();

	const char *translatedCaption = _polyglot->getTranslation(caption);
	const char *translatedText = _polyglot->getTranslation(text);

	messageBox(translatedCaption, translatedText, kMessageBoxTypeOk);
}

bool EmiEngine::inputBox(const char *caption, const char *prompt, char *buffer, int bufferSize, const char *defaultText) {
	Common::String message = caption;

	message += ": ";
	message += prompt;

	Common::String defaultString;

	if (defaultText)
		defaultString = defaultText;

	InputDialog dialog(message, defaultString);

	if (!dialog.runModal())
		return false;

	Common::strlcpy(buffer, dialog.getString().c_str(), bufferSize);

	return true;
}

bool EmiEngine::playMovie(const char *fileName, bool subtitles, bool flag6194, bool flag6195, int startFrame) {
	LString name = fileName;

	if (!strrchr(fileName, '.'))
		name += ".bik";

	LString resolvedName;

	if (!gpFileServices->resolvePath(resolvedName, name.c_str()))
		return false;

	if (!Movie::startMovie(resolvedName.c_str(), subtitles, flag6194, flag6195, startFrame))
		return false;

	_mode = kModeMovie;

	return true;
}

void EmiEngine::setPendingMusicState(int musicState) {
	_pendingMusicState = musicState;
}

int EmiEngine::getMode() {
	return _mode;
}

void EmiEngine::setMode(int newMode) {
	_mode = newMode;
}

int EmiEngine::getSavedMode() {
	return _savedMode;
}

void EmiEngine::setSavedMode(int newMode) {
	_savedMode = newMode;
}

int EmiEngine::getCpuSpeed() {
	return _cpuSpeed;
}

void EmiEngine::setCpuSpeed(int newSpeed) {
	_cpuSpeed = newSpeed;
}

bool EmiEngine::hasGammaControl() {
	return CDrawContext::context && CDrawContext::context->hasGammaControl();
}

void EmiEngine::setGamma(float gamma) {
	if (CDrawContext::context)
		CDrawContext::context->setGamma(gamma);
}

bool EmiEngine::setIsland(const char *island) {
	if (CBundleServices::isIslandBundleOpen(island))
		return true;

	if (!_island.empty()) {
		CBundleServices::closeIslandBundles(_island.c_str());

		ResourceManager *resourceManager = dynamic_cast<ResourceManager *>(gpResourceManager);

		if (resourceManager)
			resourceManager->purgeAll();
	}

	_island = island;

	if (_island.empty())
		return true;

	return CBundleServices::openIslandBundles(_island.c_str());
}

float EmiEngine::getFrameTime() {
	return _frameTime;
}

bool EmiEngine::isDumpingFrames() {
	return _dumpingFrames;
}

void EmiEngine::setSaveLoadIdle(bool idle) {
	_saveLoadIdle = idle;
}

bool EmiEngine::startupModules(CDrawContext *drawContext) {
	if (_modulesStarted)
		return false;

	ResourceManager::startup();
	Room_v2::startup();
	SputControl_v2::startup();

	_lua = new Lua_V2();
	_lua->startup();

	TextEvent::startup(drawContext);
	ChoreInstance::startup();
	Actor::startup(drawContext);
	PrimitiveQueue_v2::startup();
	Sounds::startSounds(false);
	startupMovies(drawContext);

	uint physicalMemory = getTotalPhysMem();
	uint hardCap = kMaxHardCap;

	if (hardCap >= physicalMemory - kReservedMemory)
		hardCap = getTotalPhysMem() - kReservedMemory;

	LMemoryServices::mHardCap = hardCap;
	_modulesStarted = true;

	return true;
}

void EmiEngine::shutdownModules() {
	if (!_modulesStarted)
		return;

	SputControl_v2::shutdown();
	Sounds::shutdownSounds();
	Actor::shutdown();
	ChoreInstance::shutdown();
	TextEvent::shutdown();

	_lua->shutdown();
	delete _lua;
	_lua = nullptr;

	Room_v2::shutdown();
	PrimitiveQueue_v2::shutdown();
	ResourceManager::shutdown();

	_modulesStarted = false;
}

bool EmiEngine::openGameview(const char *bootParam) {
	if (_gameviewOpen) {
		gpConsoleServices->logError("Game is already running.\n");

		return false;
	}

	if (!_lua->boot(0, bootParam))
		return false;

	_mode = kModeGame;

	if (!SputControl_v2::open())
		return false;

	_gameviewOpen = true;

	return true;
}

void EmiEngine::closeGameview() {
	if (!_gameviewOpen) {
		gpConsoleServices->logError("Gameview not open.\n");

		return;
	}

	_mode = kModeNone;
	SputControl_v2::close();
	Room_v2::loadRoom(nullptr);
	releaseSubtitleFont();
	_gameviewOpen = false;
}

bool EmiEngine::callExitHook() {
	if (!_lua) {
		warning("EmiEngine::callExitHook(): Lua is not started");

		return true;
	}

	return !_lua->doExitCallback();
}

void EmiEngine::updateSounds(bool force) {
	Sounds::soundsUpdate(&force);
}

bool EmiEngine::updateMovie(float frameTime) {
	return Movie::updateMovie();
}

void EmiEngine::startupMovies(CDrawContext *drawContext) {
	Movie::startupMovies(drawContext);
}

void EmiEngine::releaseSubtitleFont() {
	Movie::releaseSubtitleFont();
}

void EmiEngine::setMusicState(int musicState, int startMs) {
	bool immediate = false;

	Sounds::setMusicState(musicState, startMs, immediate);
}

} // End of namespace Grim
