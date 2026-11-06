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

#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/movie/movie.h"
#include "engines/grim/res.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

SputSmush::SputSmush(MoviePlayer *pMovie, Res *pRes, StdDisplay *pStdDisplay, SputRender *pSputRender, StdVBuffer *pMainGfxBuffer,
		WuRegistry *pWuRegistry) :
		_pMovie(pMovie), _pRes(pRes), _pStdDisplay(pStdDisplay), _pSputRender(pSputRender), _pMainGfxBuffer(pMainGfxBuffer),
		_pWuRegistry(pWuRegistry), _pHS(nullptr), _bStarted(0), _pPresentBuf(nullptr), _buf(0), _playing(0), _paused(0), _abort(0), _looping(0), _x(0), _y(0),
		_startTime(0), _lastPeriodicTime(0) {
	memset(_curMovieFile, 0, sizeof(_curMovieFile));
}

int SputSmush::getMovieTime() {
	return _pMovie->getMovieTime();
}

int SputSmush::initialize(SysStruct *pHS) {
	_pHS = pHS;

	// TODO: SmushPlay_SysInitialize(pHS)
	// TODO: SmushPlay_SysSetVideoCallbacks(): the callbacks are startVideo, stopVideo, lockMovieBuffer, unlockMovieBuffer, setVideoPalette

	_pPresentBuf = nullptr;
	_buf = 1;

	return 0;
}

int SputSmush::save(Common::WriteStream *pStream) {
	pStream->writeSint32LE(_playing);
	pStream->writeSint32LE(_paused);
	pStream->writeSint32LE(_abort);
	pStream->write(_curMovieFile, sizeof(_curMovieFile));
	pStream->writeSint32LE(_looping);
	pStream->writeSint32LE(_x);
	pStream->writeSint32LE(_y);

	if (pStream->err()) {
		warning("SputSmush::save(): failed to write the movie state");
		return 1;
	}

	return 0;
}

int SputSmush::restore(Common::ReadStream *pStream) {
	setTarget(_pMainGfxBuffer);

	_playing = pStream->readSint32LE();
	_paused = pStream->readSint32LE();
	_abort = pStream->readSint32LE();
	pStream->read(_curMovieFile, sizeof(_curMovieFile));
	_looping = pStream->readSint32LE();
	_x = pStream->readSint32LE();
	_y = pStream->readSint32LE();

	if (pStream->err() || pStream->eos()) {
		warning("SputSmush::restore(): failed to read the movie state");
		return 1;
	}

	if (!memchr(_curMovieFile, 0, sizeof(_curMovieFile))) {
		warning("SputSmush::restore(): the movie name is not terminated");
		_curMovieFile[sizeof(_curMovieFile) - 1] = '\0';
	}

	if (_playing)
		startMovie(_curMovieFile, _looping, _x, _y);

	return 0;
}

int SputSmush::startup() {
	char startSmush[10] = "";
	int bFound = _pWuRegistry->getStr("StartSmush", startSmush, sizeof(startSmush), "");

	if (!bFound || scumm_stricmp(startSmush, "FALSE")) {
		int startupFailed = 0; // TODO: SmushPlay_SysStartup()

		if (startupFailed)
			return 1;

		_bStarted = 1;
	}

	return 0;
}

int SputSmush::shutdown() {
	if (_bStarted) {
		// TODO: SmushPlay_SysShutdown()
	}

	return 0;
}

int SputSmush::terminate() {
	// TODO: SmushPlay_SysTerminate()
	return 0;
}

int SputSmush::periodic() {
	int result = 1;
	int finished = 0;

	if (!_bStarted || !_playing)
		return 1;

	if (_paused)
		return 0;

	// TODO: SmushPlay_SetTranslationTable(sputRender_ColorLookup)

	if (!_abort) {
		// TODO: SmushPlay_UpdateMovie(&finished): the player is updated by its own timer
		finished = !_pMovie->isPlaying();
	}

	if (finished || _abort) {
		_playing = 0;

		StdRect frameRect = { _pMovie->getX(), _pMovie->getY(), _pMovie->getWidth(), _pMovie->getHeight() };

		_pMovie->stop();
		_pSputRender->markAllPagesDirty(&frameRect, 0);
	} else {
		result = 0;
	}

	// TODO: SmushPlay_SysConfigureColorLookup(0)
	_lastPeriodicTime = g_system->getMillis();

	return result;
}

int SputSmush::setTarget(StdVBuffer *pBuffer) {
	if (!_bStarted)
		return 1;

	_pPresentBuf = pBuffer;

	return 0;
}

int SputSmush::startMovie(const char *name, int bLooping, int x, int y) {
	if (!_bStarted)
		return 1;

	if (_playing) {
		_pMovie->stop();
		_playing = 0;
	}

	// TODO: SmushPlay_SetTranslationTable(sputRender_ColorLookup)

	_looping = bLooping;
	_x = x;
	_y = y;

	// TODO: SmushPlay_StartMovie(): target size, pixel format flag, a negative x or y takes the position of the movie
	if (!_pMovie->play(name, bLooping != 0, x, y))
		return 1;

	_paused = 0;
	_abort = 0;
	_playing = 1;

	if (name != _curMovieFile) {
		if (strlen(name) >= sizeof(_curMovieFile))
			warning("SputSmush::startMovie(): the movie name is too long");

		strncpy(_curMovieFile, name, sizeof(_curMovieFile));
		_curMovieFile[sizeof(_curMovieFile) - 1] = '\0';
	}

	_startTime = g_system->getMillis();

	return 0;
}

int SputSmush::stopMovie() {
	int result = _bStarted;

	if (_bStarted) {
		_abort = 1;
		_paused = 0;
	}

	return result;
}

int SputSmush::getMovieLength(const char *name) {
	char path[128];
	int movieLength = 0;

	if (_pRes->findExternalFile(name, path, sizeof(path)) == 1)
		movieLength = 0; // TODO: SmushPlay_GetMovieLength(path)

	return movieLength;
}

int SputSmush::startVideo(int width, int height, int depth) {
	if (!_bStarted)
		return 1;

	if (!_pPresentBuf) {
		warning("SputSmush::startVideo(): the target is not set");
		return 1;
	}

	return _pPresentBuf->tex.width != width || _pPresentBuf->tex.height != height || _pPresentBuf->tex.format.bpp != depth;
}

int SputSmush::stopVideo() {
	return _bStarted == 0;
}

int SputSmush::lockMovieBuffer(LecBitmap *pBitmap, StdRect *pRect) {
	if (!_bStarted)
		return 1;

	if (!_pPresentBuf) {
		warning("SputSmush::lockMovieBuffer(): the target is not set");
		return 1;
	}

	_pStdDisplay->vBufferLock(_pPresentBuf);

	pBitmap->bytesPerPixel = _pPresentBuf->tex.format.bpp >> 3;
	pBitmap->pPixels = _pPresentBuf->pPixels;
	pBitmap->height = _pPresentBuf->tex.height;
	pBitmap->width = _pPresentBuf->tex.width;
	pBitmap->pitchInBytes = _pPresentBuf->tex.pitchInBytes;

	pRect->y = 0;
	pRect->x = 0;
	pRect->height = _pPresentBuf->tex.height;
	pRect->width = _pPresentBuf->tex.width;

	return 0;
}

int SputSmush::unlockMovieBuffer() {
	if (!_bStarted)
		return 1;

	if (!_pPresentBuf) {
		warning("SputSmush::unlockMovieBuffer(): the target is not set");
		return 1;
	}

	_pStdDisplay->vBufferUnlock(_pPresentBuf);

	return 0;
}

int SputSmush::setVideoPalette(uint32 *pPalette, int start, int count) {
	return _bStarted == 0;
}

int SputSmush::moviePlaying() {
	return _playing;
}

int SputSmush::pause(int bPause) {
	_paused = bPause;

	if (_pMovie->isPlaying())
		_pMovie->pause(bPause != 0);

	return 0;
}

} // End of namespace Grim
