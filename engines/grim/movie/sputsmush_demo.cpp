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

#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/movie/smush_mixer.h"
#include "engines/grim/res.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/movie/sputsmush_demo.h"

namespace Grim {

SputSmushDemo::SputSmushDemo(Res *pRes, StdDisplay *pStdDisplay, SputRender *pSputRender, StdVBuffer *pMainGfxBuffer, WuRegistry *pWuRegistry) :
		SputSmush(pRes, pStdDisplay, pSputRender, pMainGfxBuffer, pWuRegistry), _pGrim(nullptr), _pMovieBuffer(nullptr), _bDemoPlaying(0),
		_bDemoPaused(0), _bFinished(0) {
	memset(_aPaletteTable, 0, sizeof(_aPaletteTable));
	memset(_aMoviePalette, 0, sizeof(_aMoviePalette));

	_pDriver = new SmushDemoDriver();
	_pPlayer = new SmushDemoPlayer(this, _pDriver);
}

SputSmushDemo::~SputSmushDemo() {
	if (_bDemoPlaying)
		_pPlayer->smushPlayStopMovie();

	delete _pPlayer;
	_pPlayer = nullptr;
	delete _pDriver;
	_pDriver = nullptr;
}

int SputSmushDemo::getMovieTime() {
	return 0;
}

int SputSmushDemo::initialize(SysStruct *pHS) {
	_pHS = pHS;
	_pPresentBuf = nullptr;
	_pMovieBuffer = nullptr;
	_buf = 1;

	return 0;
}

int SputSmushDemo::startup() {
	_bStarted = 1;

	return 0;
}

int SputSmushDemo::shutdown() {
	return 0;
}

int SputSmushDemo::terminate() {
	return 0;
}

int SputSmushDemo::periodic() {
	if (!_bDemoPlaying || _bDemoPaused)
		return 0;

	int finished = 0;

	if (_pPlayer->smushPlayUpdateMovie(&finished))
		return 1;

	blitFrame();

	if (finished || _bFinished) {
		const byte *pPixels;
		StdRect rect;

		_bDemoPlaying = 0;
		_pPlayer->smushPlayStopMovie();
		_pPlayer->getFrameBuffer(&pPixels, &rect.width, &rect.height);
		_pPlayer->getSize(&rect.x, &rect.y);
		_pSputRender->markAllPagesDirty(&rect, 0);

		return 1;
	}

	return 0;
}

int SputSmushDemo::setTarget(StdVBuffer *pBuffer) {
	_pPresentBuf = pBuffer;

	return 0;
}

int SputSmushDemo::startMovie(const char *name, int bLooping, int x, int y) {
	char path[128];

	g_grim->getImuse()->soundsFloodMusicBuffer();

	// Not finding the file is not a failure for the caller, as in the EXE
	if (_pRes->findExternalFile(name, path, sizeof(path)) != 1)
		return 0;

	if (!_pPresentBuf) {
		warning("SputSmushDemo::startMovie(): the target is not set");
		return 1;
	}

	if (_bDemoPlaying) {
		_pPlayer->smushPlayStopMovie();
		_bDemoPlaying = 0;
	}

	if (_pPlayer->smushPlayStartMovie(path, 0, 1000000, _pPresentBuf->tex.width, _pPresentBuf->tex.height, 0)) {
		if (_pMovieBuffer)
			_pStdDisplay->vBufferFree(_pMovieBuffer);

		_pMovieBuffer = nullptr;

		return 1;
	}

	_bDemoPaused = 0;
	_bFinished = 0;
	_bDemoPlaying = 1;

	return 0;
}

int SputSmushDemo::stopMovie() {
	_bFinished = 1;

	return 1;
}

int SputSmushDemo::getMovieLength(const char *name) {
	return 0;
}

int SputSmushDemo::moviePlaying() {
	return _bDemoPlaying;
}

int SputSmushDemo::pause(int bPause) {
	_bDemoPaused = bPause;

	if (bPause)
		_pPlayer->smushPlayPauseMovie();
	else
		_pPlayer->smushPlayResumeMovie();

	return 0;
}

int SputSmushDemo::videoStart(int width, int height, int depth) {
	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.width = width;
	format.height = height;
	format.format.colorMode = 0;
	format.format.bpp = 8;

	_pMovieBuffer = _pStdDisplay->vBufferNew(&format, 0, 0, _aMoviePalette);

	return _pMovieBuffer == nullptr;
}

void SputSmushDemo::videoStop() {
	if (_pMovieBuffer) {
		if (_pGrim->getLibMode() == 3) {
			setTarget(_pSputRender->drawBuffer());
			blitFrame();
		}

		_pStdDisplay->vBufferFree(_pMovieBuffer);
	}

	_pMovieBuffer = nullptr;
}

void SputSmushDemo::videoLock(SmushDemoTarget *target, SmushDemoRect *rect) {
	target->pixels = _pMovieBuffer->pPixels;
	target->width = _pMovieBuffer->tex.width;
	target->height = _pMovieBuffer->tex.height;
	target->bytesPerPixel = 1;

	rect->x = 0;
	rect->y = 0;
	rect->width = _pPresentBuf->tex.width;
	rect->height = _pPresentBuf->tex.height;
}

void SputSmushDemo::videoUnlock() {
}

void SputSmushDemo::videoSetPalette(const uint32 *palette, int start, int count) {
	for (int i = start; i < start + count; i++) {
		uint32 color = palette[i];
		uint16 red = (color & 0xF8) << 8;
		uint16 green = (color >> 5) & 0x7E0;
		uint16 blue = (color >> 19) & 0x1F;

		_aPaletteTable[i] = red | green | blue;
	}

	_pSputRender->colorCorrect(&_aPaletteTable[start], count);
}

void SputSmushDemo::blitFrame() {
	const byte *pFramePixels;
	int width;
	int height;
	int x;
	int y;

	_pPlayer->getFrameBuffer(&pFramePixels, &width, &height);
	_pPlayer->getSize(&x, &y);

	if (!_pMovieBuffer || !_pPresentBuf)
		return;

	if (x < 0 || y < 0 || x + width > _pPresentBuf->tex.width || y + height > _pPresentBuf->tex.height) {
		warning("SputSmushDemo::blitFrame(): the frame does not fit in the target");
		return;
	}

	_pStdDisplay->vBufferLock(_pMovieBuffer);
	_pStdDisplay->vBufferLock(_pPresentBuf);

	const byte *pSource = _pMovieBuffer->pPixels;
	byte *pDestination = _pPresentBuf->pPixels + y * _pPresentBuf->tex.pitchInBytes + 2 * x;

	for (int row = 0; row < height; row++) {
		uint16 *pRow = reinterpret_cast<uint16 *>(pDestination);

		for (int column = 0; column < width; column++)
			pRow[column] = _aPaletteTable[pSource[column]];

		pSource += _pMovieBuffer->tex.pitchInBytes;
		pDestination += _pPresentBuf->tex.pitchInBytes;
	}

	_pStdDisplay->vBufferUnlock(_pMovieBuffer);
	_pStdDisplay->vBufferUnlock(_pPresentBuf);
}

} // End of namespace Grim
