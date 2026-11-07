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

#ifndef GRIM_MOVIE_SMUSH_PLAY_H
#define GRIM_MOVIE_SMUSH_PLAY_H

#include "common/mutex.h"
#include "common/scummsys.h"

#include "engines/grim/movie/smush_anim.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class SeekableReadStream;
}

namespace Grim {

class SmushAnimator;
class SmushDecoder;
class SmushDriver;
class SmushSound;
class SmushTimer;
class SmushWave;

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

class SmushPlayVideo {
public:
	virtual ~SmushPlayVideo() {}

	virtual int startVideo(int width, int height, int depth, int pixelFormat) = 0;
	virtual int stopVideo() = 0;
	virtual int lockMovieBuffer(LecBitmap *pBitmap, StdRect *pRect) = 0;
	virtual int unlockMovieBuffer(LecBitmap *pBitmap, StdRect *pRect) = 0;
	virtual int setVideoPalette(uint32 *pPalette, int start, int count) = 0;
};

class SmushPlay : public SmushAnimClient {
public:
	SmushPlay(bool remastered);
	~SmushPlay();

	int sysInitialize(SysStruct *pHS);
	void sysTerminate();
	int sysStartup();
	void sysShutdown();
	void sysSetVideoCallbacks(SmushPlayVideo *pVideo);
	void sysConfigureColorLookup(int config);
	int startMovie(const char *name, int flags, int bufferBytes, int x, int y, int width, int height, int offset, int pixelFormat);
	int stopMovie();
	int updateMovie(int *pFinished);
	int pauseMovie();
	int resumeMovie();
	int getMovieTime();
	void setTranslationTable(const uint16 *pTable);
	void getFrameRect(StdRect *pFrameRect, StdRect *pSrcRect);
	int getMovieLength(const char *path);

private:
	static void timerCallback(void *pContext);

	int animatorRender(int flags) override;
	void resetFrameCounter() override;
	int getFrameCounter() override;

	Common::SeekableReadStream *openFile(const char *path);
	int getInfo(Common::SeekableReadStream *pStream, SmushMovieInfo *pInfo);
	int openAnim(const char *path, int fps);
	void restartAnim(int fps);
	void closeAnim();
	int fakeBlit(LecBitmap *pBitmap, const uint16 *pTable);
	int blit(LecBitmap *pDest, LecBitmap *pSource, StdRect *pRect);
	void blitDouble(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows);
	void blitDoubleRows(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows);
	void blitCopy(const byte *pSource, int sourcePitch, byte *pDest, int destPitch, int rowBytes, int rows);

	SysStruct *_pHS;
	SmushPlayVideo *_pVideo;
	SmushTimer *_pTimer;
	SmushDriver *_pDriver;
	SmushSound *_pSound;
	SmushWave *_pWave;
	SmushDecoder *_pDecoder;
	SmushAnimPlay *_pAnimPlay;
	SmushAnimator *_pAnimator;
	Common::Mutex _mutex;
	int _bInitialized;
	int _bStarted;
	int _bTimerInstalled;
	int _bMovieIsPlaying;
	int _movieFlags;
	SmushMovieInfo _movieInfo;
	char _curFileName[260];
	StdRect _frameRect;
	StdRect _srcRect;
	LecBitmap _internalBitmap;
	LecBitmap *_pInternalBitmap;
	const uint16 *_pTranslationTable;
	int _colorLookupConfig;
};

} // End of namespace Grim

#endif
