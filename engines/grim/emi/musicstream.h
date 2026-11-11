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

#ifndef GRIM_EMI_MUSICSTREAM_H
#define GRIM_EMI_MUSICSTREAM_H

#include "common/mutex.h"
#include "common/scummsys.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class MusicStream : public Audio::AudioStream {
public:
	enum {
		kStatusDone = 2,
		kStatusPlaying = 4,
		kStatusStopped = 8
	};

	static MusicStream *open(const LString &fileName);
	~MusicStream() override;

	void start();
	void pause(bool paused);
	int getStatus();
	void setVolume(int volume);
	void setPan(int pan);
	void setLoopCount(int loopCount);
	void setLoopBlock(int startByte, int endByte);
	void setPosition(int bytePosition);
	int getPosition();
	void setMsPosition(int milliseconds);
	void getMsPosition(int *total, int *current);
	void refreshMixerVolume();

	int readBuffer(int16 *buffer, const int numSamples) override;
	bool isStereo() const override;
	int getRate() const override;
	bool endOfData() const override;

private:
	MusicStream(Audio::SeekableAudioStream *decoder, uint dataSize);

	int bytesToFrames(int bytes);
	int framesToBytes(int frames);
	void seekDecoder(int frame);
	void setFramePosition(int frame);
	int getMixerVolume();
	int getMixerBalance();

	Common::Mutex _mutex;
	Audio::SeekableAudioStream *_decoder;
	Audio::SoundHandle _handle;
	uint _dataSize;
	int _totalFrames;
	int _rate;
	bool _stereo;
	bool _started;
	bool _paused;
	bool _done;
	int _playFrame;
	int _pendingFrame;
	int _loopStartFrame;
	int _loopEndFrame;
	int _loopCount;
	int _volume;
	int _pan;
};

} // End of namespace Grim

#endif
