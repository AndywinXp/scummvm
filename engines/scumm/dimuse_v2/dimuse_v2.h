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
#include "scumm/dimuse_v2/dimuse_v2_defs.h"
#include "scumm/dimuse_v2/dimuse_v2_internalmixer.h"
#include "scumm/dimuse_v2/dimuse_v2_groups.h"
#include "scumm/dimuse_v2/dimuse_v2_timer.h"
#include "scumm/dimuse_v2/dimuse_v2_fades.h"
#include "scumm/dimuse_v2/dimuse_v2_triggers.h"
#include "scumm/dimuse_v1/dimuse_bndmgr.h"
#include "scumm/dimuse_v1/dimuse_sndmgr.h"
#include "scumm/dimuse_v1/dimuse_tables.h"
#include "scumm/music.h"
#include "scumm/sound.h"
#include "audio/mixer.h"
#include "audio/decoders/raw.h"

namespace Audio {
class AudioStream;
class Mixer;
class QueuingAudioStream;
}

namespace Scumm {

struct DiMUSEDispatch;
struct DiMUSETrack;
struct DiMUSEStreamZone;

class DiMUSE_v2 : public DiMUSE {
private:
	Common::Mutex _mutex;
	ScummEngine_v7 *_vm;
	Audio::Mixer *_mixer;

	DiMUSESndMgr *_sound;
	DiMUSEInternalMixer *_internalMixer;
	DiMUSEGroupsHandler *_groupsHandler;
	DiMUSETimerHandler *_timerHandler;
	DiMUSEFadesHandler *_fadesHandler;
	DiMUSETriggersHandler *_triggersHandler;
	int _callbackFps;
	static void timer_handler(void *refConf);
	void callback();

	int _currentSpeechVolume, _currentSpeechFrequency, _currentSpeechPan;
	int _curMixerMusicVolume, _curMixerSpeechVolume, _curMixerSFXVolume;
	bool _radioChatterSFX = false;

	int32 _attributes[188];	// internal attributes for each music file to store and check later
	int32 _nextSeqToPlay;	// id of sequence type of music needed played
	int32 _curMusicState;	// current or previous id of music
	int32 _curMusicSeq;		// current or previous id of sequence music
	int32 _curMusicCue;		// current cue for current music. used in FT
	int _stopSequenceFlag;
	int _scriptInitializedFlag = 0;

	void setDigMusicState(int stateId);
	void setDigMusicSequence(int seqId);
	void playDigMusic(const char *songName, const imuseDigTable *table, int attribPos, bool sequence);
	void setComiMusicState(int stateId);
	void setComiMusicSequence(int seqId);
	void playComiMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence);
	
public:
	DiMUSE_v2(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps);
	~DiMUSE_v2() override;

	void startSound(int sound) override
	{ error("DiMUSE_v2::startSound(int) should be never called"); }

	void setMusicVolume(int vol) override {}
	void stopSound(int sound) override;
	void stopAllSounds() override;

	int getSoundStatus(int sound) const override { return 0; };
	int isSoundRunning(int soundId); // Needed because getSoundStatus is a const function, and I needed a workaround

	int startVoice(int soundId, Audio::AudioStream *input) override { return 0; };
	int startVoice(int soundId, const char *soundName) override;
	void saveLoadEarly(Common::Serializer &ser) override {};
	void resetState() override {};
	void setRadioChatterSFX(bool state) override;
	void setAudioNames(int32 num, char *names) override {};
	int  startSfx(int soundId, int priority) override;
	void setPriority(int soundId, int priority) override {};
	void setVolume(int soundId, int volume) override;
	void setPan(int soundId, int pan) override;
	void setFrequency(int soundId, int frequency) override;
	int  getCurSpeechVolume() override;
	int  getCurSpeechPan() override;
	int  getCurSpeechFrequency() override;
	void pause(bool pause) override;
	void parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) override;
	void refreshScripts() override;
	void flushTracks() override;

	int32 getCurMusicPosInMs() override;
	int32 getCurVoiceLipSyncWidth() override;
	int32 getCurVoiceLipSyncHeight() override;
	int32 getCurMusicLipSyncWidth(int syncId) override;
	int32 getCurMusicLipSyncHeight(int syncId) override;
	void getSpeechLipSyncInfo(int32 *width, int32 *height);
	void getMusicLipSyncInfo(int syncId, int32 *width, int32 *height);

	uint8 *diMUSE_audioBuffer;
	int diMUSE_feedSize = 512;
	int diMUSE_sampleRate = 22050;

	// General
	int diMUSE_terminate();
	int diMUSE_initialize();
	int diMUSE_pause();
	int diMUSE_resume();
	int diMUSE_save();
	int diMUSE_restore();
	int diMUSE_setGroupVol(int groupId, int volume);
	int diMUSE_startSound(int soundId, int priority);
	int diMUSE_stopSound(int soundId);
	int diMUSE_stopAllSounds();
	int diMUSE_getNextSound(int soundId);
	int diMUSE_setParam(int soundId, int paramId, int value);
	int diMUSE_getParam(int soundId, int paramId);
	int diMUSE_fadeParam(int soundId, int opcode, int destValue, int fadeLength, int oneShot = 0);
	int diMUSE_setHook(int soundId, int hookId);
	int diMUSE_setTrigger(int soundId, int marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n);
	int diMUSE_startStream(int soundId, int priority, int groupId);
	int diMUSE_switchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1);
	int diMUSE_processStreams();
	int diMUSE_queryStream();
	int diMUSE_feedStream();
	int diMUSE_lipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height);
	int diMUSE_setGroupVol_Music(int volume);
	int diMUSE_setGroupVol_SFX(int volume);
	int diMUSE_setGroupVol_Voice(int volume);
	int diMUSE_getGroupVol_Music();
	int diMUSE_getGroupVol_SFX();
	int diMUSE_getGroupVol_Voice();
	int diMUSE_initializeScript();
	int diMUSE_terminateScript();
	int diMUSE_saveScript();
	int diMUSE_restoreScript();
	void diMUSE_refreshScript();
	int diMUSE_setState(int soundId);
	int diMUSE_setSequence(int soundId);
	int diMUSE_setCuePoint();
	int diMUSE_setAttribute(int attrIndex, int attrVal);

	// Utils
	int diMUSE_addTrackToList(DiMUSETrack **listPtr, DiMUSETrack *listPtr_Item);
	int diMUSE_removeTrackFromList(DiMUSETrack **listPtr, DiMUSETrack *itemPtr);
	int diMUSE_addStreamZoneToList(DiMUSEStreamZone **listPtr, DiMUSEStreamZone *listPtr_Item);
	int diMUSE_removeStreamZoneFromList(DiMUSEStreamZone **listPtr, DiMUSEStreamZone *itemPtr);
	int diMUSE_SWAP32(uint8 *value);
	void diMUSE_strcpy(char *dst, char *marker);
	int diMUSE_strcmp(char *marker1, char *marker2);
	int diMUSE_strlen(char *marker);
	int diMUSE_clampNumber(int value, int minValue, int maxValue);
	int diMUSE_clampTuning(int value, int minValue, int maxValue);
	int diMUSE_checkHookId(int *trackHookId, int sampleHookId);

	// Script
	int script_parse(int a1, int a0, int param1, int param2, int param3, int param4, int param5, int param6, int param7);
	int script_init();
	int script_terminate();
	int script_save();
	int script_restore();
	void script_refresh();
	void script_setState(int soundId);
	void script_setSequence(int soundId);
	int script_setCuePoint();
	int script_setAttribute(int attrIndex, int attrVal);
	int script_callback(char *marker);

	// CMDs
	int cmd_pauseCount;
	int cmd_hostIntHandler;
	int cmd_hostIntUsecCount;
	int cmd_runningHostCount;
	int cmd_running60HzCount;
	int cmd_running10HzCount;

	void diMUSEHeartbeat();
	int cmds_handleCmds(int cmd, int b, int c, int d, int e, int f,
		int g, int h, int i, int j, int k,
		int l, int m, int n, int o);

	int cmds_init();
	int cmds_deinit();
	int cmds_terminate();
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

	// Streamer
	DiMUSEStream streamer_streams[MAX_STREAMS];
	DiMUSEStream *streamer_lastStreamLoaded;
	int streamer_bailFlag;

	int streamer_moduleInit();
	DiMUSEStream *streamer_alloc(int soundId, int bufId, int maxRead);
	int streamer_clearSoundInStream(DiMUSEStream *streamPtr);
	int streamer_processStreams();
	uint8 *streamer_reAllocReadBuffer(DiMUSEStream *streamPtr, /*unsigned*/int reallocSize);
	uint8 *streamer_copyBufferAbsolute(DiMUSEStream *streamPtr, int offset, int size);
	int streamer_setIndex1(DiMUSEStream *streamPtr, /*unsigned*/ int offset);
	int streamer_setIndex2(DiMUSEStream *streamPtr, /*unsigned*/ int offset);
	int streamer_getFreeBuffer(DiMUSEStream *streamPtr);
	int streamer_setSoundToStreamWithCurrentOffset(DiMUSEStream *streamPtr, int soundId, int currentOffset);
	int streamer_queryStream(DiMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int streamer_feedStream(DiMUSEStream *streamPtr, uint8 *srcBuf, /*unsigned*/ int sizeToFeed, int paused);
	int streamer_fetchData(DiMUSEStream *streamPtr);

	DiMUSESoundBuffer _soundBuffers[4];

	void DiMUSE_allocSoundBuffer(int bufId, int size, int loadSize, int criticalSize);
	void DiMUSE_deallocSoundBuffer(int bufId);

	// Tracks
	DiMUSETrack tracks[MAX_TRACKS];
	DiMUSETrack *tracks_trackList;

	int tracks_trackCount;
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
	void tracks_clear(DiMUSETrack *trackPtr);
	int tracks_setParam(int soundId, int opcode, int value);
	int tracks_getParam(int soundId, int opcode);
	int tracks_lipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height);
	int tracks_setHook(int soundId, int hookId);
	int tracks_getHook(int soundId);
	void tracks_free();

	int tracks_debug();

	// Dispatch
	DiMUSEDispatch dispatches[MAX_DISPATCHES];
	DiMUSEStreamZone streamZones[MAX_STREAMZONES];
	uint8 *dispatch_buf;
	int dispatch_size;
	uint8 *dispatch_smallFadeBufs;
	uint8 *dispatch_largeFadeBufs;
	int dispatch_fadeSize;
	int dispatch_largeFadeFlags[LARGE_FADES];
	int dispatch_smallFadeFlags[SMALL_FADES];
	int dispatch_fadeStartedFlag;
	int dispatch_bufferedHookId;
	int dispatch_requestedFadeSize;
	int dispatch_curStreamBufSize;
	int dispatch_curStreamCriticalSize;
	int dispatch_curStreamFreeSpace;
	int dispatch_curStreamPaused;

	int dispatch_moduleInit();
	DiMUSEDispatch *dispatch_getDispatchByTrackId(int trackId);
	int dispatch_save(uint8 *dst, int size);
	int dispatch_restore(uint8 *src);
	int dispatch_allocStreamZones();
	int dispatch_alloc(DiMUSETrack *trackPtr, int groupId);
	int dispatch_release(DiMUSETrack *trackPtr);
	int dispatch_switchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag);
	void dispatch_processDispatches(DiMUSETrack *trackPtr, int feedSize, int sampleRate);
	void dispatch_predictFirstStream();
	int dispatch_getNextMapEvent(DiMUSEDispatch *dispatchPtr);
	int dispatch_convertMap(uint8 *rawMap, uint8 *destMap);
	void dispatch_predictStream(DiMUSEDispatch *dispatch);
	void dispatch_parseJump(DiMUSEDispatch *dispatchPtr, DiMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent);
	DiMUSEStreamZone *dispatch_allocStreamZone();
	void dispatch_free();

	// Files
	char _currentSpeechFile[60];
	int files_moduleInit();
	int files_moduleDeinit();
	uint8 *files_getSoundAddrData(int soundId);
	int files_fetchMap(int soundId);
	int files_getNextSound(int soundId);
	int files_checkRange(int soundId);
	int files_seek(int soundId, int offset, int mode, int bufId);
	int files_read(int soundId, uint8 *buf, int size, int bufId);
	void files_getFilenameFromSoundId(int soundId, char *fileName);
	DiMUSESoundBuffer *files_getBufInfo(int bufId);
	void files_openSound(int soundId);
	void files_closeSound(int soundId);
	void files_closeAllSounds();

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
	int wave_startStream(int soundId, int priority, int groupId);
	int wave_switchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1);
	int wave_processStreams();
	int wave_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int wave_feedStream(int soundId, int *srcBuf, int sizeToFeed, int paused);
	int wave_lipSync(int soundId, int syncId, int msPos, int *width, int *height);

	// Waveapi
	waveOutParams waveapi_waveOutParams;

	int waveapi_sampleRate;
	int waveapi_bytesPerSample;
	int waveapi_numChannels;
	int waveapi_zeroLevel;
	int waveapi_preferredFeedSize = 0;
	uint8 *waveapi_mixBuf;
	uint8 *waveapi_outBuf;

	int waveapi_xorTrigger = 0;
	int waveapi_writeIndex = 0;
	int waveapi_disableWrite = 0;

	int waveapi_moduleInit(int sampleRate, waveOutParams *waveoutParamStruct);
	void waveapi_write(uint8 **lpData, int *feedSize, int *sampleRate);
	int waveapi_free();
	void waveapi_callback();
	void waveapi_increaseSlice();
	void waveapi_decreaseSlice();
};

} // End of namespace Scumm

#endif
