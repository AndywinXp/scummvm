/* ScummVM - Graphic Adventure Engine
*
* ScummVM is the legal property of its developers, whose names
* are too numerous to list here. Please refer to the COPYRIGHT
* file distributed with this source distribution.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#if !defined(SCUMM_IMUSE_DIGI_V2_MIXER_H) && defined(ENABLE_SCUMM_7_8)
#define SCUMM_IMUSE_DIGI_V2_MIXER_H

#include "common/scummsys.h"
#include "common/mutex.h"
#include "common/serializer.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "scumm/dimuse.h"
#include "scumm/dimuse_v2/dimuse_v2.h"
#include "scumm/music.h"
#include "scumm/sound.h"
#include "audio/mixer.h"
#include "audio/audiostream.h"

namespace Audio {
class AudioStream;
class Mixer;
class QueuingAudioStream;
}

namespace Scumm {

class DiMUSEInternalMixer {

private:
	int *_amp8Table;
	int *_amp12Table;
	int *_softLMID;
	int *_softLTable;
	
	uint8 *_mixBuf;

	Audio::Mixer *_mixer;
	Audio::SoundHandle _channelHandle;
	int _mixBufSize;
	int _radioChatter;
	int _outWordSize;
	int _outChannelCount;
	int _stereoReverseFlag;

	void mixBits8Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixBits12Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixBits16Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

	void mixBits8ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixBits12ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixBits16ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

	void mixBits8ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);
	void mixBits12ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);
	void mixBits16ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);

	void mixBits8Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixBits12Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixBits16Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

public:
	DiMUSEInternalMixer(Audio::Mixer *mixer);
	~DiMUSEInternalMixer();
	int  init(int bytesPerSample, int numChannels, uint8 *mixBuf, int mixBufSize, int sizeSampleKB, int mixChannelsNum);
	void setRadioChatter();
	void clearRadioChatter();
	int  clearMixerBuffer();

	void mix(uint8 *srcBuf, int inFrameCount, int wordSize, int channelCount, int feedSize, int mixBufStartIndex, int volume, int pan);
	int  loop(uint8 **destBuffer, int len);
	Audio::QueuingAudioStream *_stream;
};

} // End of namespace Scumm

#endif
