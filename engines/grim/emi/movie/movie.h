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

#ifndef GRIM_EMI_MOVIE_H
#define GRIM_EMI_MOVIE_H

#include "common/scummsys.h"

#include "engines/grim/emi/input/eventhandler.h"
#include "engines/grim/emi/files/lfile.h"

namespace Common {
struct Event;
class ReadStream;
class SeekableReadStream;
class WriteStream;
}

namespace Graphics {
struct Surface;
}

namespace Video {
class BinkDecoder;
}

namespace Grim {

class EmiEngine;
class TextEvent;

class Movie : public EventHandler {
public:
	enum {
		kMaxSubtitles = 256,
		kSubtitleKeySize = 16
	};

	struct SubtitleEntry {
		int start;
		int end;
		char text[kSubtitleKeySize];
	};

	Movie(EmiEngine *engine, const char *name, bool subtitles, bool newFlag6194, bool newFlag6195, int frame);
	~Movie() override;

	bool handleEvent(const Event *event) override;
	void pause();
	void resume();
	bool update();
	void presentFrame();
	uint readHeader();
	bool loadSubtitles();
	void updateSubtitle(int frame);
	bool open();
	void drawFrame();

	static bool startMovie(EmiEngine *engine, const char *fileName, bool subtitles, bool flag6194, bool flag6195, int frame);
	static void stopMovie(EmiEngine *engine);
	static bool updateMovie(EmiEngine *engine);
	static void presentCurrent(EmiEngine *engine);
	static void pauseCurrent(EmiEngine *engine);
	static void resumeCurrent(EmiEngine *engine);
	static void skipCurrent(EmiEngine *engine);
	static void startupMovies(EmiEngine *engine);
	static void releaseSubtitleFont();
	static bool fileExists(EmiEngine *engine, const char *path);
	static void xorDecryptD2(byte *data, int size);
	static void requestMoviePs2(EmiEngine *engine, const char *name);
	static bool isMovieRequestedPs2(EmiEngine *engine);
	static bool playRequestedMoviePs2(EmiEngine *engine);
	static bool playMoviePs2(EmiEngine *engine, const char *name);
	static Common::SeekableReadStream *openMoviePs2(EmiEngine *engine, const LString &path);
	static void drawFramePs2(EmiEngine *engine, const Graphics::Surface &frame, Graphics::Surface &screen);
	static bool isSkipEventPs2(EmiEngine *engine, const Common::Event &event);
	static int writePs2State(void *data, int bytes);
	static int readPs2State(void *data, int bytes);
	static void loadLoadingIconPs2(EmiEngine *engine);
	static void releaseLoadingIconPs2(EmiEngine *engine);
	static bool drawLoadingIconPs2(EmiEngine *engine);

	LString fileName;
	bool finished;
	bool frameReady;
	SubtitleEntry subtitles[kMaxSubtitles];
	int numSubtitles;
	int subtitleIndex;
	TextEvent *subtitleEvent;
	bool i9nBundleOpen;
	bool subtitlesOn;
	bool flag6194;
	bool flag6195;
	bool useDirectDraw;
	int clearCount;
	Video::BinkDecoder *bink;
	const Graphics::Surface *frameSurface;
	bool paused;
};

} // End of namespace Grim

#endif
