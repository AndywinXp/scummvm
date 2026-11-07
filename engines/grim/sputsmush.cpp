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

#include "graphics/surface.h"

#include "engines/grim/movie/movie.h"
#include "engines/grim/res.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

SputSmush::SputSmush(MoviePlayer *pMovie, Res *pRes, StdDisplay *pStdDisplay, SputRender *pSputRender, StdVBuffer *pMainGfxBuffer,
		WuRegistry *pWuRegistry) :
		_pMovie(pMovie), _pRes(pRes), _pStdDisplay(pStdDisplay), _pSputRender(pSputRender), _pMainGfxBuffer(pMainGfxBuffer),
		_pWuRegistry(pWuRegistry), _pHS(nullptr), _bPlayInitialized(0), _bPlayStarted(0), _pTranslationTable(nullptr),
		_colorLookupConfig(0), _bStarted(0), _pPresentBuf(nullptr), _buf(0), _playing(0), _paused(0), _abort(0), _looping(0), _x(0), _y(0),
		_startTime(0), _lastPeriodicTime(0) {
	memset(_curMovieFile, 0, sizeof(_curMovieFile));
}

int SputSmush::getMovieTime() {
	return _pMovie->getMovieTime();
}

int SputSmush::initialize(SysStruct *pHS) {
	_pHS = pHS;

	smushPlaySysInitialize();

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
		if (smushPlaySysStartup())
			return 1;

		_bStarted = 1;
	}

	return 0;
}

int SputSmush::shutdown() {
	if (_bStarted)
		smushPlaySysShutdown();

	return 0;
}

int SputSmush::terminate() {
	smushPlaySysTerminate();

	return 0;
}

int SputSmush::periodic() {
	int result = 1;
	int finished = 0;

	if (!_bStarted || !_playing)
		return 1;

	if (_paused)
		return 0;

	smushPlaySetTranslationTable(_pSputRender->getColorLookup());

	if (_abort || !smushPlayUpdateMovie(&finished)) {
		if (finished || _abort) {
			_playing = 0;

			StdRect frameRect = { _pMovie->getX(), _pMovie->getY(), _pMovie->getWidth(), _pMovie->getHeight() };

			smushPlayStopMovie();
			_pSputRender->markAllPagesDirty(&frameRect, 0);
		} else {
			result = 0;
		}
	}

	smushPlaySysConfigureColorLookup(0);
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
		smushPlayStopMovie();
		_playing = 0;
	}

	if (!_pPresentBuf) {
		warning("SputSmush::startMovie(): the target is not set");
		return 1;
	}

	smushPlaySetTranslationTable(_pSputRender->getColorLookup());

	_looping = bLooping;
	_x = x;
	_y = y;

	if (smushPlayStartMovie(name, bLooping, x, y, _pPresentBuf->tex.width, _pPresentBuf->tex.height))
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
		movieLength = smushPlayGetMovieLength(path);

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

int SputSmush::smushPlaySysInitialize() {
	if (_bPlayInitialized)
		return 1;

	_bPlayInitialized = 1;

	return 0;
}

void SputSmush::smushPlaySysTerminate() {
	_bPlayInitialized = 0;
}

int SputSmush::smushPlaySysStartup() {
	if (!_bPlayInitialized || _bPlayStarted)
		return 1;

	_bPlayStarted = 1;

	return 0;
}

void SputSmush::smushPlaySysShutdown() {
	_bPlayStarted = 0;
}

void SputSmush::smushPlaySysConfigureColorLookup(int config) {
	_colorLookupConfig = config;
}

void SputSmush::smushPlaySetTranslationTable(const uint16 *pTable) {
	_pTranslationTable = pTable;
}

int SputSmush::smushPlayStartMovie(const char *name, int bLooping, int x, int y, int width, int height) {
	if (startVideo(width, height, 16))
		return 1;

	if (!_pMovie->play(name, bLooping != 0, x, y)) {
		stopVideo();

		return 1;
	}

	return 0;
}

int SputSmush::smushPlayStopMovie() {
	_pMovie->stop();
	stopVideo();

	return 0;
}

int SputSmush::smushPlayUpdateMovie(int *pFinished) {
	LecBitmap bitmap;
	StdRect rect;
	Graphics::Surface *pFrame = _pMovie->getDstSurface();

	_pMovie->clearUpdateNeeded();

	if (!lockMovieBuffer(&bitmap, &rect)) {
		smushPlayBlit(&bitmap, pFrame, &rect);
		unlockMovieBuffer();
	}

	*pFinished = !_pMovie->isPlaying();

	return 0;
}

int SputSmush::smushPlayBlit(LecBitmap *pDest, const Graphics::Surface *pSource, const StdRect *pRect) {
	if (!pDest || !pSource || !pSource->getPixels())
		return 0;

	if (pSource->format.bytesPerPixel != 2 || pDest->bytesPerPixel != 2) {
		warning("SputSmush::smushPlayBlit(): only 16 bpp frames are supported");
		return 0;
	}

	int x = _pMovie->getX();
	int y = _pMovie->getY();
	int width = pSource->w;
	int height = pSource->h;

	if (x < pRect->x) {
		width += x - pRect->x;
		x = pRect->x;
	}

	if (x + width > pRect->x + pRect->width)
		width = pRect->x + pRect->width - x;

	if (y < pRect->y) {
		height += y - pRect->y;
		y = pRect->y;
	}

	if (y + height > pRect->y + pRect->height)
		height = pRect->y + pRect->height - y;

	if (width < 0 || height < 0) {
		warning("SputSmush::smushPlayBlit(): the movie is outside of the target");
		return 0;
	}

	byte *pDestPixels = pDest->pPixels + pDest->pitchInBytes * y + pDest->bytesPerPixel * x;

	smushPlayBlitCopy((const byte *)pSource->getPixels(), pSource->pitch, pDestPixels, pDest->pitchInBytes,
			pSource->format.bytesPerPixel * width, height);

	return 0;
}

void SputSmush::smushPlayBlitCopy(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows) {
	int numPixels = (rowBytes >> 2) * 2;

	for (int row = 0; row < rows; row++) {
		const uint16 *pSourceRow = (const uint16 *)(pSource + sourcePitch * row);
		uint16 *pDestRow = (uint16 *)(pDest + destPitch * row);

		if (_pTranslationTable) {
			for (int i = 0; i < numPixels; i++)
				pDestRow[i] = _pTranslationTable[pSourceRow[i]];
		} else {
			for (int i = 0; i < numPixels; i++)
				pDestRow[i] = pSourceRow[i];
		}
	}
}

int SputSmush::smushPlayGetMovieLength(const char *path) {
	if (!_pHS) {
		warning("SputSmush::smushPlayGetMovieLength(): the module is not initialized");
		return 0;
	}

	Common::SeekableReadStream *pStream = _pHS->openFile(path);

	if (!pStream)
		return 0;

	SmushMovieInfo info;

	memset(&info, 0, sizeof(info));
	smushPlayGetInfo(pStream, &info);
	delete pStream;
	pStream = nullptr;

	return (int)((double)info.frames * 66.666664);
}

int SputSmush::smushPlayGetInfo(Common::SeekableReadStream *pStream, SmushMovieInfo *pInfo) {
	if (!pStream)
		return 0;

	int32 position = pStream->pos();

	pStream->seek(16, SEEK_SET);
	pInfo->version0 = pStream->readByte();
	pInfo->version1 = pStream->readByte();
	pInfo->frames = pStream->readSint16LE();
	pInfo->x = pStream->readSint16LE();
	pInfo->y = pStream->readSint16LE();
	pInfo->width = pStream->readSint16LE();
	pInfo->height = pStream->readSint16LE();
	pInfo->field_C = pStream->readSint16LE();
	pInfo->frameDelay = pStream->readSint32LE();
	pInfo->field_12 = pStream->readSint32LE();
	pStream->read(pInfo->palette, sizeof(pInfo->palette));
	pInfo->field_416 = pStream->readSint16LE();

	if (pInfo->width <= 0 || pInfo->height <= 0 || pInfo->field_12 <= 0)
		return -2;

	pStream->seek(position, SEEK_SET);

	return 0;
}

} // End of namespace Grim
