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

class DiMUSE_InternalMixer {

private:
	int *mixer_amp8Table;
	int *mixer_amp12Table;
	int *mixer_softLMID;
	int *mixer_softLTable;
	int mixer_mixBuf[4096];
	Audio::Mixer *_mixer;
	Audio::SoundHandle _channelHandle;
	int mixer_mixBufSize;
	int mixer_radioChatter = 0;
	int mixer_outWordSize;
	int mixer_outChannelCount;
	int mixer_stereoReverseFlag;

	// Lookup table for a linear volume ramp (0 to 16) accounting for panning (-8 to 8)
	int8 mixer_table[284] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
						      0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  1,  1,  1,  1,  2,  2,  2,
						      2,  2,  3,  3,  3,  3,  3,  3,  0,  0,  1,  1,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  0,
						      0,  1,  1,  2,  2,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,
						      5,  5,  6,  6,  6,  6,  6,  0,  1,  1,  2,  3,  3,  4,  4,  5,  5,  6,  6,  6,  7,  7,  7,  7,  0,  1,
						      2,  2,  3,  4,  4,  5,  6,  6,  7,  7,  7,  8,  8,  8,  8,  0,  1,  2,  3,  3,  4,  5,  6,  6,  7,  7,
						      8,  8,  9,  9,  9,  9,  0,  1,  2,  3,  4,  5,  6,  6,  7,  8,  8,  9,  9,  10, 10, 10, 10, 0,  1,  2,
						      3,  4,  5,  6,  7,  8,  9,  9,  10, 10, 11, 11, 11, 11, 0,  1,  2,  3,  5,  6,  7,  8,  8,  9,  10, 11,
						      11, 11, 12, 12, 12, 0,  1,  3,  4,  5,  6,  7,  8,  9,  10, 11, 11, 12, 12, 13, 13, 13, 0,  1,  3,  4,
						      5,  7,  8,  9,  10, 11, 12, 12, 13, 13, 14, 14, 14, 0,  1,  3,  4,  6,  7,  8,  10, 11, 12, 12, 13, 14,
						      14, 15, 15, 15, 0,  2,  3,  5,  6,  8,  9,  10, 11, 12, 13, 14, 15, 15, 16, 16, 16, 0,  0,  0 };


	void mixer_mixBits8Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixer_mixBits12Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixer_mixBits16Mono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

	void mixer_mixBits8ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixer_mixBits12ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixer_mixBits16ConvertToMono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

	void mixer_mixBits8ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);
	void mixer_mixBits12ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);
	void mixer_mixBits16ConvertToStereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);

	void mixer_mixBits8Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixer_mixBits12Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
	void mixer_mixBits16Stereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

public:
	DiMUSE_InternalMixer(Audio::Mixer *mixer);
	~DiMUSE_InternalMixer();
	int  mixer_initModule(int bytesPerSample, int numChannels, int *mixBuf, int offsetBeginMixBuf, int sizeSampleKB, int mixChannelsNum);
	void mixer_setRadioChatter();
	void mixer_clearRadioChatter();
	int  mixer_clearMixBuff();

	void mixer_mix(uint8 *srcBuf, int inFrameCount, int wordSize, int channelCount, int feedSize, int mixBufStartIndex, int volume, int pan);
	int  mixer_loop(uint8 *destBuffer, int len);
	Audio::QueuingAudioStream *_stream;
};

} // End of namespace Scumm

#endif
