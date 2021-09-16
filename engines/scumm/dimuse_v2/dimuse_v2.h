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
#include "scumm/dimuse_v2/dimuse_v2_files.h"
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

	DiMUSEInternalMixer *_internalMixer;
	DiMUSEGroupsHandler *_groupsHandler;
	DiMUSETimerHandler *_timerHandler;
	DiMUSEFadesHandler *_fadesHandler;
	DiMUSETriggersHandler *_triggersHandler;
	DiMUSEFilesHandler *_filesHandler;

	int _callbackFps;
	static void timer_handler(void *refConf);
	void callback();

	// These three are manipulated in the waveOut functions
	uint8 *_outputAudioBuffer;
	int _outputFeedSize;
	int _outputSampleRate;

	int _currentSpeechVolume, _currentSpeechFrequency, _currentSpeechPan;
	int _curMixerMusicVolume, _curMixerSpeechVolume, _curMixerSFXVolume;
	bool _radioChatterSFX;

	int32 _attributes[188];	// internal attributes for each music file to store and check later
	int32 _nextSeqToPlay;	// id of sequence type of music needed played
	int32 _curMusicState;	// current or previous id of music
	int32 _curMusicSeq;		// current or previous id of sequence music
	int _stopSequenceFlag;
	int _scriptInitializedFlag;

	void diMUSEHeartbeat();

	void setDigMusicState(int stateId);
	void setDigMusicSequence(int seqId);
	void playDigMusic(const char *songName, const imuseDigTable *table, int attribPos, bool sequence);
	void setComiMusicState(int stateId);
	void setComiMusicSequence(int seqId);
	void playComiMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence);

	// Script
	int scriptParse(int cmd, int a, int b);
	int scriptInit();
	int scriptTerminate();
	int scriptSave();
	int scriptRestore();
	void scriptRefresh();
	void scriptSetState(int soundId);
	void scriptSetSequence(int soundId);
	int scriptSetCuePoint();
	int scriptSetAttribute(int attrIndex, int attrVal);

	// CMDs
	int _cmdsPauseCount;
	int _cmdsRunning60HzCount;
	int _cmdsRunning10HzCount;

	int cmdsInit();
	int cmdsDeinit();
	int cmdsTerminate();
	int cmdsPause();
	int cmdsResume();
	void cmdsSaveLoad(Common::Serializer &ser);
	int cmdsStartSound(int soundId, int priority);
	int cmdsStopSound(int soundId);
	int cmdsStopAllSounds();
	int cmdsGetNextSound(int soundId);
	int cmdsSetParam(int soundId, int opcode, int value);
	int cmdsGetParam(int soundId, int opcode);
	int cmdsSetHook(int soundId, int hookId);
	int cmdsGetHook(int soundId);

	// Streamer
	DiMUSEStream _streams[MAX_STREAMS];
	DiMUSEStream *_lastStreamLoaded;
	int _streamerBailFlag;

	int streamerInit();
	DiMUSEStream *streamerAllocateSound(int soundId, int bufId, int maxRead);
	int streamerClearSoundInStream(DiMUSEStream *streamPtr);
	int streamerProcessStreams();
	uint8 *streamerReAllocReadBuffer(DiMUSEStream *streamPtr, int reallocSize);
	uint8 *streamerCopyBufferAbsolute(DiMUSEStream *streamPtr, int offset, int size);
	int streamerSetIndex1(DiMUSEStream *streamPtr, int offset);
	int streamerSetIndex2(DiMUSEStream *streamPtr, int offset);
	int streamerGetFreeBuffer(DiMUSEStream *streamPtr);
	int streamerSetSoundToStreamWithCurrentOffset(DiMUSEStream *streamPtr, int soundId, int currentOffset);
	int streamerQueryStream(DiMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int streamerFeedStream(DiMUSEStream *streamPtr, uint8 *srcBuf, int sizeToFeed, int paused);
	int streamerFetchData(DiMUSEStream *streamPtr);

	// Tracks
	DiMUSETrack _tracks[MAX_TRACKS];
	DiMUSETrack *_trackList;

	int _trackCount;
	int _tracksPauseTimer;
	int _tracksPrefSampleRate;
	int _tracksMicroSecsToFeed;

	int tracksInit();
	void tracksPause();
	void tracksResume();
	void tracksSaveLoad(Common::Serializer &ser);
	void tracksSetGroupVol();
	void tracksCallback();
	int tracksStartSound(int soundId, int tryPriority, int group);
	int tracksStopSound(int soundId);
	int tracksStopAllSounds();
	int tracksGetNextSound(int soundId);
	int tracksQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int tracksFeedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused);
	void tracksClear(DiMUSETrack *trackPtr);
	int tracksSetParam(int soundId, int opcode, int value);
	int tracksGetParam(int soundId, int opcode);
	int tracksLipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height);
	int tracksSetHook(int soundId, int hookId);
	int tracksGetHook(int soundId);
	void tracksDeinit();

	// Dispatch
	DiMUSEDispatch _dispatches[MAX_DISPATCHES];
	DiMUSEStreamZone _streamZones[MAX_STREAMZONES];
	uint8 *_dispatchBuffer;
	int _dispatchSize;
	uint8 *_dispatchSmallFadeBufs;
	uint8 *_dispatchLargeFadeBufs;
	int _dispatchFadeSize;
	int _dispatchLargeFadeFlags[LARGE_FADES];
	int _dispatchSmallFadeFlags[SMALL_FADES];
	int _dispatchFadeStartedFlag;
	int _dispatchBufferedHookId;
	int _dispatchRequestedFadeSize;
	int _dispatchCurStreamBufSize;
	int _dispatchCurStreamCriticalSize;
	int _dispatchCurStreamFreeSpace;
	int _dispatchCurStreamPaused;

	int dispatchInit();
	DiMUSEDispatch *dispatchGetDispatchByTrackId(int trackId);
	void dispatchSaveLoad(Common::Serializer &ser);
	int dispatchAllocStreamZones();
	int dispatchAlloc(DiMUSETrack *trackPtr, int groupId);
	int dispatchRelease(DiMUSETrack *trackPtr);
	int dispatchSwitchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag);
	void dispatchProcessDispatches(DiMUSETrack *trackPtr, int feedSize, int sampleRate);
	void dispatchPredictFirstStream();
	int dispatchGetNextMapEvent(DiMUSEDispatch *dispatchPtr);
	int dispatchConvertMap(uint8 *rawMap, uint8 *destMap);
	void dispatchPredictStream(DiMUSEDispatch *dispatch);
	void dispatchParseJump(DiMUSEDispatch *dispatchPtr, DiMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent);
	DiMUSEStreamZone *dispatchAllocStreamZone();

	// Wave (mainly a wrapper for Tracks functions)
	int _waveSlicingHalted;

	int waveInit();
	int waveTerminate();
	int wavePause();
	int waveResume();
	void waveSaveLoad(Common::Serializer &ser);
	void waveUpdateGroupVolumes();
	int waveStartSound(int soundId, int priority);
	int waveStopSound(int soundId);
	int waveStopAllSounds();
	int waveGetNextSound(int soundId);
	int waveSetParam(int soundId, int opcode, int value);
	int waveGetParam(int soundId, int opcode);
	int waveSetHook(int soundId, int hookId);
	int waveGetHook(int soundId);
	int waveStartStream(int soundId, int priority, int groupId);
	int waveSwitchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1);
	int waveProcessStreams();
	int waveQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int waveFeedStream(int soundId, int *srcBuf, int sizeToFeed, int paused);
	int waveLipSync(int soundId, int syncId, int msPos, int *width, int *height);

	// Waveapi
	waveOutParamsStruct waveOutSettings;

	int _waveOutSampleRate;
	int _waveOutBytesPerSample;
	int _waveOutNumChannels;
	int _waveOutZeroLevel;
	int _waveOutPreferredFeedSize;
	uint8 *_waveOutMixBuffer;
	uint8 *_waveOutOutputBuffer;

	int _waveOutXorTrigger;
	int _waveOutWriteIndex;
	int _waveOutDisableWrite;

	int waveOutInit(int sampleRate, waveOutParamsStruct *waveOutSettings);
	void waveOutWrite(uint8 **audioBuffer, int *feedSize, int *sampleRate);
	int waveOutDeinit();
	void waveOutCallback();
	void waveOutIncreaseSlice();
	void waveOutDecreaseSlice();
public:
	DiMUSE_v2(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps);
	~DiMUSE_v2() override;

	// Wrapper functions used by the main engine

	void startSound(int sound) override { error("DiMUSE_v2::startSound(int) should be never called"); }
	void setMusicVolume(int vol) override {}
	void stopSound(int sound) override;
	void stopAllSounds() override;
	int getSoundStatus(int sound) const override { return 0; }
	int isSoundRunning(int soundId); // Needed because getSoundStatus is a const function, and I needed a workaround
	int startVoice(int soundId, Audio::AudioStream *input) override { return 0; }
	int startVoice(int soundId, const char *soundName) override;
	void saveLoadEarly(Common::Serializer &ser) override;
	void resetState() override {};
	void setRadioChatterSFX(bool state) override;
	void setAudioNames(int32 num, char *names) override {};
	int  startSfx(int soundId, int priority) override;
	void setPriority(int soundId, int priority) override {};
	void setVolume(int soundId, int volume) override;
	void setPan(int soundId, int pan) override;
	void setFrequency(int soundId, int frequency) override;
	int  getCurSpeechVolume() const override;
	int  getCurSpeechPan() const override;
	int  getCurSpeechFrequency() const override;
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

	// General engine functions
	int diMUSETerminate();
	int diMUSEInitialize();
	int diMUSEPause();
	int diMUSEResume();
	void diMUSESaveLoad(Common::Serializer &ser);
	int diMUSESetGroupVol(int groupId, int volume);
	int diMUSEStartSound(int soundId, int priority);
	int diMUSEStopSound(int soundId);
	int diMUSEStopAllSounds();
	int diMUSEGetNextSound(int soundId);
	int diMUSESetParam(int soundId, int paramId, int value);
	int diMUSEGetParam(int soundId, int paramId);
	int diMUSEFadeParam(int soundId, int opcode, int destValue, int fadeLength, int oneShot = 0);
	int diMUSESetHook(int soundId, int hookId);
	int diMUSESetTrigger(int soundId, int marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n);
	int diMUSEStartStream(int soundId, int priority, int groupId);
	int diMUSESwitchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1);
	int diMUSEProcessStreams();
	int diMUSEQueryStream();
	int diMUSEFeedStream();
	int diMUSELipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height);
	int diMUSESetMusicGroupVol(int volume);
	int diMUSESetSFXGroupVol(int volume);
	int diMUSESetVoiceGroupVol(int volume);
	int diMUSEGetMusicGroupVol();
	int diMUSEGetSFXGroupVol();
	int diMUSEGetVoiceGroupVol();
	void diMUSEUpdateGroupVolumes();
	int diMUSEInitializeScript();
	int diMUSETerminateScript();
	int diMUSESaveScript();
	int diMUSERestoreScript();
	void diMUSERefreshScript();
	int diMUSESetState(int soundId);
	int diMUSESetSequence(int soundId);
	int diMUSESetCuePoint();
	int diMUSESetAttribute(int attrIndex, int attrVal);

	// Utils
	int addTrackToList(DiMUSETrack **listPtr, DiMUSETrack *listPtr_Item);
	int removeTrackFromList(DiMUSETrack **listPtr, DiMUSETrack *itemPtr);
	int addStreamZoneToList(DiMUSEStreamZone **listPtr, DiMUSEStreamZone *listPtr_Item);
	int removeStreamZoneFromList(DiMUSEStreamZone **listPtr, DiMUSEStreamZone *itemPtr);
	int clampNumber(int value, int minValue, int maxValue);
	int clampTuning(int value, int minValue, int maxValue);
	int checkHookId(int *trackHookId, int sampleHookId);

	// CMDs
	int cmdsHandleCmd(int cmd, int b, int c, int d, int e, int f,
		int g, int h, int i, int j, int k,
		int l, int m, int n, int o);

	// Script
	int scriptTriggerCallback(char *marker);
};

} // End of namespace Scumm

#endif
