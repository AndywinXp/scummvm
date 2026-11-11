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

#ifndef GRIM_EMI_CWAVECHANNEL_H
#define GRIM_EMI_CWAVECHANNEL_H

#include "audio/audiostream.h"
#include "audio/mixer.h"

#include "engines/grim/emi/caudiochannel.h"

namespace Grim {

class CSoundCue;

class CWaveChannel : public CAudioChannel {
public:
	enum {
		kSampleFree = 1,
		kSampleDone = 2,
		kSamplePlaying = 4,
		kSampleStopped = 8,
		kSamplePlayingReleased = 16
	};

	CWaveChannel();
	~CWaveChannel() override;

	void *operator new(size_t size);
	void operator delete(void *block);

	int update() override;
	bool assignToSample(CSoundCue *newCue) override;
	void release() override;
	void start() override;
	void stop() override;
	void pause() override;
	void resume() override;
	int getStatus() override;
	int getPositionMs(int *totalMs) override;
	void setPositionMs(int milliseconds) override;
	void setVolume(float volume) override;
	void setPlaybackRate(int rate) override;
	void setPan(float pan) override;
	bool setReverb(int preset, float time, float mix, float unusedFirst, float damping, float unusedSecond) override;
	bool hasFiniteLoopCount() override;
	void setLoopCount(int count) override;
	void setCutoff(float nearDistance, float farDistance) override;
	void applyCueSettings(CSoundCue *newCue) override;

	void updateSpatialization();
	void refreshMixerVolume();
	bool isSampleActive();
	void releaseDigitalOutput();
	void reacquireDigitalOutput();

	bool started;
	bool hasSample;
	CSoundCue *cue;

	static int silentTailLoopCount;

	static int panToBalance(int pan);
	static int volumeToMixer(int volume);

private:
	void initSample();
	void releaseSample();
	void setSampleAddress(const byte *address, uint size);
	void setSampleType(int format);
	void setSampleLoopBlock(uint start, uint end);
	void startSample();
	void stopSample();
	void resumeSample();
	void endSample();
	int getSampleStatus();
	int getSampleLoopCount();
	void setSampleMsPosition(int milliseconds);
	uint getSamplePositionMs();
	uint getSampleLengthMs();
	void getSampleState(uint64 elapsedBytes, uint *position, int *loopCount, bool *finished);
	void rebaseSample();
	void finishSample();
	uint64 getElapsedBytes();
	static int getPanScale(int index);
	int getMixerVolume();
	int getMixerBalance();
	uint getFrameSize();
	uint getBytesPerSecond();
	uint msToBytes(uint milliseconds);
	uint bytesToMs(uint bytes);
	void getLoopBlock(uint *start, uint *end);
	void playMixerStream();
	void stopMixerStream();
	void pauseMixerStream(bool pauseIt);
	Audio::AudioStream *makeLoopingStream(uint offset, byte rawFlags);

	int sampleStatus;
	const byte *sampleAddress;
	uint sampleSize;
	bool sampleStereo;
	bool sample16Bit;
	int sampleRate;
	int sampleVolume;
	int samplePan;
	int sampleLoopCount;
	uint sampleLoopStart;
	uint sampleLoopEnd;
	bool silentTail;
	uint samplePosition;
	uint runStartMs;
	Audio::SoundHandle soundHandle;
	bool reverbActive;
	int reverbEnvironment;
	float reverbTime;
	float reverbMix;
	float reverbDamping;
};

} // End of namespace Grim

#endif
