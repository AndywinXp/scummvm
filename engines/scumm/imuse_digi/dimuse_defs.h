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

#if !defined(SCUMM_IMUSE_DIGI_DEFS_H) && defined(ENABLE_SCUMM_7_8)
#define SCUMM_IMUSE_DIGI_DEFS_H

namespace Scumm {

#define MAX_GROUPS 16
#define MAX_FADES 16
#define MAX_TRIGGERS 8
#define MAX_DEFERS 8
#define MAX_TRACKS 8
#define LARGE_FADES 1
#define SMALL_FADES 4
#define MAX_DISPATCHES 8
#define MAX_STREAMZONES 50
#define LARGE_FADE_DIM 350000
#define SMALL_FADE_DIM 44100
#define MAX_FADE_VOLUME 8323072
#define MAX_STREAMS 3
#define MAX_SOUNDID 4294967280
#define DIMUSE_SAMPLERATE 22050
#define DIMUSE_GROUP_SFX 1
#define DIMUSE_GROUP_SPEECH 2
#define DIMUSE_GROUP_MUSIC 3
#define DIMUSE_GROUP_MUSICEFF 4
#define DIMUSE_BUFFER_SPEECH 1
#define DIMUSE_BUFFER_MUSIC 2
#define DIMUSE_BUFFER_SFX 3
#define NUM_HEADERS 8
#define SMUSH_SOUNDID 12345678

// Parameters IDs
#define P_BOGUS_ID	     0x0
#define P_SND_TRACK_NUM  0x100
#define P_TRIGS_SNDS     0x200
#define P_MARKER         0x300
#define P_GROUP          0x400
#define P_PRIORITY       0x500
#define P_VOLUME         0x600
#define P_PAN            0x700
#define P_DETUNE         0x800
#define P_TRANSPOSE      0x900
#define P_MAILBOX        0xA00
#define P_UNKNOWN        0xF00
#define P_SND_HAS_STREAM 0x1800
#define P_STREAM_BUFID   0x1900
#define P_SND_POS_IN_MS  0x1A00

struct IMuseDigiDispatch;
struct IMuseDigiTrack;
struct IMuseDigiStreamZone;

typedef struct {
	int sound;
	char text[256];
	int opcode;
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
	int g;
	int h;
	int i;
	int j;
	int clearLater;
} IMuseDigiTrigger;

typedef struct {
	int counter;
	int opcode;
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
	int g;
	int h;
	int i;
	int j;
} IMuseDigiDefer;

typedef struct {
	int status;
	int sound;
	int param;
	int currentVal;
	int counter;
	int length;
	int slope;
	int slopeMod;
	int modOvfloCounter;
	int nudge;
} IMuseDigiFade;

struct IMuseDigiTrack {
	IMuseDigiTrack *prev;
	IMuseDigiTrack *next;
	IMuseDigiDispatch *dispatchPtr;
	int soundId;
	int marker;
	int group;
	int priority;
	int vol;
	int effVol;
	int pan;
	int detune;
	int transpose;
	int pitchShift;
	int mailbox;
	int jumpHook;
	int syncSize_0;
	byte *syncPtr_0;
	int syncSize_1;
	byte *syncPtr_1;
	int syncSize_2;
	byte *syncPtr_2;
	int syncSize_3;
	byte *syncPtr_3;
};

struct IMuseDigiStreamZone {
	IMuseDigiStreamZone *prev;
	IMuseDigiStreamZone *next;
	int useFlag;
	int offset;
	int size;
	int fadeFlag;
};

typedef struct {
	int soundId;
	int curOffset;
	int endOffset;
	int bufId;
	uint8 *buf;
	int bufFreeSize;
	int loadSize;
	int criticalSize;
	int maxRead;
	int loadIndex;
	int readIndex;
	int paused;
	int vocLoopFlag;
	int vocLoopTriggerOffset;
} IMuseDigiStream;

typedef struct {
	uint8 *buffer;
	int bufSize;
	int loadSize;
	int criticalSize;
} IMuseDigiSndBuffer;

struct IMuseDigiDispatch {
	IMuseDigiTrack *trackPtr;
	int wordSize;
	int sampleRate;
	int channelCount;
	int currentOffset;
	int audioRemaining;
	int map[2048]; // For DIG it's 256
	IMuseDigiStream *streamPtr;
	int streamBufID;
	IMuseDigiStreamZone *streamZoneList;
	int streamErrFlag;
	uint8 *fadeBuf;
	int fadeOffset;
	int fadeRemaining;
	int fadeWordSize;
	int fadeSampleRate;
	int fadeChannelCount;
	int fadeSyncFlag;
	int fadeSyncDelta;
	int fadeVol;
	int fadeSlope;
	int vocLoopStartingPoint;
};

typedef struct {
	int bytesPerSample;
	int numChannels;
	uint8 *mixBuf;
	int mixBufSize;
	int sizeSampleKB;
} waveOutParamsStruct;

} // End of namespace Scumm
#endif
