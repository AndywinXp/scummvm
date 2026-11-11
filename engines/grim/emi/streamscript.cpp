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
#include "common/config-manager.h"
#include "common/fs.h"
#include "common/path.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/grim/emi/cueentry.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/musicstream.h"
#include "engines/grim/emi/streamscript.h"

namespace Grim {

static const int kLineSize = 1024;
static const int kGameDirectoryDepth = 4;
static const int kCenterPan = 64;

StreamScript *StreamScript::activeScript = nullptr;
int (*StreamScript::streamingGate)() = nullptr;
int16 StreamScript::instanceCount = 0;
uint StreamScript::firstUpdateMs = 0;
uint StreamScript::lastUpdateMs = 0;
float StreamScript::masterVolume = 1.0f;
float StreamScript::currentFadeRate = 1.0f;
float StreamScript::fadeStep = (float)(1.0 / (double)40.0f);
LString StreamScript::searchDirs[StreamScript::kMaxSearchDirs];
int StreamScript::searchDirCount = 0;
int StreamScript::openMissCount = 0;
int StreamScript::findMissCount = 0;
Common::FSDirectory *StreamScript::gameDirectory = nullptr;

StreamScript::StreamScript(const char *scriptFileName) {
	head = nullptr;
	loaded = false;

	instanceCount++;

	if (instanceCount > 1)
		return;

	fileName = scriptFileName;

	Common::SeekableReadStream *script = openAudioFile(scriptFileName);

	if (!script)
		return;

	CueEntry *lastCue = nullptr;
	LString quoted;
	char line[kLineSize];
	char jmmLine[kLineSize];
	char token[kLineSize];
	char typeText[kLineSize];
	typeText[0] = 0;

	int cueId = 0;
	int cueX = 0;
	int cueY = 0;
	int cueSync = 0;
	int ignoredTrim = 0;
	int trim = 0;
	float parsedStart = 0.0f;
	float parsedEnd = 0.0f;
	float parsedJumpSrc = 0.0f;
	float parsedJumpDst = 0.0f;
	int parsedField16 = 0;
	double parsedRatio = 0.0;
	double parsedPcm = 0.0;
	int parsedSrcByte = 0;
	int parsedDstByte = 0;
	int parsedLoopCount = 0;

	while (!script->eos()) {
		line[0] = 0;
		token[0] = 0;

		script->readLine(line, sizeof(line));
		sscanf(line, " %s ", token);

		if (strcmp(token, ".cueButton") == 0) {
			sscanf(line, "%s id %d x %d y %d sync %d type %s trim %d", token, &cueId, &cueX, &cueY, &cueSync, typeText, &ignoredTrim);

			char *quote = strchr(line, '"');

			if (quote) {
				quote++;

				char *closingQuote = strchr(quote, '"');

				if (closingQuote) {
					*closingQuote = 0;
					quoted = quote;
				}
			}

			CueEntry *cue = new CueEntry(quoted.c_str(), &cueId, &cueSync, lastCue, strcmp(typeText, "episode") == 0);

			lastCue = cue;
			head = cue;
		} else if (strcmp(token, ".playFile") == 0) {
			if (!lastCue)
				continue;

			sscanf(line, "%s trim %d", token, &trim);

			char *quote = strchr(line, '"');

			if (!quote)
				continue;

			quote++;

			char *closingQuote = strchr(quote, '"');

			if (closingQuote) {
				*closingQuote = 0;
				quoted = quote;
			}

			if (!lastCue->stream.fileName.empty())
				continue;

			CueStream &cueStream = lastCue->stream;

			cueStream.fileName = findAudioFile(quoted.c_str());
			cueStream.trim = (float)((double)trim * (double)0.0078739999f);

			LString jmmPath = cueStream.fileName;
			uint dotPosition = jmmPath.rfind('.');

			if (dotPosition != LString::npos)
				jmmPath = jmmPath.substr(0, dotPosition);

			jmmPath += ".jmm";

			Common::SeekableReadStream *jmm = openStream(jmmPath.c_str());

			if (!jmm) {
				cueStream.start = 0.0;
				cueStream.jumps[0].srcMs = -1.0;
				cueStream.jumps[0].dstMs = 0.0;
				cueStream.jumps[0].field16 = 0;
				cueStream.hasJump = 1;

				continue;
			}

			while (!jmm->eos()) {
				jmmLine[0] = 0;

				jmm->readLine(jmmLine, sizeof(jmmLine));
				sscanf(jmmLine, " %s ", token);

				if (strcmp(token, ".start") == 0) {
					sscanf(jmmLine, " %s %f", token, &parsedStart);
					cueStream.start = (double)parsedStart;
				} else if (strcmp(token, ".jump") == 0) {
					int assigned = sscanf(jmmLine, " %s %f %f %d %lf %lf %d %d %d", token, &parsedJumpSrc, &parsedJumpDst, &parsedField16, &parsedRatio, &parsedPcm, &parsedSrcByte, &parsedDstByte, &parsedLoopCount);

					// The disasm stores the loop count over the low bytes of the pcm rate
					if (assigned >= 9) {
						uint64 pcmBits;

						memcpy(&pcmBits, &parsedPcm, sizeof(pcmBits));
						pcmBits = (pcmBits & ~(uint64)0xFFFF) | (((uint32)parsedLoopCount >> 16) & 0xFFFF);
						memcpy(&parsedPcm, &pcmBits, sizeof(pcmBits));
					}

					cueStream.jumps[0].srcMs = (double)parsedJumpSrc;
					cueStream.jumps[0].dstMs = (double)parsedJumpDst;
					cueStream.jumps[0].field16 = (int16)parsedField16;
					cueStream.hasJump = 1;

					if (parsedRatio > 0.0) {
						cueStream.ratio = parsedRatio;
						cueStream.invRatio = 1.0 / parsedRatio;
					} else {
						cueStream.ratio = 1.0;
						cueStream.invRatio = 1.0;
					}

					if (parsedPcm > 0.0) {
						cueStream.pcmBytesPerMs = parsedPcm;
						cueStream.invPcmBytesPerMs = 1.0 / parsedPcm;
					} else {
						cueStream.pcmBytesPerMs = 1.0;
						cueStream.invPcmBytesPerMs = 1.0;
					}

					cueStream.startByte = (int)(parsedPcm * cueStream.start + 0.5);
					cueStream.srcByte = parsedSrcByte;
					cueStream.dstByte = parsedDstByte;
					cueStream.loopCount = (int16)parsedLoopCount;

					break;
				} else if (strcmp(token, ".end") == 0) {
					sscanf(jmmLine, " %s %f", token, &parsedEnd);
					cueStream.end = (double)parsedEnd;

					break;
				}
			}

			delete jmm;
		}
	}

	delete script;
	loaded = true;
}

StreamScript::~StreamScript() {
	instanceCount--;

	if (instanceCount > 0)
		return;

	closeStreams();

	while (head) {
		CueEntry *next = head->next;

		delete head;
		head = next;
	}

	loaded = false;
	fileName.clear();
}

void *StreamScript::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void StreamScript::operator delete(void *block) {
	gpMemoryServices->free(block);
}

void StreamScript::closeStreams() {
	if (!isStreamingEnabled())
		return;

	if (!loaded)
		return;

	CueEntry *cue = head;

	while (cue) {
		if (!cue->stream.fileName.empty() && cue->stream.musicStream) {
			cue->stream.musicStream->pause(true);

			delete cue->stream.musicStream;
			cue->stream.musicStream = nullptr;
			cue->state = 0;
		}

		cue = cue->next;
	}
}

bool StreamScript::openStreamScript(const char *scriptFileName) {
	if (activeScript)
		return false;

	StreamScript *script = new StreamScript(scriptFileName);

	activeScript = script;

	if (!script->loaded) {
		delete script;
		activeScript = nullptr;

		return false;
	}

	return true;
}

void StreamScript::closeStreamScript() {
	StreamScript *script = activeScript;

	if (script) {
		delete script;
		activeScript = nullptr;
	}
}

void StreamScript::setStreamScriptVolume(float volume) {
	float clamped = volume;

	if (!(volume >= 0.0f))
		clamped = 0.0f;
	else if (volume > 1.0f)
		clamped = 1.0f;

	if (!isStreamingEnabled())
		return;

	if (!activeScript)
		return;

	masterVolume = clamped;

	CueEntry *cue = activeScript->head;

	while (cue) {
		if (!cue->stream.fileName.empty() && cue->stream.musicStream) {
			int streamVolume = (int)((double)cue->stream.trim * (double)cue->fadeVolume * (double)masterVolume * 127.0 + (double)0.5f);

			cue->stream.musicStream->setVolume(streamVolume);
		}

		cue = cue->next;
	}
}

void StreamScript::startStream(const uint &id, const int &startMs, const bool &immediate) {
	if (!isStreamingEnabled())
		return;

	if (!activeScript)
		return;

	CueEntry *cue = activeScript->head;
	CueEntry *keptCue = nullptr;
	bool otherPlaying = false;
	int otherSync = 0;
	MusicStream *otherStream = nullptr;
	double otherStart = 0.0;
	double otherMsPerByte = 0.0;

	while (cue) {
		if ((uint)cue->id != id && !cue->stream.fileName.empty() && cue->stream.musicStream) {
			CueEntry *closedCue = nullptr;

			otherPlaying = true;

			if (keptCue) {
				closedCue = keptCue;

				if (!(keptCue->fadeVolume >= cue->fadeVolume))
					keptCue = cue;
				else
					closedCue = cue;

				closedCue->fadeVolume = 0.0f;
				closedCue->stream.musicStream->pause(true);

				// The original would use the closed stream for the sync position
				if (otherStream == closedCue->stream.musicStream) {
					warning("StreamScript::startStream(): the stream to sync with has been closed");
					otherStream = nullptr;
				}

				delete closedCue->stream.musicStream;
				closedCue->stream.musicStream = nullptr;
				closedCue->state = 0;
			} else {
				keptCue = cue;
			}

			if (closedCue != cue) {
				cue->state = 2;

				if (cue->sync > 0) {
					otherSync = cue->sync;
					otherStream = cue->stream.musicStream;
					otherStart = cue->stream.start;
					otherMsPerByte = cue->stream.invPcmBytesPerMs * cue->stream.ratio;
				}
			}
		}

		cue = cue->next;
	}

	cue = activeScript->head;

	while (cue) {
		if ((uint)cue->id == id && !cue->stream.fileName.empty()) {
			if (!cue->stream.musicStream) {
				cue->stream.musicStream = MusicStream::open(cue->stream.fileName);

				if (cue->stream.musicStream) {
					MusicStream *musicStream = cue->stream.musicStream;

					currentFadeRate = cue->fadeRate;
					cue->looped = false;
					cue->lastPositionMs = 0;
					cue->ended = false;

					int startPosition = cue->stream.startByte;

					if (otherSync > 0 && otherStream && cue->sync == otherSync) {
						int otherPosition = otherStream->getPosition();
						double offsetMs = (double)otherPosition * otherMsPerByte - otherStart;

						if (!(offsetMs >= 0.0))
							offsetMs = 0.0;

						startPosition += (int)(offsetMs * cue->stream.pcmBytesPerMs + (double)0.05f);
					}

					musicStream->setLoopBlock(cue->stream.dstByte, cue->stream.srcByte);
					musicStream->setLoopCount(cue->stream.loopCount);

					int bytePosition = (int)((double)startPosition * cue->stream.invRatio + 0.5);

					if (startMs >= 0) {
						musicStream->setMsPosition(startMs);
						cue->lastPositionMs = startMs;
					} else {
						musicStream->setPosition(bytePosition);
					}

					if (immediate)
						cue->fadeVolume = 1.0f;
					else if (!otherPlaying && startMs < 0)
						cue->fadeVolume = 1.0f;
					else
						cue->fadeVolume = 0.0f;

					int streamVolume = (int)((double)masterVolume * (double)cue->stream.trim * (double)cue->fadeVolume * 127.0 + (double)0.5f);

					musicStream->setVolume(streamVolume);
					musicStream->setPan(kCenterPan);
					musicStream->start();
				}
			}

			cue->state = 1;
		}

		cue = cue->next;
	}
}

bool StreamScript::consumeStreamFlags(const uint &id, bool &looped, bool &ended) {
	if (!isStreamingEnabled())
		return false;

	if (!activeScript)
		return false;

	looped = false;
	ended = false;

	CueEntry *cue = activeScript->head;

	while (cue) {
		if ((uint)cue->id == id) {
			looped = cue->looped;
			ended = cue->ended;
			cue->looped = false;
			cue->ended = false;

			return true;
		}

		cue = cue->next;
	}

	return false;
}

void StreamScript::updateStreams() {
	uint now = g_system->getMillis();

	if (firstUpdateMs == 0) {
		firstUpdateMs = now;
		lastUpdateMs = now;

		return;
	}

	int delta = (int)(now - lastUpdateMs);

	lastUpdateMs = now;

	float elapsed = (float)((double)delta * (double)0.001f);

	if (!isStreamingEnabled())
		return;

	if (!activeScript)
		return;

	CueEntry *cue = activeScript->head;

	while (cue) {
		MusicStream *musicStream = cue->stream.musicStream;

		if (musicStream) {
			if (!cue->looped) {
				int current = 0;

				musicStream->getMsPosition(nullptr, &current);

				if (current < cue->lastPositionMs)
					cue->looped = true;

				cue->lastPositionMs = current;
			}

			if (!cue->ended) {
				if (musicStream->getStatus() == MusicStream::kStatusDone)
					cue->ended = true;
			}

			if (cue->state != 0) {
				double change = (double)currentFadeRate * (double)elapsed;
				float fade;

				if (cue->state == 1)
					fade = (float)(change + (double)cue->fadeVolume);
				else
					fade = (float)((double)cue->fadeVolume - change);

				cue->fadeVolume = fade;

				if (!(fade >= 0.0f)) {
					cue->fadeVolume = 0.0f;

					musicStream->pause(true);

					delete musicStream;
					cue->stream.musicStream = nullptr;
					cue->state = 0;
				} else {
					if (fade > 1.0f) {
						cue->fadeVolume = 1.0f;
						cue->state = 0;
					}

					int streamVolume = (int)((double)masterVolume * (double)cue->stream.trim * (double)cue->fadeVolume * 127.0 + (double)0.5f);

					musicStream->setVolume(streamVolume);
				}
			}

			// The mixer has no master volume of its own for the streams
			if (cue->stream.musicStream)
				cue->stream.musicStream->refreshMixerVolume();
		}

		cue = cue->next;
	}
}

void StreamScript::pauseStreams() {
	if (!isStreamingEnabled())
		return;

	if (!activeScript)
		return;

	CueEntry *cue = activeScript->head;

	while (cue) {
		if (!cue->stream.fileName.empty() && cue->stream.musicStream) {
			int status = cue->stream.musicStream->getStatus();

			if (status == MusicStream::kStatusPlaying || cue->stream.paused != 0) {
				cue->stream.paused = 1;
				cue->stream.musicStream->pause(true);
			}
		}

		cue = cue->next;
	}
}

void StreamScript::resumeStreams() {
	if (!isStreamingEnabled())
		return;

	if (!activeScript)
		return;

	CueEntry *cue = activeScript->head;

	while (cue) {
		if (!cue->stream.fileName.empty() && cue->stream.musicStream) {
			int status = cue->stream.musicStream->getStatus();

			if (status != MusicStream::kStatusPlaying && cue->stream.paused != 0) {
				cue->stream.musicStream->pause(false);
				cue->stream.paused = 0;
			}
		}

		cue = cue->next;
	}
}

int StreamScript::getStreamPositionMs(const uint &id, const bool &relative) {
	if (!isStreamingEnabled())
		return 0;

	if (!activeScript)
		return 0;

	int result = -1;
	CueEntry *cue = activeScript->head;

	while (cue) {
		if ((uint)cue->id == id && !cue->stream.fileName.empty() && cue->stream.musicStream) {
			int position = 0;

			cue->stream.musicStream->getMsPosition(nullptr, &position);

			if (relative) {
				position = (int)((double)position - cue->stream.start + 0.5);

				if (position < 0)
					position = 0;
			}

			result = position;
		}

		cue = cue->next;
	}

	return result;
}

void StreamScript::closeAllStreams() {
	if (!isStreamingEnabled())
		return;

	if (!activeScript)
		return;

	CueEntry *cue = activeScript->head;

	while (cue) {
		if (!cue->stream.fileName.empty() && cue->stream.musicStream) {
			cue->stream.musicStream->pause(true);

			delete cue->stream.musicStream;
			cue->stream.musicStream = nullptr;
			cue->state = 0;
		}

		cue = cue->next;
	}
}

void StreamScript::setStreamingGate(int (*gate)()) {
	streamingGate = gate;
}

int StreamScript::isStreamingEnabled() {
	if (streamingGate)
		return streamingGate();

	return 0;
}

Common::SeekableReadStream *StreamScript::openStream(const char *path) {
	// Like CPathingFileServices, the leading ".\" is not part of the lookup
	if (path[0] == '.' && path[1] == '\\')
		path += 2;

	Common::Path filePath(path, '\\');
	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);

	if (stream)
		return stream;

	if (!gameDirectory)
		gameDirectory = new Common::FSDirectory(Common::FSNode(ConfMan.getPath("path")), kGameDirectoryDepth, false, true);

	return gameDirectory->createReadStreamForMember(filePath);
}

Common::SeekableReadStream *StreamScript::openAudioFile(const char *audioFileName) {
	Common::SeekableReadStream *stream = openStream(audioFileName);

	if (stream)
		return stream;

	for (int i = 0; i < searchDirCount; i++) {
		LString path = searchDirs[i] + audioFileName;

		stream = openStream(path.c_str());

		if (stream)
			return stream;
	}

	openMissCount++;

	return nullptr;
}

LString StreamScript::findAudioFile(const char *audioFileName) {
	Common::SeekableReadStream *stream = openStream(audioFileName);

	if (stream) {
		delete stream;

		return LString(audioFileName);
	}

	for (int i = 0; i < searchDirCount; i++) {
		LString path = searchDirs[i] + audioFileName;

		stream = openStream(path.c_str());

		if (stream) {
			delete stream;

			return path;
		}
	}

	findMissCount++;

	return LString(audioFileName);
}

void StreamScript::pushAudioSearchDir(const char *directory) {
	LString &slot = searchDirs[searchDirCount];

	slot = directory;

	if (slot.lastChar() != '\\')
		slot += '\\';

	searchDirCount++;

	if (searchDirCount >= kMaxSearchDirs)
		searchDirCount = kMaxSearchDirs - 1;
}

void StreamScript::clearAudioSearchDirs() {
	searchDirCount = 0;

	delete gameDirectory;
	gameDirectory = nullptr;
}

bool StreamScript::openMusicScript(const char *scriptFileName) {
	if (!isStreamingEnabled())
		return false;

	return openStreamScript(scriptFileName);
}

void StreamScript::setMusicStreamVolume(float volume) {
	setStreamScriptVolume(volume);
}

void StreamScript::startMusicStream(const uint &id, const int &startMs, const bool &immediate) {
	startStream(id, startMs, immediate);
}

bool StreamScript::consumeMusicStreamFlags(const uint &id, bool &looped, bool &ended) {
	return consumeStreamFlags(id, looped, ended);
}

int StreamScript::getMusicStreamPositionMs(const uint &id, const bool &relative) {
	return getStreamPositionMs(id, relative);
}

} // End of namespace Grim
