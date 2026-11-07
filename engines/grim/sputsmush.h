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

#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class ReadStream;
class SeekableReadStream;
class WriteStream;
}

namespace Graphics {
struct Surface;
}

namespace Grim {

class MoviePlayer;
class Res;
class SputRender;
class WuRegistry;

struct LecBitmap {
	byte *pPixels;
	int width;
	int height;
	int bytesPerPixel;
	int field_10;
	int pitchInBytes;
};

struct SmushMovieInfo {
	byte version0;
	byte version1;
	int16 frames;
	int16 x;
	int16 y;
	int16 width;
	int16 height;
	int16 field_C;
	int32 frameDelay;
	int32 field_12;
	byte palette[1024];
	int16 field_416;
};

class SputSmush {
public:
	SputSmush(MoviePlayer *pMovie, Res *pRes, StdDisplay *pStdDisplay, SputRender *pSputRender, StdVBuffer *pMainGfxBuffer,
			WuRegistry *pWuRegistry);

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
	int startVideo(int width, int height, int depth);
	int stopVideo();
	int lockMovieBuffer(LecBitmap *pBitmap, StdRect *pRect);
	int unlockMovieBuffer();
	int setVideoPalette(uint32 *pPalette, int start, int count);
	int moviePlaying();
	int pause(int bPause);

private:
	int smushPlaySysInitialize();
	void smushPlaySysTerminate();
	int smushPlaySysStartup();
	void smushPlaySysShutdown();
	void smushPlaySysConfigureColorLookup(int config);
	void smushPlaySetTranslationTable(const uint16 *pTable);
	int smushPlayStartMovie(const char *name, int bLooping, int x, int y, int width, int height);
	int smushPlayStopMovie();
	int smushPlayUpdateMovie(int *pFinished);
	int smushPlayBlit(LecBitmap *pDest, const Graphics::Surface *pSource, const StdRect *pRect);
	void smushPlayBlitCopy(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows);
	int smushPlayGetMovieLength(const char *path);
	int smushPlayGetInfo(Common::SeekableReadStream *pStream, SmushMovieInfo *pInfo);

	MoviePlayer *_pMovie;
	Res *_pRes;
	StdDisplay *_pStdDisplay;
	SputRender *_pSputRender;
	StdVBuffer *_pMainGfxBuffer;
	WuRegistry *_pWuRegistry;
	SysStruct *_pHS;
	int _bPlayInitialized;
	int _bPlayStarted;
	const uint16 *_pTranslationTable;
	int _colorLookupConfig;
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
