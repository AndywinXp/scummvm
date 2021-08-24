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

#if !defined(SCUMM_IMUSE_DIGI_V2_H) && defined(ENABLE_SCUMM_7_8)
#define SCUMM_IMUSE_DIGI_V2_H

#include "common/scummsys.h"
#include "common/mutex.h"
#include "common/serializer.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "scumm/dimuse.h"
#include "scumm/dimuse_v1/dimuse_v1.h"
#include "scumm/dimuse_v1/dimuse_bndmgr.h"
#include "scumm/dimuse_v1/dimuse_sndmgr.h"
#include "scumm/music.h"
#include "scumm/sound.h"

namespace Audio {
	class AudioStream;
	class Mixer;
}

namespace Scumm {

enum {
	MAX_GROUPS = 16,
	MAX_FADES = 16,
	MAX_TRIGGERS = 8,
	MAX_DEFERS = 8,
	MAX_TRACKS = 8,
	LARGE_FADES = 1,
	SMALL_FADES = 4,
	MAX_DISPATCHES = 8,
	MAX_STREAMZONES = 50,
	LARGE_FADE_DIM = 350000,
	SMALL_FADE_DIM = 44100,
	MAX_FADE_VOLUME = 8323072,
	MAX_STREAMS = 3,
	IMUSE_GROUP_SFX = 1,
	IMUSE_GROUP_SPEECH = 2,
	IMUSE_GROUP_MUSIC = 3,
	IMUSE_GROUP_MUSICEFF = 4
};

class DiMUSE_v2 : public DiMUSE {
	//struct iMUSEDispatch;
	//struct iMUSETrack;
private:
	Common::Mutex _mutex;
	ScummEngine_v7 *_vm;
	Audio::Mixer *_mixer;

	int _callbackFps;		// value how many times callback needs to be called per second

	static void timer_handler(void *refConf);
	void callback();

public:
	DiMUSE_v2(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps);
	~DiMUSE_v2() override;

	void startSound(int sound) override
	{ error("DiMUSE_v2::startSound(int) should be never called"); }

	void setMusicVolume(int vol) override {}
	void stopSound(int sound) override {};
	void stopAllSounds() override {};
	int getSoundStatus(int sound) const override {
		return 0;
	};

	int startVoice(int soundId, Audio::AudioStream *input) override { return 0; };
	int startVoice(int soundId, const char *soundName) override { return 0; };
	void saveLoadEarly(Common::Serializer &ser) override {};
	void resetState() override {};
	void setRadioChatterSFX(bool state) override {};
	void setAudioNames(int32 num, char *names) override {};
	int  startSfx(int soundId, int priority) override { return 0; };
	void setPriority(int soundId, int priority) override {};
	void setVolume(int soundId, int volume) override {};
	void setPan(int soundId, int pan) override {};
	void pause(bool pause) override;
	void parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) override;
	void refreshScripts() override {};
	void flushTracks() override {};

	int32 getCurMusicPosInMs() override { return 0; };
	int32 getCurVoiceLipSyncWidth() override { return 0; };
	int32 getCurVoiceLipSyncHeight() override { return 0; };
	int32 getCurMusicLipSyncWidth(int syncId) override { return 0; };
	int32 getCurMusicLipSyncHeight(int syncId) override { return 0; };
	int32 getSoundElapsedTimeInMs(int soundId) override { return 0; };

	char *iMUSE_audioBuffer;
	int iMUSE_feedSize; // Always 1024 apparently
	int iMUSE_sampleRate;

	struct iMUSEDispatch;
	struct iMUSETrack;
	struct iMUSEStreamZone;

	// CMDs
	int cmd_pauseCount;
	int cmd_hostIntHandler;
	int cmd_hostIntUsecCount;
	int cmd_runningHostCount;
	int cmd_running60HzCount;
	int cmd_running10HzCount;

	void iMUSEHeartbeat();
	int cmds_handleCmds(int cmd, int arg_0, int arg_1, int arg_2, int arg_3, int arg_4,
		int arg_5, int arg_6, int arg_7, int arg_8, int arg_9,
		int arg_10, int arg_11, int arg_12, int arg_13);

	int cmds_init();
	int cmds_deinit();
	int cmds_terminate();
	int cmds_persistence(int cmd, void *funcCall);
	int cmds_print(int param1, int param2, int param3, int param4, int param5, int param6, int param7);
	int cmds_pause();
	int cmds_resume();
	int cmds_save(int *buffer, int bufferSize);
	int cmds_restore(int *buffer);
	int cmds_startSound(int soundId, int priority);
	int cmds_stopSound(int soundId);
	int cmds_stopAllSounds();
	int cmds_getNextSound(int soundId);
	int cmds_setParam(int soundId, int opcode, int value);
	int cmds_getParam(int soundId, int opcode);
	int cmds_setHook(int soundId, int hookId);
	int cmds_getHook(int soundId);
	int cmds_debug();

	// Timer
	int timer_intFlag;
	int timer_oldVec;
	int timer_usecPerInt;

	int timer_moduleInit();
	int timer_moduleDeinit();
	int timer_getUsecPerInt();
	int timer_moduleFree();

	// Streamer
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
	} iMUSEStream;

	iMUSEStream streamer_streams[MAX_STREAMS];
	iMUSEStream *streamer_lastStreamLoaded;
	int streamer_bailFlag;

	int streamer_moduleInit();
	iMUSEStream *streamer_alloc(int soundId, int bufId, int maxRead);
	int streamer_clearSoundInStream(iMUSEStream *streamPtr);
	int streamer_processStreams();
	int *streamer_reAllocReadBuffer(iMUSEStream *streamPtr, unsigned int reallocSize);
	int *streamer_copyBufferAbsolute(iMUSEStream *streamPtr, int offset, int size);
	int streamer_setIndex1(iMUSEStream *streamPtr, unsigned int offset);
	int streamer_setIndex2(iMUSEStream *streamPtr, unsigned int offset);
	int streamer_getFreeBuffer(iMUSEStream *streamPtr);
	int streamer_setSoundToStreamWithCurrentOffset(iMUSEStream *streamPtr, int soundId, int currentOffset);
	int streamer_queryStream(iMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int streamer_feedStream(iMUSEStream *streamPtr, uint8 *srcBuf, unsigned int sizeToFeed, int paused);
	int streamer_fetchData(iMUSEStream *streamPtr);

	// Tracks


	struct iMUSETrack {
		int *prev;
		int *next;
		iMUSEDispatch *dispatchPtr;
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
	};

	iMUSETrack tracks[MAX_TRACKS];
	iMUSETrack *tracks_trackList;

	int tracks_trackCount;
	int tracks_waveCall;
	int tracks_pauseTimer;
	int tracks_prefSampleRate;
	int tracks_running40HzCount;
	int tracks_uSecsToFeed;

	int tracks_moduleInit();
	void tracks_pause();
	void tracks_resume();
	int tracks_save(uint8 *buffer, int leftSize);
	int tracks_restore(uint8 *buffer);
	void tracks_setGroupVol();
	void tracks_callback();
	int tracks_startSound(int soundId, int tryPriority, int group);
	int tracks_stopSound(int soundId);
	int tracks_stopAllSounds();
	int tracks_getNextSound(int soundId);
	int tracks_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int tracks_feedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused);
	void tracks_clear(iMUSETrack *trackPtr);
	int tracks_setParam(int soundId, int opcode, int value);
	int tracks_getParam(int soundId, int opcode);
	int tracks_lipSync(int soundId, int syncId, int msPos, int *width, char *height);
	int tracks_setHook(int soundId, int hookId);
	int tracks_getHook(int soundId);
	void tracks_free();

	int tracks_debug();

	// Dispatch
	struct iMUSEStreamZone{
		int *prev;
		int *next;
		int useFlag;
		int offset;
		int size;
		int fadeFlag;
	};

	struct iMUSEDispatch {
		iMUSETrack *trackPtr;
		int wordSize;
		int sampleRate;
		int channelCount;
		int currentOffset;
		int audioRemaining;
		int map[256]; // For COMI it's bigger
		iMUSEStream *streamPtr;
		int streamBufID;
		iMUSEStreamZone *streamZoneList;
		int streamErrFlag;
		int *fadeBuf;
		int fadeOffset;
		int fadeRemaining;
		int fadeWordSize;
		int fadeSampleRate;
		int fadeChannelCount;
		int fadeSyncFlag;
		int fadeSyncDelta;
		int fadeVol;
		int fadeSlope;
	};

	iMUSEDispatch dispatches[MAX_DISPATCHES];
	iMUSEStreamZone streamZones[MAX_STREAMZONES];
	int *dispatch_buf;
	int dispatch_size;
	int *dispatch_smallFadeBufs;
	int *dispatch_largeFadeBufs;
	int dispatch_fadeSize;
	int dispatch_largeFadeFlags[LARGE_FADES];
	int dispatch_smallFadeFlags[SMALL_FADES];
	int dispatch_fadeStartedFlag;
	int buff_hookid;
	int dispatch_requestedFadeSize;
	int dispatch_curStreamBufSize;
	int dispatch_curStreamCriticalSize;
	int dispatch_curStreamFreeSpace;
	int dispatch_curStreamPaused;

	int dispatch_moduleInit();
	iMUSEDispatch *dispatch_getDispatchByTrackId(int trackId);
	int dispatch_save(uint8 *dst, int size);
	int dispatch_restore(uint8 *src);
	int dispatch_allocStreamZones();
	int dispatch_alloc(iMUSETrack *trackPtr, int groupId);
	int dispatch_release(iMUSETrack *trackPtr);
	int dispatch_switchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag);
	void dispatch_processDispatches(iMUSETrack *trackPtr, int feedSize, int sampleRate);
	int dispatch_predictFirstStream();
	int dispatch_getNextMapEvent(iMUSEDispatch *dispatchPtr);
	int dispatch_convertMap(uint8 *rawMap, uint8 *destMap);
	void dispatch_predictStream(iMUSEDispatch *dispatch);
	void dispatch_parseJump(iMUSEDispatch *dispatchPtr, iMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent);
	iMUSEStreamZone *dispatch_allocStreamZone();
	void dispatch_free();

	// Groups
	int groupEffVols[MAX_GROUPS];
	int groupVols[MAX_GROUPS];

	int groups_moduleInit();
	int groups_moduleDeinit();
	int groups_setGroupVol(int id, int volume);
	int groups_getGroupVol(int id);
	int groups_moduleDebug();

	// Fades
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
	} iMUSEFade;

	iMUSEFade fades[MAX_FADES];
	int fadesOn;

	int fades_moduleInit();
	int fades_moduleDeinit();
	int fades_save(unsigned char *buffer, int sizeLeft);
	int fades_restore(unsigned char *buffer);
	int fades_fadeParam(int soundId, int opcode, int destinationValue, int fadeLength);
	void fades_clearFadeStatus(int soundId, int opcode);
	void fades_loop();
	void fades_moduleFree();
	int fades_moduleDebug();

	// Triggers
	typedef struct {
		int sound;
		char text[256];
		int opcode;
		int args_0_;
		int args_1_;
		int args_2_;
		int args_3_;
		int args_4_;
		int args_5_;
		int args_6_;
		int args_7_;
		int args_8_;
		int args_9_;
		int clearLater;
	} iMUSETrigger;

	typedef struct {
		int counter;
		int opcode;
		int args_0_;
		int args_1_;
		int args_2_;
		int args_3_;
		int args_4_;
		int args_5_;
		int args_6_;
		int args_7_;
		int args_8_;
		int args_9_;
	} iMUSEDefer;

	iMUSETrigger trigs[MAX_TRIGGERS];
	iMUSEDefer defers[MAX_DEFERS];

	int  triggers_moduleInit();
	int  triggers_clear();
	int  triggers_save(int *buffer, int bufferSize);
	int  triggers_restore(int *buffer);
	int  triggers_setTrigger(int soundId, char *marker, int opcode);
	int  triggers_checkTrigger(int soundId, char *marker, int opcode);
	int  triggers_clearTrigger(int soundId, char *marker, int opcode);
	void triggers_processTriggers(int soundId, char *marker);
	int  triggers_deferCommand(int count, int opcode);
	void triggers_loop();
	int  triggers_countPendingSounds(int soundId);
	int  triggers_moduleFree();
	void triggers_copyTEXT(char *dst, char *marker);
	int  triggers_compareTEXT(char *marker1, char *marker2);
	int  triggers_getMarkerLength(char *marker);

	int  triggers_defersOn;
	int  triggers_midProcessing;
	char triggers_textBuffer[256];
	char triggers_empty_marker = '\0';

	// Files
	int files_moduleInit();
	int files_moduleDeinit();
	uint8 *files_getSoundAddrData(int soundId);
	int files_fetchMap(int soundId);
	int files_getNextSound(int soundId);
	int files_checkRange(int soundId);
	int files_seek(int soundId, int offset, int mode);
	int files_read(int soundId, uint8 *buf, int size);
	//iMUSESoundBuffer *files_getBufInfo(int bufId);

	// Wave
	int wvSlicingHalted = 1;

	int wave_init();
	int wave_terminate();
	int wave_pause();
	int wave_resume();
	int wave_save(unsigned char *buffer, int bufferSize);
	int wave_restore(unsigned char *buffer);
	void wave_setGroupVol();
	int wave_startSound(int soundId, int priority);
	int wave_stopSound(int soundId);
	int wave_stopAllSounds();
	int wave_getNextSound(int soundId);
	int wave_setParam(int soundId, int opcode, int value);
	int wave_getParam(int soundId, int opcode);
	int wave_setHook(int soundId, int hookId);
	int wave_getHook(int soundId);
	int wave_startSoundInGroup(int soundId, int priority, int groupId);
	int wave_switchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1);
	int wave_processStreams();
	int wave_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int wave_feedStream(int soundId, int srcBuf, int sizeToFeed, int paused);
	int wave_lipSync(int soundId, int syncId, int msPos, int *width, int *height);

	// Waveapi

};

} // End of namespace Scumm

#endif
