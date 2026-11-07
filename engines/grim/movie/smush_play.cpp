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

#include "common/compression/deflate.h"
#include "common/endian.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/timer.h"

#include "graphics/surface.h"

#include "engines/grim/movie/codecs/smush_decoder.h"
#include "engines/grim/movie/smush_animator.h"
#include "engines/grim/movie/smush_mixer.h"
#include "engines/grim/movie/smush_play.h"
#include "engines/grim/movie/smush_sound.h"
#include "engines/grim/movie/smush_timer.h"
#include "engines/grim/movie/smush_wave.h"

namespace Grim {

SmushPlay::SmushPlay(bool remastered) :
		_pHS(nullptr), _pVideo(nullptr), _bInitialized(0), _bStarted(0), _bTimerInstalled(0), _bMovieIsPlaying(0), _movieFlags(0),
		_pInternalBitmap(nullptr), _pTranslationTable(nullptr), _colorLookupConfig(0) {
	memset(&_movieInfo, 0, sizeof(_movieInfo));
	memset(_curFileName, 0, sizeof(_curFileName));
	memset(&_frameRect, 0, sizeof(_frameRect));
	memset(&_srcRect, 0, sizeof(_srcRect));
	memset(&_internalBitmap, 0, sizeof(_internalBitmap));

	_pDecoder = new SmushDecoder();
	_pTimer = new SmushTimer();
	_pDriver = new SmushDriver(_pTimer, remastered);
	_pSound = new SmushSound(_pTimer, _pDriver);
	_pWave = new SmushWave(_pSound);
	_pAnimPlay = new SmushAnimPlay(_pTimer, _pSound);
	_pAnimator = new SmushAnimator(_pDecoder);
	_pDecoder->setWave(_pWave);
}

SmushPlay::~SmushPlay() {
	if (_bStarted)
		sysShutdown();

	Common::StackLock lock(_mutex);

	delete _pAnimPlay;
	_pAnimPlay = nullptr;
	delete _pAnimator;
	_pAnimator = nullptr;
	delete _pWave;
	_pWave = nullptr;
	delete _pSound;
	_pSound = nullptr;
	delete _pDriver;
	_pDriver = nullptr;
	delete _pTimer;
	_pTimer = nullptr;
	delete _pDecoder;
	_pDecoder = nullptr;
}

int SmushPlay::sysInitialize(SysStruct *pHS) {
	_pHS = pHS;

	if (_bInitialized)
		return 1;

	_bInitialized = 1;

	return 0;
}

void SmushPlay::sysTerminate() {
	_bInitialized = 0;
}

int SmushPlay::sysStartup() {
	if (!_bInitialized || _bStarted)
		return 1;

	{
		Common::StackLock lock(_mutex);

		if (_pTimer->moduleStartup())
			return 1;

		if (_pSound->moduleStartup()) {
			_pTimer->moduleShutdown();
			return 1;
		}

		if (_pWave->moduleStartup()) {
			_pSound->moduleShutdown();
			_pTimer->moduleShutdown();
			return 1;
		}
	}

	g_system->getTimerManager()->installTimerProc(&timerCallback, 10000, this, "smushPlay");
	_bTimerInstalled = 1;
	_bStarted = 1;

	return 0;
}

void SmushPlay::sysShutdown() {
	if (!_bStarted)
		return;

	if (_bTimerInstalled) {
		g_system->getTimerManager()->removeTimerProc(&timerCallback);
		_bTimerInstalled = 0;
	}

	Common::StackLock lock(_mutex);

	_pWave->moduleShutdown();
	_pSound->moduleShutdown();
	_pTimer->moduleShutdown();
	_bStarted = 0;
}

void SmushPlay::sysSetVideoCallbacks(SmushPlayVideo *pVideo) {
	_pVideo = pVideo;
}

void SmushPlay::sysConfigureColorLookup(int config) {
	_colorLookupConfig = config;
}

void SmushPlay::timerCallback(void *pContext) {
	SmushPlay *pPlay = static_cast<SmushPlay *>(pContext);
	Common::StackLock lock(pPlay->_mutex);

	pPlay->_pTimer->timerThread();
}

int SmushPlay::startMovie(const char *name, int flags, int bufferBytes, int x, int y, int width, int height, int offset, int pixelFormat) {
	Common::StackLock lock(_mutex);

	if (!_pVideo)
		return 1;

	Common::SeekableReadStream *pStream = openFile(name);

	if (!pStream)
		return 1;

	if (strlen(name) >= sizeof(_curFileName))
		warning("SmushPlay::startMovie(): the movie name is too long");

	strncpy(_curFileName, name, sizeof(_curFileName) - 1);
	_curFileName[sizeof(_curFileName) - 1] = '\0';

	pStream->seek(offset, SEEK_CUR);

	if (getInfo(pStream, &_movieInfo)) {
		delete pStream;
		return 1;
	}

	delete pStream;
	pStream = nullptr;

	if (_pVideo->startVideo(width, height, 16, pixelFormat))
		return 1;

	_movieFlags = flags & 0xFFFFFFF;

	if (openAnim(name, 15)) {
		_pVideo->stopVideo();
		return 1;
	}

	if (x < 0)
		_frameRect.x = _movieInfo.x;
	else
		_frameRect.x = x;

	if (y < 0)
		_frameRect.y = _movieInfo.y;
	else
		_frameRect.y = y;

	_frameRect.width = _movieInfo.width;
	_frameRect.height = _movieInfo.height;
	_bMovieIsPlaying = 1;

	return 0;
}

int SmushPlay::stopMovie() {
	Common::StackLock lock(_mutex);

	if (_bMovieIsPlaying) {
		closeAnim();
		_pSound->souEngineStopAll();
		_pInternalBitmap = nullptr;
	}

	_bMovieIsPlaying = 0;

	if (_pVideo)
		_pVideo->stopVideo();

	return 0;
}

int SmushPlay::updateMovie(int *pFinished) {
	Common::StackLock lock(_mutex);
	LecBitmap bitmap;
	StdRect rect;

	memset(&bitmap, 0, sizeof(bitmap));
	memset(&rect, 0, sizeof(rect));

	if (!_pVideo) {
		warning("SmushPlay::updateMovie(): the video callbacks are not set");
		*pFinished = 1;
		return 1;
	}

	int state = _pAnimPlay->anmRender(this);

	if (!_pVideo->lockMovieBuffer(&bitmap, &rect)) {
		blit(&bitmap, _pInternalBitmap, &rect);
		_pVideo->unlockMovieBuffer(&bitmap, &rect);
	}

	if (state < 0 || state == 4) {
		_pAnimPlay->anmStop();
		_pInternalBitmap = nullptr;

		if (_movieFlags & 0x10) {
			*pFinished = 0;

			int fps = 15;

			if (_movieInfo.frameDelay)
				fps = (_movieInfo.frameDelay / 2 + 1000000) / _movieInfo.frameDelay;

			restartAnim(fps);
			_bMovieIsPlaying = 1;
		} else {
			*pFinished = 1;
			_pDecoder->close();
			_bMovieIsPlaying = 0;
		}
	} else {
		*pFinished = 0;
		_bMovieIsPlaying = 1;
	}

	return 0;
}

int SmushPlay::pauseMovie() {
	Common::StackLock lock(_mutex);

	if (_bMovieIsPlaying)
		_pAnimator->animatorPause(true);

	return 0;
}

int SmushPlay::resumeMovie() {
	Common::StackLock lock(_mutex);

	if (_bMovieIsPlaying)
		_pAnimator->animatorPause(false);

	return 0;
}

int SmushPlay::getMovieTime() {
	Common::StackLock lock(_mutex);

	if (_bInitialized && _bStarted && _bMovieIsPlaying)
		return _pSound->souEngineQueryTime(0);

	return 0;
}

void SmushPlay::setTranslationTable(const uint16 *pTable) {
	_pTranslationTable = pTable;
}

void SmushPlay::getFrameRect(StdRect *pFrameRect, StdRect *pSrcRect) {
	if (pFrameRect)
		*pFrameRect = _frameRect;

	if (pSrcRect)
		*pSrcRect = _srcRect;
}

int SmushPlay::getMovieLength(const char *path) {
	Common::SeekableReadStream *pStream = openFile(path);

	if (!pStream)
		return 0;

	SmushMovieInfo info;

	memset(&info, 0, sizeof(info));
	getInfo(pStream, &info);
	delete pStream;
	pStream = nullptr;

	return (int)((double)info.frames * 66.666664);
}

int SmushPlay::animatorRender(int flags) {
	int state = _pAnimator->animatorRender(flags);

	if (state < 0)
		return state;

	const Graphics::Surface *pSurface = _pDecoder->getFrameSurface();

	_internalBitmap.pPixels = (byte *)pSurface->getPixels();
	_internalBitmap.width = pSurface->w;
	_internalBitmap.height = pSurface->h;
	_internalBitmap.bytesPerPixel = pSurface->format.bytesPerPixel;
	_internalBitmap.field_10 = 0;
	_internalBitmap.pitchInBytes = pSurface->pitch;
	fakeBlit(&_internalBitmap, nullptr);

	return state;
}

void SmushPlay::resetFrameCounter() {
	_pDecoder->resetBlocky16Frames();
}

int SmushPlay::getFrameCounter() {
	return _pDecoder->getBlocky16Frames();
}

Common::SeekableReadStream *SmushPlay::openFile(const char *path) {
	if (!_pHS) {
		warning("SmushPlay::openFile(): the module is not initialized");
		return nullptr;
	}

	Common::SeekableReadStream *pStream = _pHS->openFile(path);

	if (!pStream)
		return nullptr;

	return Common::wrapCompressedReadStream(pStream);
}

int SmushPlay::getInfo(Common::SeekableReadStream *pStream, SmushMovieInfo *pInfo) {
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

int SmushPlay::openAnim(const char *path, int fps) {
	Common::SeekableReadStream *pStream = openFile(path);

	if (!pStream)
		return 1;

	if (!_pDecoder->loadStream(pStream))
		return 1;

	_pDecoder->start();

	if (_pAnimPlay->anmStart(fps)) {
		_pDecoder->close();
		return 1;
	}

	_pAnimator->animatorNewAnim();

	return 0;
}

void SmushPlay::restartAnim(int fps) {
	_pDecoder->rewind();
	_pDecoder->start();
	_pAnimPlay->anmStart(fps);
	_pAnimator->animatorNewAnim();
}

void SmushPlay::closeAnim() {
	_pAnimPlay->anmStop();
	_pDecoder->close();
}

int SmushPlay::fakeBlit(LecBitmap *pBitmap, const uint16 *pTable) {
	_pInternalBitmap = pBitmap;

	if (!_pTranslationTable)
		_pTranslationTable = pTable;

	return 0;
}

int SmushPlay::blit(LecBitmap *pDest, LecBitmap *pSource, StdRect *pRect) {
	if (!pDest || !pSource)
		return 0;

	if (_movieFlags & 1) {
		_movieFlags &= ~6;

		if (_frameRect.width <= pRect->width / 2)
			_movieFlags |= 4;
		else if (_frameRect.height < pRect->height)
			_movieFlags |= 2;
	}

	int x = _frameRect.x;
	int y = _frameRect.y;
	int rowScale = ((_movieFlags & 6) != 0) + 1;
	int columnScale = ((_movieFlags & 4) != 0) + 1;
	int width = _frameRect.width * columnScale;
	int height = rowScale * _frameRect.height;

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

	int rows = height / rowScale;
	int rowBytes = pSource->bytesPerPixel * (width / columnScale);

	if (rows < 0 || rowBytes < 0) {
		warning("SmushPlay::blit(): the movie is outside of the target");
		return 0;
	}

	byte *pDestPixels = pDest->pPixels + pDest->pitchInBytes * rowScale * y + columnScale * x * pDest->bytesPerPixel;

	if (_movieFlags & 4)
		blitDouble(pSource->pPixels, pSource->pitchInBytes, pDestPixels, pDest->pitchInBytes, rowBytes, rows);
	else if (_movieFlags & 2)
		blitDoubleRows(pSource->pPixels, pSource->pitchInBytes, pDestPixels, pDest->pitchInBytes, rowBytes, rows);
	else
		blitCopy(pSource->pPixels, pSource->pitchInBytes, pDestPixels, pDest->pitchInBytes, rowBytes, rows);

	return 0;
}

void SmushPlay::blitDouble(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows) {
	if (rows == 0)
		return;

	for (int row = 0; row < rows; row++) {
		const byte *pSourceEnd = pSource + rowBytes;

		do {
			uint32 value = READ_LE_UINT32(pSource);
			uint32 low = (value & 0xFF) | ((value << 8) & 0xFF0000);
			uint32 high = (value >> 8) & 0xFF00;
			uint32 top = value & 0xFF000000;
			uint32 first = low | (low << 8);
			uint32 second = ((high | top) >> 8) | high | top;

			WRITE_LE_UINT32(pDest, first);
			WRITE_LE_UINT32(pDest + destPitch, first);
			WRITE_LE_UINT32(pDest + 4, second);
			WRITE_LE_UINT32(pDest + destPitch + 4, second);
			pSource += 4;
			pDest += 8;
		} while (pSource < pSourceEnd);

		pSource += sourcePitch - rowBytes;
		pDest += 2 * destPitch - 2 * rowBytes;
	}
}

void SmushPlay::blitDoubleRows(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows) {
	int numDwords = rowBytes >> 2;

	for (int row = 0; row < rows; row++) {
		for (int i = 0; i < numDwords; i++) {
			uint32 value = READ_UINT32(pSource + 4 * i);

			WRITE_UINT32(pDest + 4 * i, value);
			WRITE_UINT32(pDest + destPitch + 4 * i, value);
		}

		pSource += sourcePitch;
		pDest += 2 * destPitch;
	}
}

void SmushPlay::blitCopy(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows) {
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

} // End of namespace Grim
