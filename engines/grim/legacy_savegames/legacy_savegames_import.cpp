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

#include "common/archive.h"
#include "common/debug.h"
#include "common/memstream.h"
#include "common/path.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/legacy_savegames/legacy_savegames_controls.h"
#include "engines/grim/legacy_savegames/legacy_savegames_import.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"

namespace Grim {

// Values of the legacy engine
static const uint32 kLegacyIrisClose = 1;
static const uint32 kLegacyModeDraw = 4;

// The lib modes from 1 (pause) to 4 (user paint) are the ones of the legacy EngineMode
static const int kFirstLibMode = 1;
static const int kLastLibMode = 4;
static const int kDefaultPrevMode = 2;

static const uint kMaxMovieNameSize = 128;

int32 LegacyImportContext::getDeadId(Common::HashMap<int32, int32> &deadIds, int32 oldId) {
	if (deadIds.contains(oldId))
		return deadIds.getVal(oldId);

	// Odd and negative: no pointer, index or handle of the running game is like that
	int32 deadId = nextDeadId;

	nextDeadId -= 2;
	deadIds.setVal(oldId, deadId);

	return deadId;
}

LegacyImporter::LegacyImporter(GrimEngine *pGrim) : _pGrim(pGrim), _pLuaStream(nullptr), _canPlayMovie(false), _prevMode(kDefaultPrevMode) {
}

LegacyImporter::~LegacyImporter() {
	delete _pLuaStream;
	_pLuaStream = nullptr;
}

bool LegacyImporter::archiveHasFile(const Common::String &name) {
	Common::String path(name);

	path.replace('\\', '/');
	path.toLowercase();

	Common::String baseName = Common::lastPathComponent(path, '/');
	Common::Path fullPath(path, '/');
	Common::Path basePath(baseName, Common::Path::kNoSeparator);

	return SearchMan.hasFile(fullPath) || SearchMan.hasFile(basePath);
}

Common::SeekableReadStream *LegacyImporter::openCostumeFile(const Common::String &name) {
	return _pGrim->openFile(name.c_str());
}

const LegacyBitmap *LegacyImporter::findBitmap(int32 id) const {
	for (uint i = 0; i < _data.bitmaps.size(); i++) {
		if (_data.bitmaps[i].id == id)
			return &_data.bitmaps[i];
	}

	return nullptr;
}

const LegacyBitmapFont *LegacyImporter::findBitmapFont(int32 id) const {
	for (uint i = 0; i < _data.bitmapFonts.size(); i++) {
		if (_data.bitmapFonts[i].id == id)
			return &_data.bitmapFonts[i];
	}

	return nullptr;
}

const LegacyObjectState *LegacyImporter::findObjectState(int32 id) const {
	for (uint i = 0; i < _data.objectStates.size(); i++) {
		if (_data.objectStates[i].id == id)
			return &_data.objectStates[i];
	}

	return nullptr;
}

int LegacyImporter::findSetIndex(int32 id) const {
	for (uint i = 0; i < _data.sets.size(); i++) {
		if (_data.sets[i].id == id)
			return (int)i;
	}

	return -1;
}

bool LegacyImporter::prepare(Common::SeekableReadStream *pStream) {
	LegacySaveFile file(pStream, DisposeAfterUse::NO);

	if (!file.open() || !file.readAll(this, _data))
		return false;

	_pLuaStream = file.openLuaSection();

	if (!_pLuaStream) {
		warning("LegacyImporter::prepare(): the savegame has no Lua section");
		return false;
	}

	if (!planRooms())
		return false;

	planFonts();
	checkPools();

	// The mode to go back to when the user paint mode that restore() starts is over
	uint prevMode = _data.grim.mode;

	if (prevMode == kLegacyModeDraw)
		prevMode = _data.grim.previousMode;

	if (prevMode >= (uint)kFirstLibMode && prevMode <= (uint)kLastLibMode) {
		_prevMode = (int)prevMode;
	} else {
		warning("LegacyImporter::prepare(): the savegame was made in the mode %u, going on in the normal mode", prevMode);
		_prevMode = kDefaultPrevMode;
	}

	getDefaultFontNames(_pGrim->_pSputText, &_printLineFont, &_defaultFont, &_blastTextFont);

	return true;
}

void LegacyImporter::planFonts() {
	for (uint i = 0; i < _data.bitmapFonts.size(); i++) {
		const LegacyBitmapFont &font = _data.bitmapFonts[i];

		if (archiveHasFile(font.fileName))
			_fontPlan.push_back(i);
		else
			warning("LegacyImporter::planFonts(): the font file %s is missing, the font is skipped", font.fileName.c_str());
	}
}

void LegacyImporter::checkPools() {
	if (!_data.ttfFonts.empty())
		warning("LegacyImporter::checkPools(): the %u TTF fonts of the savegame cannot be imported", _data.ttfFonts.size());

	if (!_data.textObjects.empty())
		warning("LegacyImporter::checkPools(): the %u text objects of the savegame are not imported", _data.textObjects.size());

	if (!_data.primitives.empty())
		warning("LegacyImporter::checkPools(): the %u primitives of the savegame are not imported", _data.primitives.size());

	const LegacyMovieState &movie = _data.movie;
	char path[kMaxMovieNameSize];

	_canPlayMovie = false;

	if (movie.fileName.empty() || movie.videoFinished)
		return;

	if (movie.fileName.size() >= kMaxMovieNameSize)
		warning("LegacyImporter::checkPools(): the movie name %s is too long, the movie is skipped", movie.fileName.c_str());
	else if (_pGrim->_pRes->findExternalFile(movie.fileName.c_str(), path, sizeof(path)) != 1)
		warning("LegacyImporter::checkPools(): the movie file %s is missing, the movie is skipped", movie.fileName.c_str());
	else
		_canPlayMovie = true;
}

bool LegacyImporter::apply() {
	SputRender *pSputRender = _pGrim->_pSputRender;

	pSputRender->drawProgressBar(0.2f);
	importFonts();
	importText();
	pSputRender->drawProgressBar(0.3f);

	if (!importRooms())
		return false;

	pSputRender->drawProgressBar(0.4f);
	resetActors(_pGrim->_pSputActor);

	if (!importActors())
		return false;

	int camActorIndex = importCameraActor();

	importSelectedActor();
	importShadowColor();
	importRender(camActorIndex);
	importPrimitives();
	importMovie();

	_pGrim->_libMode = 4;
	_pGrim->_sputPrevMode = _prevMode;
	pSputRender->drawProgressBar(0.75f);

	if (!importLua())
		return false;

	pSputRender->drawProgressBar(1.0f);
	importSounds();
	applyDeveloperMode();
	repeatMusicPause();
	remapControls();

	return true;
}

int LegacyImporter::importCameraActor() {
	SputActorModule *pSputActor = _pGrim->_pSputActor;
	int index = pSputActor->allocateActor();

	pSputActor->actorDeref(index)->flags |= 1;

	return index;
}

void LegacyImporter::importSelectedActor() {
	int32 id = _data.grim.selectedActorId;

	if (!id)
		return;

	if (!_ctx.actorIndices.contains(id)) {
		warning("LegacyImporter::importSelectedActor(): the selected actor %d was not imported", id);
		return;
	}

	_pGrim->_pSputActor->setSelectedActor(_ctx.actorIndices.getVal(id));
}

void LegacyImporter::importShadowColor() {
	const LegacyColor &color = _data.driver.shadowColor;

	_pGrim->_pSputCostume->setShadowColor(color.r, color.g, color.b);
}

void LegacyImporter::importRender(int camActorIndex) {
	const LegacyIrisState &iris = _data.iris;
	int irisMode = 0;
	int centerX = 320;
	int centerY = 240;
	int duration = 0;
	int elapsed = 0;

	if (iris.playing && iris.length > 0) {
		// The legacy iris shrinks to the target point or grows from it: x1 = target * factor
		float factor = (float)iris.currTime / (float)iris.length;

		if (iris.direction != kLegacyIrisClose)
			factor = 1.0f - factor;

		irisMode = (iris.direction == kLegacyIrisClose) ? 2 : 3;
		duration = iris.length;
		elapsed = iris.currTime;

		if (factor > 0.05f) {
			centerX = CLIP((int)((float)iris.x1 / factor + 0.5f), 0, 640);
			centerY = CLIP((int)((float)iris.y1 / factor + 0.5f), 0, 480);
		}
	} else if (!iris.playing && iris.direction == kLegacyIrisClose) {
		irisMode = 1;
	}

	setRenderState(_pGrim->_pSputRender, irisMode, centerX, centerY, duration, elapsed, camActorIndex);
}

void LegacyImporter::importPrimitives() {
	_pGrim->_pSputPrimitive->getCurrent()->purge();
}

void LegacyImporter::importMovie() {
	const LegacyMovieState &movie = _data.movie;

	if (_canPlayMovie)
		startMovie(_pGrim->_pSputSmush, movie.fileName.c_str(), movie.videoLooping ? 1 : 0, movie.x, movie.y);
	else
		startMovie(_pGrim->_pSputSmush, nullptr, 0, 0, 0);
}

void LegacyImporter::importSounds() {
	if (_data.soundData.empty()) {
		warning("LegacyImporter::importSounds(): the savegame has no sound data");
		return;
	}

	Common::MemoryReadStream stream(_data.soundData.data(), _data.soundData.size());

	if (_data.minorVersion >= 30) {
		_pGrim->soundsRestore(&stream, true);

		if (stream.pos() != stream.size())
			warning("LegacyImporter::importSounds(): %d bytes of the sound data are left", (int)(stream.size() - stream.pos()));

		return;
	}

	if (_data.soundData.size() < 8) {
		warning("LegacyImporter::importSounds(): the sound data is too short");
		return;
	}

	int32 stateIndex = stream.readSint32LE();
	int32 seqIndex = stream.readSint32LE();

	_pGrim->getImuse()->soundsRestoreLegacy(stateIndex, seqIndex);
}

void LegacyImporter::remapControls() {
	SputControl *pSputControl = _pGrim->_pSputControl;
	LegacyLuaRefs engineRefs;

	getEngineRefs(&engineRefs);

	Common::String summary = legacyRemapControls(engineRefs.systemTable, pSputControl->getSymbols(), pSputControl->getNumSymbols());

	debug(1, "LegacyImporter::remapControls(): %s", summary.c_str());
}

void GrimEngine::restoreLegacy() {
	LegacyImporter importer(this);

	if (!importer.prepare(_pSaveReadStream)) {
		warning("GrimEngine::restoreLegacy(): the savegame cannot be loaded, the game goes on");
		delete _pSaveReadStream;
		_pSaveReadStream = nullptr;

		return;
	}

	restorePrologue();

	if (!importer.apply())
		error("GrimEngine::restoreLegacy(): the savegame was damaged halfway and the game cannot go on");

	restoreEpilogue();
}

} // End of namespace Grim
