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

#include "common/archive.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/substream.h"
#include "common/textconsole.h"

#include "graphics/surface.h"

// LEGACY: #include "engines/grim/grim.h"
// LEGACY: #include "engines/grim/localize.h"
// LEGACY: #include "engines/grim/textobject.h"
// LEGACY: #include "engines/grim/textsplit.h"
#include "engines/grim/movie/bink.h"

#include "video/bink_decoder.h"

#ifdef USE_BINK

namespace Grim {

MoviePlayer *CreateBinkPlayer(bool demo) {
	return new BinkPlayer(demo);
}

BinkPlayer::BinkPlayer(bool demo) : MoviePlayer(), _demo(demo) {
	_videoDecoder = new Video::BinkDecoder();
	_subtitleIndex = _subtitles.begin();
}

bool BinkPlayer::bikCheck(Common::SeekableReadStream *stream, uint32 pos) {
	stream->seek(pos);
	uint32 tag = stream->readUint32BE();
	return (tag & 0xFFFFFF00) == MKTAG('B', 'I', 'K', 0);
}

void BinkPlayer::deinit() {
	// LEGACY: g_grim->setMovieSubtitle(nullptr);
	MoviePlayer::deinit();
}

void BinkPlayer::handleFrame() {
	MoviePlayer::handleFrame();

	if (!_showSubtitles || _subtitleIndex == _subtitles.end())
		return;

	unsigned int startFrame, endFrame, curFrame;
	startFrame = _subtitleIndex->_startFrame;
	endFrame = _subtitleIndex->_endFrame;
	curFrame = _videoDecoder->getCurFrame();
	if (startFrame <= curFrame && curFrame <= endFrame) {
		if (!_subtitleIndex->active) {
			// LEGACY: TextObject *textObject = new TextObject();
			// LEGACY: textObject->setDefaults(&g_grim->_sayLineDefaults);
			// LEGACY: Color c(255, 255, 255);
			// LEGACY: textObject->setFGColor(c);
			// LEGACY: textObject->setIsSpeech();
			// LEGACY: if (g_grim->getMode() == GrimEngine::SmushMode) {
			// LEGACY: 	// TODO: How to center exactly and put the text exactly
			// LEGACY: 	// at the bottom  even if there are multiple lines?
			// LEGACY: 	textObject->setX(640 / 2);
			// LEGACY: 	textObject->setY(40);
			// LEGACY: }
			// LEGACY: textObject->setText(g_localizer->localize(_subtitleIndex->_textId.c_str()), false);
			// LEGACY: g_grim->setMovieSubtitle(textObject);
			_subtitleIndex->active = true;
		}
	} else if (endFrame < curFrame) {
		if (_subtitleIndex->active) {
			// LEGACY: g_grim->setMovieSubtitle(nullptr);
			_subtitleIndex->active = false;
			_subtitleIndex++;
		}
	}
}

bool BinkPlayer::loadFile(const Common::String &filename) {
	_fname = filename;

	if (_demo) {
		// LEGACY: Common::String subname = filename + ".sub";
		// The demo uses a .lab suffix
		_fname = filename + ".lab";
		bool ret = MoviePlayer::loadFile(_fname);

		// Load subtitles from adjacent .sub file, if present
		// LEGACY: Common::SeekableReadStream *substream = SearchMan.createReadStreamForMember(Common::Path(subname));
		// LEGACY: if (substream) {
		// LEGACY: 	TextSplitter tsSub("", substream);
		// LEGACY: 	while (!tsSub.isEof()) {
		// LEGACY: 		unsigned int start, end;
		// LEGACY: 		char textId[256];
		// LEGACY:
		// LEGACY: 		// extract single subtitle entry
		// LEGACY: 		tsSub.scanString("%d\t%d\t%s", 3, &start, &end, textId);
		// LEGACY:
		// LEGACY: 		Subtitle st(start, end, textId);
		// LEGACY: 		_subtitles.push_back(st);
		// LEGACY: 	}
		// LEGACY: 	delete substream;
		// LEGACY: 	_subtitleIndex = _subtitles.begin();
		// LEGACY: }

		return ret;
	}

	_fname += ".m4b";

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(Common::Path(_fname));
	if (!stream) {
		warning("BinkPlayer::loadFile(): Can't create stream for: %s", _fname.c_str());
		return false;
	}

	// set the default start of the bink video in case there is no SMUSH header
	uint32 startBinkPos = 0x0;

	// clear existing subtitles
	_subtitles.clear();

	char header[6];
	// read the first 5 bytes of the header
	stream->read(header, 5);
	header[5] = 0;

	// LEGACY: if (!strcmp(header, "SMUSH")) {
	// LEGACY: 	// handle SMUSH header
	// LEGACY: 	unsigned char smushHeader[0x2000];
	// LEGACY:
	// LEGACY: 	// read the first part
	// LEGACY: 	uint32 consumed = 16;
	// LEGACY: 	stream->read(smushHeader, consumed);
	// LEGACY:
	// LEGACY: 	// decode the first part
	// LEGACY: 	for (unsigned int i = 0; i < consumed; i++) {
	// LEGACY: 		smushHeader[i] ^= 0xd2;
	// LEGACY: 	}
	// LEGACY:
	// LEGACY: 	Common::MemoryReadStream msStart(smushHeader, consumed);
	// LEGACY: 	TextSplitter tsStart("", &msStart);
	// LEGACY:
	// LEGACY: 	// extract the length / the start of the following BINK header
	// LEGACY: 	tsStart.scanString("%d", 1, &startBinkPos);
	// LEGACY:
	// LEGACY: 	assert(startBinkPos < sizeof(smushHeader));
	// LEGACY:
	// LEGACY: 	// read the rest (5 bytes less because of the string "SMUSH" at the beginning)
	// LEGACY: 	stream->read(smushHeader+consumed, startBinkPos - consumed - 5);
	// LEGACY:
	// LEGACY: 	// decode the reset
	// LEGACY: 	for (unsigned int i = consumed; i < startBinkPos - 5; i++) {
	// LEGACY: 		smushHeader[i] ^= 0xd2;
	// LEGACY: 	}
	// LEGACY: 	consumed = startBinkPos - 5;
	// LEGACY:
	// LEGACY: 	Common::MemoryReadStream msSmush(smushHeader, consumed);
	// LEGACY: 	TextSplitter tsSmush("", &msSmush);
	// LEGACY:
	// LEGACY: 	// skip the first line which contains the length
	// LEGACY: 	tsSmush.nextLine();
	// LEGACY:
	// LEGACY: 	tsSmush.expectString("BEGINDATA");
	// LEGACY: 	while (!tsSmush.checkString("ENDOFDATA")) {
	// LEGACY: 		unsigned int start, end;
	// LEGACY: 		char textId[256];
	// LEGACY:
	// LEGACY: 		// extract single subtitle entry
	// LEGACY: 		tsSmush.scanString("%d\t%d\t%s", 3, &start, &end, textId);
	// LEGACY:
	// LEGACY: 		Subtitle st(start, end, textId);
	// LEGACY: 		_subtitles.push_back(st);
	// LEGACY: 	}
	// LEGACY: 	tsSmush.expectString("ENDOFDATA");
	// LEGACY: }

	// set current subtitle index to the first subtitle
	_subtitleIndex = _subtitles.begin();

	if (!bikCheck(stream, startBinkPos)) {
		warning("BinkPlayer::loadFile(): Could not find BINK header for: %s", _fname.c_str());
		delete stream;
		return false;
	}

	Common::SeekableReadStream *bink = nullptr;
	bink = new Common::SeekableSubReadStream(stream, startBinkPos, stream->size(), DisposeAfterUse::YES);
	return _videoDecoder->loadStream(bink);
}

} // end of namespace Grim

#endif // USE_BINK
