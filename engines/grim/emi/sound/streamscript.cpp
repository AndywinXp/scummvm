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

#include "engines/grim/emi/sound/audiosystem.h"
#include "engines/grim/emi/sound/cueentry.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/sound/musicstream.h"
#include "engines/grim/emi/sound/sounds.h"
#include "engines/grim/emi/sound/streamscript.h"

namespace Grim {

static const int kLineSize = 1024;
static const int kGameDirectoryDepth = 4;
static const int kCenterPan = 64;

StreamScript::StreamScript(Sounds *sounds) {
	_sounds = sounds;
	_head = nullptr;
	_loaded = false;

	_firstUpdateMs = 0;
	_lastUpdateMs = 0;
	_masterVolume = 1.0f;
	_currentFadeRate = 1.0f;
	_searchDirCount = 0;
	_openMissCount = 0;
	_findMissCount = 0;
	_gameDirectory = nullptr;
}

StreamScript::~StreamScript() {
	closeStreamScript();
	clearAudioSearchDirs();
}

void *StreamScript::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void StreamScript::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void StreamScript::closeStreams() {
	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	CueEntry *cue = _head;

	while (cue) {
		if (!cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			cue->_stream._musicStream->pause(true);

			delete cue->_stream._musicStream;
			cue->_stream._musicStream = nullptr;
			cue->_state = 0;
		}

		cue = cue->_next;
	}
}

bool StreamScript::openStreamScript(const char *scriptFileName) {
	if (_loaded)
		return false;

	_fileName = scriptFileName;

	Common::SeekableReadStream *script = openAudioFile(scriptFileName);

	if (!script) {
		_fileName.clear();

		return false;
	}

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
			_head = cue;
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

			if (!lastCue->_stream._fileName.empty())
				continue;

			CueStream &cueStream = lastCue->_stream;

			cueStream._fileName = findAudioFile(quoted.c_str());
			cueStream._trim = (float)((double)trim * (double)0.0078739999f);

			LString jmmPath = cueStream._fileName;
			uint dotPosition = jmmPath.rfind('.');

			if (dotPosition != LString::npos)
				jmmPath = jmmPath.substr(0, dotPosition);

			jmmPath += ".jmm";

			Common::SeekableReadStream *jmm = openStream(jmmPath.c_str());

			if (!jmm) {
				cueStream._start = 0.0;
				cueStream._jumps[0].srcMs = -1.0;
				cueStream._jumps[0].dstMs = 0.0;
				cueStream._jumps[0].field16 = 0;
				cueStream._hasJump = 1;

				continue;
			}

			while (!jmm->eos()) {
				jmmLine[0] = 0;

				jmm->readLine(jmmLine, sizeof(jmmLine));
				sscanf(jmmLine, " %s ", token);

				if (strcmp(token, ".start") == 0) {
					sscanf(jmmLine, " %s %f", token, &parsedStart);
					cueStream._start = (double)parsedStart;
				} else if (strcmp(token, ".jump") == 0) {
					int assigned = sscanf(jmmLine, " %s %f %f %d %lf %lf %d %d %d", token, &parsedJumpSrc, &parsedJumpDst, &parsedField16, &parsedRatio, &parsedPcm, &parsedSrcByte, &parsedDstByte, &parsedLoopCount);

					// The disasm stores the loop count over the low bytes of the pcm rate
					if (assigned >= 9) {
						uint64 pcmBits;

						memcpy(&pcmBits, &parsedPcm, sizeof(pcmBits));
						pcmBits = (pcmBits & ~(uint64)0xFFFF) | (((uint32)parsedLoopCount >> 16) & 0xFFFF);
						memcpy(&parsedPcm, &pcmBits, sizeof(pcmBits));
					}

					cueStream._jumps[0].srcMs = (double)parsedJumpSrc;
					cueStream._jumps[0].dstMs = (double)parsedJumpDst;
					cueStream._jumps[0].field16 = (int16)parsedField16;
					cueStream._hasJump = 1;

					if (parsedRatio > 0.0) {
						cueStream._ratio = parsedRatio;
						cueStream._invRatio = 1.0 / parsedRatio;
					} else {
						cueStream._ratio = 1.0;
						cueStream._invRatio = 1.0;
					}

					if (parsedPcm > 0.0) {
						cueStream._pcmBytesPerMs = parsedPcm;
						cueStream._invPcmBytesPerMs = 1.0 / parsedPcm;
					} else {
						cueStream._pcmBytesPerMs = 1.0;
						cueStream._invPcmBytesPerMs = 1.0;
					}

					cueStream._startByte = (int)(parsedPcm * cueStream._start + 0.5);
					cueStream._srcByte = parsedSrcByte;
					cueStream._dstByte = parsedDstByte;
					cueStream._loopCount = (int16)parsedLoopCount;

					break;
				} else if (strcmp(token, ".end") == 0) {
					sscanf(jmmLine, " %s %f", token, &parsedEnd);
					cueStream._end = (double)parsedEnd;

					break;
				}
			}

			delete jmm;
		}
	}

	delete script;
	_loaded = true;

	return true;
}

void StreamScript::closeStreamScript() {
	if (!_loaded)
		return;

	closeStreams();

	while (_head) {
		CueEntry *next = _head->_next;

		delete _head;
		_head = next;
	}

	_loaded = false;
	_fileName.clear();
}

void StreamScript::setStreamScriptVolume(float volume) {
	float clamped = volume;

	if (!(volume >= 0.0f))
		clamped = 0.0f;
	else if (volume > 1.0f)
		clamped = 1.0f;

	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	_masterVolume = clamped;

	CueEntry *cue = _head;

	while (cue) {
		if (!cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			int streamVolume = (int)((double)cue->_stream._trim * (double)cue->_fadeVolume * (double)_masterVolume * 127.0 + (double)0.5f);

			cue->_stream._musicStream->setVolume(streamVolume);
		}

		cue = cue->_next;
	}
}

void StreamScript::startStream(const uint &id, const int &startMs, const bool &immediate) {
	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	CueEntry *cue = _head;
	CueEntry *keptCue = nullptr;
	bool otherPlaying = false;
	int otherSync = 0;
	MusicStream *otherStream = nullptr;
	bool otherStreamClosed = false;
	double otherStart = 0.0;
	double otherMsPerByte = 0.0;

	while (cue) {
		if ((uint)cue->_id != id && !cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			CueEntry *closedCue = nullptr;

			otherPlaying = true;

			if (keptCue) {
				closedCue = keptCue;

				if (!(keptCue->_fadeVolume >= cue->_fadeVolume))
					keptCue = cue;
				else
					closedCue = cue;

				closedCue->_fadeVolume = 0.0f;
				closedCue->_stream._musicStream->pause(true);

				// The original would use the closed stream for the sync position
				if (otherStream == closedCue->_stream._musicStream) {
					otherStream = nullptr;
					otherStreamClosed = true;
				}

				delete closedCue->_stream._musicStream;
				closedCue->_stream._musicStream = nullptr;
				closedCue->_state = 0;
			} else {
				keptCue = cue;
			}

			if (closedCue != cue) {
				cue->_state = 2;

				if (cue->_sync > 0) {
					otherSync = cue->_sync;
					otherStream = cue->_stream._musicStream;
					otherStreamClosed = false;
					otherStart = cue->_stream._start;
					otherMsPerByte = cue->_stream._invPcmBytesPerMs * cue->_stream._ratio;
				}
			}
		}

		cue = cue->_next;
	}

	cue = _head;

	while (cue) {
		if ((uint)cue->_id == id && !cue->_stream._fileName.empty()) {
			if (!cue->_stream._musicStream) {
				cue->_stream._musicStream = MusicStream::open(this, cue->_stream._fileName);

				if (cue->_stream._musicStream) {
					MusicStream *musicStream = cue->_stream._musicStream;

					_currentFadeRate = cue->_fadeRate;
					cue->_looped = false;
					cue->_lastPositionMs = 0;
					cue->_ended = false;

					int startPosition = cue->_stream._startByte;
					bool synced = false;
					uint syncStamp = 0;

					if (otherSync > 0 && otherStream && cue->_sync == otherSync) {
						int otherPosition = otherStream->getPositionStamped(&syncStamp);

						synced = true;
						double offsetMs = (double)otherPosition * otherMsPerByte - otherStart;

						if (!(offsetMs >= 0.0))
							offsetMs = 0.0;

						startPosition += (int)(offsetMs * cue->_stream._pcmBytesPerMs + (double)0.05f);
					} else if (otherSync > 0 && otherStreamClosed && cue->_sync == otherSync) {
						// The original would sync with the closed stream here
						warning("StreamScript::startStream(): the stream to sync with has been closed");
					}

					musicStream->setLoopBlock(cue->_stream._dstByte, cue->_stream._srcByte);
					musicStream->setLoopCount(cue->_stream._loopCount);

					int bytePosition = (int)((double)startPosition * cue->_stream._invRatio + 0.5);

					if (startMs >= 0) {
						musicStream->setMsPosition(startMs);
						cue->_lastPositionMs = startMs;
					} else if (synced) {
						musicStream->setPositionSynced(bytePosition, syncStamp);
					} else {
						musicStream->setPosition(bytePosition);
					}

					if (immediate)
						cue->_fadeVolume = 1.0f;
					else if (!otherPlaying && startMs < 0)
						cue->_fadeVolume = 1.0f;
					else
						cue->_fadeVolume = 0.0f;

					int streamVolume = (int)((double)_masterVolume * (double)cue->_stream._trim * (double)cue->_fadeVolume * 127.0 + (double)0.5f);

					musicStream->setVolume(streamVolume);
					musicStream->setPan(kCenterPan);
					musicStream->start();
				}
			}

			cue->_state = 1;
		}

		cue = cue->_next;
	}
}

bool StreamScript::consumeStreamFlags(const uint &id, bool &looped, bool &ended) {
	if (!isStreamingEnabled())
		return false;

	if (!_loaded)
		return false;

	looped = false;
	ended = false;

	CueEntry *cue = _head;

	while (cue) {
		if ((uint)cue->_id == id) {
			looped = cue->_looped;
			ended = cue->_ended;
			cue->_looped = false;
			cue->_ended = false;

			return true;
		}

		cue = cue->_next;
	}

	return false;
}

void StreamScript::updateStreams() {
	uint now = g_system->getMillis();

	if (_firstUpdateMs == 0) {
		_firstUpdateMs = now;
		_lastUpdateMs = now;

		return;
	}

	int delta = (int)(now - _lastUpdateMs);

	_lastUpdateMs = now;

	float elapsed = (float)((double)delta * (double)0.001f);

	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	CueEntry *cue = _head;

	while (cue) {
		MusicStream *musicStream = cue->_stream._musicStream;

		if (musicStream) {
			if (!cue->_looped) {
				int current = 0;

				musicStream->getMsPosition(nullptr, &current);

				if (current < cue->_lastPositionMs)
					cue->_looped = true;

				cue->_lastPositionMs = current;
			}

			if (!cue->_ended) {
				if (musicStream->getStatus() == MusicStream::kStatusDone)
					cue->_ended = true;
			}

			if (cue->_state != 0) {
				double change = (double)_currentFadeRate * (double)elapsed;
				float fade;

				if (cue->_state == 1)
					fade = (float)(change + (double)cue->_fadeVolume);
				else
					fade = (float)((double)cue->_fadeVolume - change);

				cue->_fadeVolume = fade;

				if (!(fade >= 0.0f)) {
					cue->_fadeVolume = 0.0f;

					musicStream->pause(true);

					delete musicStream;
					cue->_stream._musicStream = nullptr;
					cue->_state = 0;
				} else {
					if (fade > 1.0f) {
						cue->_fadeVolume = 1.0f;
						cue->_state = 0;
					}

					int streamVolume = (int)((double)_masterVolume * (double)cue->_stream._trim * (double)cue->_fadeVolume * 127.0 + (double)0.5f);

					musicStream->setVolume(streamVolume);
				}
			}
		}

		cue = cue->_next;
	}
}

void StreamScript::pauseStreams() {
	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	CueEntry *cue = _head;

	while (cue) {
		if (!cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			int status = cue->_stream._musicStream->getStatus();

			if (status == MusicStream::kStatusPlaying || cue->_stream._paused != 0) {
				cue->_stream._paused = 1;
				cue->_stream._musicStream->pause(true);
			}
		}

		cue = cue->_next;
	}
}

void StreamScript::resumeStreams() {
	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	CueEntry *cue = _head;

	while (cue) {
		if (!cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			int status = cue->_stream._musicStream->getStatus();

			if (status != MusicStream::kStatusPlaying && cue->_stream._paused != 0) {
				cue->_stream._musicStream->pause(false);
				cue->_stream._paused = 0;
			}
		}

		cue = cue->_next;
	}
}

int StreamScript::getStreamPositionMs(const uint &id, const bool &relative) {
	if (!isStreamingEnabled())
		return 0;

	if (!_loaded)
		return 0;

	int result = -1;
	CueEntry *cue = _head;

	while (cue) {
		if ((uint)cue->_id == id && !cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			int position = 0;

			cue->_stream._musicStream->getMsPosition(nullptr, &position);

			if (relative) {
				position = (int)((double)position - cue->_stream._start + 0.5);

				if (position < 0)
					position = 0;
			}

			result = position;
		}

		cue = cue->_next;
	}

	return result;
}

void StreamScript::closeAllStreams() {
	if (!isStreamingEnabled())
		return;

	if (!_loaded)
		return;

	CueEntry *cue = _head;

	while (cue) {
		if (!cue->_stream._fileName.empty() && cue->_stream._musicStream) {
			cue->_stream._musicStream->pause(true);

			delete cue->_stream._musicStream;
			cue->_stream._musicStream = nullptr;
			cue->_state = 0;
		}

		cue = cue->_next;
	}
}

int StreamScript::isStreamingEnabled() {
	return getMilesDriver() ? 1 : 0;
}

EMIMilesDriver *StreamScript::getMilesDriver() {
	AudioSystem *audioSystem = _sounds->getAudioSystem();

	if (!audioSystem)
		return nullptr;

	return audioSystem->getMilesDriver();
}

Common::SeekableReadStream *StreamScript::openStream(const char *path) {
	// Like CPathingFileServices, the leading ".\" is not part of the lookup
	if (path[0] == '.' && path[1] == '\\')
		path += 2;

	Common::Path filePath(path, '\\');
	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);

	if (stream)
		return stream;

	if (!_gameDirectory)
		_gameDirectory = new Common::FSDirectory(Common::FSNode(ConfMan.getPath("path")), kGameDirectoryDepth, false, true);

	return _gameDirectory->createReadStreamForMember(filePath);
}

Common::SeekableReadStream *StreamScript::openAudioFile(const char *audioFileName) {
	Common::SeekableReadStream *stream = openStream(audioFileName);

	if (stream)
		return stream;

	for (int i = 0; i < _searchDirCount; i++) {
		LString path = _searchDirs[i] + audioFileName;

		stream = openStream(path.c_str());

		if (stream)
			return stream;
	}

	_openMissCount++;

	return nullptr;
}

LString StreamScript::findAudioFile(const char *audioFileName) {
	Common::SeekableReadStream *stream = openStream(audioFileName);

	if (stream) {
		delete stream;

		return LString(audioFileName);
	}

	for (int i = 0; i < _searchDirCount; i++) {
		LString path = _searchDirs[i] + audioFileName;

		stream = openStream(path.c_str());

		if (stream) {
			delete stream;

			return path;
		}
	}

	_findMissCount++;

	return LString(audioFileName);
}

bool StreamScript::hasAudioFile(const char *audioFileName) {
	Common::SeekableReadStream *stream = openStream(audioFileName);

	if (stream) {
		delete stream;

		return true;
	}

	for (int i = 0; i < _searchDirCount; i++) {
		LString path = _searchDirs[i] + audioFileName;

		stream = openStream(path.c_str());

		if (stream) {
			delete stream;

			return true;
		}
	}

	return false;
}

void StreamScript::pushAudioSearchDir(const char *directory) {
	LString &slot = _searchDirs[_searchDirCount];

	slot = directory;

	if (slot.lastChar() != '\\')
		slot += '\\';

	_searchDirCount++;

	if (_searchDirCount >= kMaxSearchDirs)
		_searchDirCount = kMaxSearchDirs - 1;
}

void StreamScript::clearAudioSearchDirs() {
	_searchDirCount = 0;

	delete _gameDirectory;
	_gameDirectory = nullptr;
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
