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
#include "common/substream.h"
#include "common/textconsole.h"

#include "audio/mixer.h"

#include "graphics/surface.h"

#ifdef USE_BINK
#include "video/bink_decoder.h"
#endif

#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/movie.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/textevent.h"

namespace Grim {

static const byte kXorKey = 0xD2;
static const int kSmushHeaderSize = 16;
static const int kSmushBufferSize = 32;
static const int kSubtitleLineSize = 400;
static const int kVoiceCategory = 2;
static const int kStandardWidth = 640;
static const int kMaxClearedFrames = 2;
static const int kSubtitleX = 320;
static const int kSubtitleY = 468;
static const byte kSubtitleJustify = 10;
static const int kEventsMask = 3;

Movie *Movie::current = nullptr;
int Movie::firstFrame = 1;
CDrawContext *Movie::drawContext = nullptr;

Movie::Movie(const char *name, bool newSubtitles, bool newFlag6194, bool newFlag6195, int frame) : EventHandler(0) {
	Sounds::lowerMasterGainForMovie();

	fileName = name;

	size_t extension = fileName.rfind('.');

	if (extension != LString::npos) {
		fileName.erase(extension);
		fileName += ".m4b";
	}

	typeMask = 0;
	finished = false;
	frameReady = false;
	memset(subtitles, 0, sizeof(subtitles));
	numSubtitles = 0;
	subtitleIndex = 0;
	subtitleEvent = nullptr;
	i9nBundleOpen = CBundleServices::isI9nBundleOpen();
	subtitlesOn = newSubtitles;
	flag6194 = newFlag6194;
	flag6195 = newFlag6195;
	firstFrame = frame;
	// The disasm draws on the surface directly only on its full screen Direct3D path, the one reproduced here
	useDirectDraw = !subtitlesOn && !flag6194;
	clearCount = 0;
	bink = nullptr;
	frameSurface = nullptr;
	paused = false;
}

Movie::~Movie() {
	if (subtitleEvent) {
		subtitleEvent->flags &= ~1u;
		subtitleEvent = nullptr;
	}

#ifdef USE_BINK
	delete bink;
#endif

	bink = nullptr;
	frameSurface = nullptr;

	Sounds::restoreMasterGain();
}

bool Movie::handleEvent(const Event *event) {
	if (event->type != Event::kKeyUp || event->vk != 27)
		return false;

	finished = true;

	return true;
}

void Movie::pause() {
#ifdef USE_BINK
	if (bink && !paused) {
		bink->pauseVideo(true);
		paused = true;
	}
#endif

	typeMask = 0;
}

void Movie::resume() {
#ifdef USE_BINK
	if (bink && paused) {
		bink->pauseVideo(false);
		paused = false;
	}
#endif

	typeMask = kEventsMask;
}

bool Movie::update() {
#ifdef USE_BINK
	if (!bink) {
		warning("Movie::update(): the movie is not open");

		return true;
	}

	// The frame number of the disasm does not move until the frame is presented
	if (frameReady || !bink->needsUpdate())
		return finished;

	frameSurface = bink->decodeNextFrame();

	if (!frameSurface) {
		warning("Movie::update(): cannot decode the next frame");
		finished = true;

		return true;
	}

	frameReady = true;

	return finished;
#else
	return true;
#endif
}

void Movie::presentFrame() {
	if (!frameReady)
		return;

	frameReady = false;
	drawFrame();

#ifdef USE_BINK
	if (!bink)
		return;

	int frameNumber = bink->getCurFrame() + 1;

	if (subtitlesOn)
		updateSubtitle(frameNumber);

	if (frameNumber == (int)bink->getFrameCount())
		finished = true;
#endif
}

uint Movie::readHeader() {
	LFile *file = gpFileServices->open(fileName.c_str(), "rb");

	if (!file)
		return 0;

	uint size = 0;
	int count = 0;
	char header[kSmushBufferSize];

	memset(header, 0, sizeof(header));
	file->read(header, kSmushHeaderSize);

	if (!strncmp(header, "SMUSH", 5)) {
		xorDecryptD2((byte *)header, kSmushHeaderSize);
		strtok(header, "\n");

		char *sizeText = strtok(nullptr, "\n");
		int value = 0;
		int parsed = 0;

		if (sizeText) {
			sizeText[4] = 0;
			parsed = sscanf(sizeText, "%d", &value);
		} else {
			warning("Movie::readHeader(): the header has no size");
		}

		if (value < 0) {
			warning("Movie::readHeader(): the header size is negative");
			value = 0;
		}

		if (parsed != 0) {
			size = value;

			if (subtitlesOn) {
				file->seek(0, 0);

				char *data = new char[size + 1];

				memset(data, 0, size + 1);
				file->read(data, size);
				xorDecryptD2((byte *)data, size);

				char *line = strtok(data, "\n");

				while (line) {
					if (!strncmp(line, "BEGINDATA", 9))
						break;

					line = strtok(nullptr, "\n");
				}

				line = strtok(nullptr, "\n");

				while (line) {
					if (!strcmp(line, "ENDOFDATA"))
						break;

					if (count >= kMaxSubtitles)
						break;

					// The disasm reads the key without a size limit
					if (sscanf(line, "%d\t%d\t%15s\t", &subtitles[count].start, &subtitles[count].end, subtitles[count].text) != 3)
						break;

					count++;
					line = strtok(nullptr, "\n");
				}

				delete[] data;
			}
		}
	}

	gpFileServices->close(file, true);
	numSubtitles = count;

	return size;
}

bool Movie::loadSubtitles() {
	LString path = fileName;
	size_t extension = path.rfind('.');

	if (extension == LString::npos)
		return false;

	path.erase(extension);
	path += ".sub";

	LFile *file = gpFileServices->open(path.c_str(), "r");

	if (!file)
		return false;

	int count = 0;
	char line[kSubtitleLineSize];

	if (file->getString(line, kSubtitleLineSize)) {
		while (count < kMaxSubtitles) {
			if (line[0] != '#') {
				// The disasm reads the key without a size limit
				if (sscanf(line, "%d\t%d\t%15s\t", &subtitles[count].start, &subtitles[count].end, subtitles[count].text) != 3) {
					gpFileServices->close(file, true);

					return false;
				}

				count++;
			}

			if (!file->getString(line, kSubtitleLineSize))
				break;
		}
	}

	numSubtitles = count;
	gpFileServices->close(file, true);

	return true;
}

void Movie::updateSubtitle(int frame) {
	if (subtitleIndex == numSubtitles)
		return;

	SubtitleEntry *entry = &subtitles[subtitleIndex];

	if (frame < entry->start)
		return;

	if (frame > entry->end) {
		if (subtitleEvent) {
			subtitleEvent->flags &= ~1u;
			subtitleEvent = nullptr;
		}

		subtitleIndex++;

		return;
	}

	if (subtitleEvent)
		return;

	const char *translated = TextEvent::translate(entry->text, nullptr);
	LString text;

	if (translated)
		text = translated;
	else
		warning("Movie::updateSubtitle(): no text for the subtitle");

	subtitleEvent = new TextEvent(text);

	if (i9nBundleOpen)
		subtitleEvent->setFont("subtitle.laf");

	subtitleEvent->x = kSubtitleX;
	subtitleEvent->y = kSubtitleY;
	subtitleEvent->geometryReady = false;
	subtitleEvent->justify = kSubtitleJustify;
	subtitleEvent->geometryReady = false;
	subtitleEvent->activate(0.0f, 1);
}

bool Movie::open() {
#ifdef USE_BINK
	if (!drawContext) {
		warning("Movie::open(): the movies are not started");

		return false;
	}

	size_t slash = fileName.rfind('\\');

	if (slash != LString::npos) {
		LString movieName = LString(".\\Movies\\") + fileName.substr(slash + 1);

		if (fileExists(movieName.c_str()))
			fileName = movieName;
	}

	uint header = readHeader();

	if (subtitlesOn && !header && !loadSubtitles())
		subtitlesOn = false;

	LFile *file = gpFileServices->open(fileName.c_str(), "rb");

	if (!file)
		return false;

	Common::SeekableReadStream *stream = file->readStream;

	file->readStream = nullptr;
	gpFileServices->close(file, true);

	if (!stream || header > stream->size()) {
		warning("Movie::open(): the movie header is bigger than the file");
		delete stream;

		return false;
	}

	Common::SeekableReadStream *binkStream = new Common::SeekableSubReadStream(stream, header, stream->size(), DisposeAfterUse::YES);

	bink = new Video::BinkDecoder();

	if (!bink->loadStream(binkStream)) {
		delete bink;
		bink = nullptr;

		return false;
	}

	if (!bink->setOutputPixelFormat(drawContext->getMovieFormat()))
		warning("Movie::open(): the decoder cannot use the format of the draw context");

	float gain = Sounds::soundVolumeToGain(Sounds::getCategoryVolume(kVoiceCategory));

	// The disasm turns NaN into 0
	if (gain > 1.0f)
		gain = 1.0f;
	else if (!(gain >= 0.0f))
		gain = 0.0f;

	bink->setVolume((byte)(gain * 255.0f + 0.5f));
	bink->setSoundType(Audio::Mixer::kSpeechSoundType);

	clearCount = 0;
	typeMask = kEventsMask;
	finished = false;
	frameReady = false;

	if (firstFrame != 1) {
		if (firstFrame < 1 || firstFrame > (int)bink->getFrameCount())
			warning("Movie::open(): the first frame %d is not in the movie", firstFrame);
		else if (!bink->seekToFrame(firstFrame - 1))
			warning("Movie::open(): cannot go to the frame %d", firstFrame);
	}

	bink->start();

	return true;
#else
	warning("Movie::open(): the Bink decoder is not built in");

	return false;
#endif
}

void Movie::drawFrame() {
	if (!drawContext || !frameSurface) {
		warning("Movie::drawFrame(): nothing to draw");

		return;
	}

	if (!useDirectDraw && (flag6194 || frameSurface->w != kStandardWidth)) {
		bool clearScreen = subtitlesOn;

		if (!clearScreen) {
			clearScreen = clearCount < kMaxClearedFrames;
			clearCount++;
		}

		if (clearScreen)
			drawContext->clear(true, false);
	}

	if (!drawContext->beginScene())
		return;

	drawContext->drawMovieFrame(*frameSurface);

	if (!useDirectDraw)
		TextEvent::update(drawContext, true);

	drawContext->endScene();
	drawContext->present(true);
}

bool Movie::startMovie(const char *fileName, bool subtitles, bool flag6194, bool flag6195, int frame) {
	if (current) {
		warning("Movie::startMovie(): a movie is already playing");
		stopMovie();
	}

	current = new Movie(fileName, subtitles, flag6194, flag6195, frame);

	if (!current->open()) {
		stopMovie();

		return false;
	}

	return true;
}

void Movie::stopMovie() {
	if (!current)
		return;

	pauseCurrent();
	delete current;
	current = nullptr;
}

bool Movie::updateMovie() {
	if (!current) {
		warning("Movie::updateMovie(): no movie is playing");

		return true;
	}

	bool movieFinished = current->update();

	if (movieFinished)
		stopMovie();

	return movieFinished;
}

void Movie::presentCurrent() {
	if (current)
		current->presentFrame();
}

void Movie::pauseCurrent() {
	if (current)
		current->pause();
}

void Movie::resumeCurrent() {
	if (current)
		current->resume();
}

void Movie::skipCurrent() {
	if (current)
		current->finished = true;
}

void Movie::startupMovies(CDrawContext *newDrawContext) {
	drawContext = newDrawContext;
}

void Movie::releaseSubtitleFont() {
	// The subtitles are text events, there is no font to release
}

bool Movie::fileExists(const char *path) {
	LFile *file = gpFileServices->open(path, "rb");

	if (!file)
		return false;

	gpFileServices->close(file, true);

	return true;
}

void Movie::xorDecryptD2(byte *data, int size) {
	for (int i = 0; i < size; i++)
		data[i] ^= kXorKey;
}

} // End of namespace Grim
