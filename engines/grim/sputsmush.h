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

#ifndef GRIM_SPUTSMUSH_H
#define GRIM_SPUTSMUSH_H

#include "common/scummsys.h"

#include "engines/grim/movie/smush_play.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class Res;
class SputRender;
class WuRegistry;

class SputSmush : public SmushPlayVideo {
public:
	SputSmush(Res *pRes, StdDisplay *pStdDisplay, SputRender *pSputRender, StdVBuffer *pMainGfxBuffer, WuRegistry *pWuRegistry);
	~SputSmush();

	int getMovieTime();
	int initialize(SysStruct *pHS);
	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);
	int startup();
	int shutdown();
	int terminate();
	int periodic();
	int setTarget(StdVBuffer *pBuffer);
	int startMovie(const char *name, int bLooping, int x, int y);
	int stopMovie();
	int getMovieLength(const char *name);
	int startVideo(int width, int height, int depth, int pixelFormat) override;
	int stopVideo() override;
	int lockMovieBuffer(LecBitmap *pBitmap, StdRect *pRect) override;
	int unlockMovieBuffer(LecBitmap *pBitmap, StdRect *pRect) override;
	int setVideoPalette(uint32 *pPalette, int start, int count) override;
	int moviePlaying();
	int pause(int bPause);

private:
	SmushPlay *_pSmushPlay;
	Res *_pRes;
	StdDisplay *_pStdDisplay;
	SputRender *_pSputRender;
	StdVBuffer *_pMainGfxBuffer;
	WuRegistry *_pWuRegistry;
	SysStruct *_pHS;
	int _bStarted;
	StdVBuffer *_pPresentBuf;
	int _buf;
	int _playing;
	int _paused;
	int _abort;
	char _curMovieFile[128];
	int _looping;
	int _x;
	int _y;
	uint _startTime;
	uint _lastPeriodicTime;
};

} // End of namespace Grim

#endif
