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

SputSmushDemo::SputSmushDemo(Res *res, StdDisplay *stdDisplay, SputRender *sputRender, StdVBuffer *mainGfxBuffer, WuRegistry *wuRegistry) :
		SputSmush(res, stdDisplay, sputRender, mainGfxBuffer, wuRegistry), _grim(nullptr), _movieBuffer(nullptr), _demoPlaying(0),
		_demoPaused(0), _finished(0) {
	memset(_paletteTable, 0, sizeof(_paletteTable));
	memset(_moviePalette, 0, sizeof(_moviePalette));

	_driver = new SmushDemoDriver();
	_player = new SmushDemoPlayer(this, _driver);
}

SputSmushDemo::~SputSmushDemo() {
	if (_demoPlaying)
		_player->smushPlayStopMovie();

	delete _player;
	_player = nullptr;
	delete _driver;
	_driver = nullptr;
}

int SputSmushDemo::getMovieTime() {
	return 0;
}

int SputSmushDemo::initialize(SysStruct *hs) {
	_hs = hs;
	_presentBuf = nullptr;
	_movieBuffer = nullptr;
	_buf = 1;

	return 0;
}

int SputSmushDemo::startup() {
	_started = 1;

	return 0;
}

int SputSmushDemo::shutdown() {
	return 0;
}

int SputSmushDemo::terminate() {
	return 0;
}

int SputSmushDemo::periodic() {
	if (!_demoPlaying || _demoPaused)
		return 0;

	int finished = 0;

	if (_player->smushPlayUpdateMovie(&finished))
		return 1;

	blitFrame();

	if (finished || _finished) {
		const byte *pixels;
		StdRect rect;

		_demoPlaying = 0;
		_player->smushPlayStopMovie();
		_player->getFrameBuffer(&pixels, &rect.width, &rect.height);
		_player->getSize(&rect.x, &rect.y);
		_sputRender->markAllPagesDirty(&rect, 0);

		return 1;
	}

	return 0;
}

int SputSmushDemo::setTarget(StdVBuffer *buffer) {
	_presentBuf = buffer;

	return 0;
}

int SputSmushDemo::startMovie(const char *name, int looping, int x, int y) {
	char path[128];

	g_grim->getImuse()->soundsFloodMusicBuffer();

	// Not finding the file is not a failure for the caller, as in the EXE
	if (_res->findExternalFile(name, path, sizeof(path)) != 1)
		return 0;

	if (!_presentBuf) {
		warning("SputSmushDemo::startMovie(): the target is not set");
		return 1;
	}

	if (_demoPlaying) {
		_player->smushPlayStopMovie();
		_demoPlaying = 0;
	}

	if (_player->smushPlayStartMovie(path, 0, 1000000, _presentBuf->tex.width, _presentBuf->tex.height, 0)) {
		if (_movieBuffer)
			_stdDisplay->vBufferFree(_movieBuffer);

		_movieBuffer = nullptr;

		return 1;
	}

	_demoPaused = 0;
	_finished = 0;
	_demoPlaying = 1;

	return 0;
}

int SputSmushDemo::stopMovie() {
	_finished = 1;

	return 1;
}

int SputSmushDemo::getMovieLength(const char *name) {
	return 0;
}

int SputSmushDemo::moviePlaying() {
	return _demoPlaying;
}

int SputSmushDemo::pause(int pause) {
	_demoPaused = pause;

	if (pause)
		_player->smushPlayPauseMovie();
	else
		_player->smushPlayResumeMovie();

	return 0;
}

int SputSmushDemo::videoStart(int width, int height, int depth) {
	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.width = width;
	format.height = height;
	format.format.colorMode = 0;
	format.format.bpp = 8;

	_movieBuffer = _stdDisplay->vBufferNew(&format, 0, 0, _moviePalette);

	return _movieBuffer == nullptr;
}

void SputSmushDemo::videoStop() {
	if (_movieBuffer) {
		if (_grim->getLibMode() == 3) {
			setTarget(_sputRender->drawBuffer());
			blitFrame();
		}

		_stdDisplay->vBufferFree(_movieBuffer);
	}

	_movieBuffer = nullptr;
}

void SputSmushDemo::videoLock(SmushDemoTarget *target, SmushDemoRect *rect) {
	target->pixels = _movieBuffer->pixels;
	target->width = _movieBuffer->tex.width;
	target->height = _movieBuffer->tex.height;
	target->bytesPerPixel = 1;

	rect->x = 0;
	rect->y = 0;
	rect->width = _presentBuf->tex.width;
	rect->height = _presentBuf->tex.height;
}

void SputSmushDemo::videoUnlock() {
}

void SputSmushDemo::videoSetPalette(const uint32 *palette, int start, int count) {
	for (int i = start; i < start + count; i++) {
		uint32 color = palette[i];
		uint16 red = (color & 0xF8) << 8;
		uint16 green = (color >> 5) & 0x7E0;
		uint16 blue = (color >> 19) & 0x1F;

		_paletteTable[i] = red | green | blue;
	}

	_sputRender->colorCorrect(&_paletteTable[start], count);
}

void SputSmushDemo::blitFrame() {
	const byte *framePixels;
	int width;
	int height;
	int x;
	int y;

	_player->getFrameBuffer(&framePixels, &width, &height);
	_player->getSize(&x, &y);

	if (!_movieBuffer || !_presentBuf)
		return;

	if (x < 0 || y < 0 || x + width > _presentBuf->tex.width || y + height > _presentBuf->tex.height) {
		warning("SputSmushDemo::blitFrame(): the frame does not fit in the target");
		return;
	}

	_stdDisplay->vBufferLock(_movieBuffer);
	_stdDisplay->vBufferLock(_presentBuf);

	const byte *source = _movieBuffer->pixels;
	byte *destination = _presentBuf->pixels + y * _presentBuf->tex.pitchInBytes + 2 * x;

	for (int row = 0; row < height; row++) {
		uint16 *rowPointer = reinterpret_cast<uint16 *>(destination);

		for (int column = 0; column < width; column++)
			rowPointer[column] = _paletteTable[source[column]];

		source += _movieBuffer->tex.pitchInBytes;
		destination += _presentBuf->tex.pitchInBytes;
	}

	_stdDisplay->vBufferUnlock(_movieBuffer);
	_stdDisplay->vBufferUnlock(_presentBuf);
}

} // End of namespace Grim
