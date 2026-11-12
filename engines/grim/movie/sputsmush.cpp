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

#include "engines/grim/res.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/movie/sputsmush.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

SputSmush::SputSmush(Res *res, StdDisplay *stdDisplay, SputRender *sputRender, StdVBuffer *mainGfxBuffer, WuRegistry *wuRegistry) :
		_res(res), _stdDisplay(stdDisplay), _sputRender(sputRender), _mainGfxBuffer(mainGfxBuffer),
		_wuRegistry(wuRegistry), _hs(nullptr), _started(0), _presentBuf(nullptr), _buf(0), _playing(0), _paused(0), _abort(0),
		_looping(0), _x(0), _y(0), _startTime(0), _lastPeriodicTime(0) {
	memset(_curMovieFile, 0, sizeof(_curMovieFile));

	_smushPlay = new SmushPlay(false);
}

SputSmush::~SputSmush() {
	delete _smushPlay;
	_smushPlay = nullptr;
}

int SputSmush::getMovieTime() {
	return _smushPlay->getMovieTime();
}

int SputSmush::initialize(SysStruct *hs) {
	_hs = hs;

	_smushPlay->sysInitialize(hs);
	_smushPlay->sysSetVideoCallbacks(this);

	_presentBuf = nullptr;
	_buf = 1;

	return 0;
}

int SputSmush::save(Common::WriteStream *stream) {
	stream->writeSint32LE(_playing);
	stream->writeSint32LE(_paused);
	stream->writeSint32LE(_abort);
	stream->write(_curMovieFile, sizeof(_curMovieFile));
	stream->writeSint32LE(_looping);
	stream->writeSint32LE(_x);
	stream->writeSint32LE(_y);

	if (stream->err()) {
		warning("SputSmush::save(): failed to write the movie state");
		return 1;
	}

	return 0;
}

int SputSmush::restore(Common::ReadStream *stream) {
	setTarget(_mainGfxBuffer);

	_playing = stream->readSint32LE();
	_paused = stream->readSint32LE();
	_abort = stream->readSint32LE();
	stream->read(_curMovieFile, sizeof(_curMovieFile));
	_looping = stream->readSint32LE();
	_x = stream->readSint32LE();
	_y = stream->readSint32LE();

	if (stream->err() || stream->eos()) {
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
	int found = _wuRegistry->getStr("StartSmush", startSmush, sizeof(startSmush), "");

	if (!found || scumm_stricmp(startSmush, "FALSE")) {
		if (_smushPlay->sysStartup())
			return 1;

		_started = 1;
	}

	return 0;
}

int SputSmush::shutdown() {
	if (_started)
		_smushPlay->sysShutdown();

	return 0;
}

int SputSmush::terminate() {
	_smushPlay->sysTerminate();

	return 0;
}

int SputSmush::periodic() {
	int result = 1;
	int finished = 0;

	if (!_started || !_playing)
		return 1;

	if (_paused)
		return 0;

	_smushPlay->setTranslationTable(_sputRender->getColorLookup());

	if (_abort || !_smushPlay->updateMovie(&finished)) {
		if (finished || _abort) {
			_playing = 0;

			StdRect frameRect;

			_smushPlay->stopMovie();
			_smushPlay->getFrameRect(&frameRect, nullptr);
			_sputRender->markAllPagesDirty(&frameRect, 0);
		} else {
			result = 0;
		}
	}

	_smushPlay->sysConfigureColorLookup(0);
	_lastPeriodicTime = g_system->getMillis();

	return result;
}

int SputSmush::setTarget(StdVBuffer *buffer) {
	if (!_started)
		return 1;

	_presentBuf = buffer;

	return 0;
}

int SputSmush::startMovie(const char *name, int looping, int x, int y) {
	if (!_started)
		return 1;

	if (_playing) {
		_smushPlay->stopMovie();
		_playing = 0;
	}

	if (!_presentBuf) {
		warning("SputSmush::startMovie(): the target is not set");
		return 1;
	}

	_smushPlay->setTranslationTable(_sputRender->getColorLookup());

	_looping = looping;
	_x = x;
	_y = y;

	if (_smushPlay->startMovie(name, looping != 0 ? 0x10 : 0, 100000, x, y, _presentBuf->tex.width, _presentBuf->tex.height, 0, 1))
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
	int result = _started;

	if (_started) {
		_abort = 1;
		_paused = 0;
	}

	return result;
}

int SputSmush::getMovieLength(const char *name) {
	char path[128];
	int movieLength = 0;

	if (_res->findExternalFile(name, path, sizeof(path)) == 1)
		movieLength = _smushPlay->getMovieLength(path);

	return movieLength;
}

int SputSmush::startVideo(int width, int height, int depth, int pixelFormat) {
	if (!_started)
		return 1;

	if (!_presentBuf) {
		warning("SputSmush::startVideo(): the target is not set");
		return 1;
	}

	return _presentBuf->tex.width != width || _presentBuf->tex.height != height || _presentBuf->tex.format.bpp != depth;
}

int SputSmush::stopVideo() {
	return _started == 0;
}

int SputSmush::lockMovieBuffer(LecBitmap *bitmap, StdRect *rect) {
	if (!_started)
		return 1;

	if (!_presentBuf) {
		warning("SputSmush::lockMovieBuffer(): the target is not set");
		return 1;
	}

	_stdDisplay->vBufferLock(_presentBuf);

	bitmap->bytesPerPixel = _presentBuf->tex.format.bpp >> 3;
	bitmap->pixels = _presentBuf->pixels;
	bitmap->height = _presentBuf->tex.height;
	bitmap->width = _presentBuf->tex.width;
	bitmap->pitchInBytes = _presentBuf->tex.pitchInBytes;

	rect->y = 0;
	rect->x = 0;
	rect->height = _presentBuf->tex.height;
	rect->width = _presentBuf->tex.width;

	return 0;
}

int SputSmush::unlockMovieBuffer(LecBitmap *bitmap, StdRect *rect) {
	if (!_started)
		return 1;

	if (!_presentBuf) {
		warning("SputSmush::unlockMovieBuffer(): the target is not set");
		return 1;
	}

	_stdDisplay->vBufferUnlock(_presentBuf);

	return 0;
}

int SputSmush::setVideoPalette(uint32 *palette, int start, int count) {
	return _started == 0;
}

int SputSmush::moviePlaying() {
	return _playing;
}

int SputSmush::pause(int pause) {
	_paused = pause;

	if (pause)
		_smushPlay->pauseMovie();
	else
		_smushPlay->resumeMovie();

	return 0;
}

} // End of namespace Grim
