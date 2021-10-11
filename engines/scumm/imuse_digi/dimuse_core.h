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
#include "scumm/imuse_digi/dimuse_core_defs.h"
#include "scumm/imuse_digi/dimuse_core_internalmixer.h"
#include "scumm/imuse_digi/dimuse_core_groups.h"
#include "scumm/imuse_digi/dimuse_core_fades.h"
#include "scumm/imuse_digi/dimuse_core_files.h"
#include "scumm/imuse_digi/dimuse_core_triggers.h"
#include "scumm/imuse_digi/dimuse_bndmgr.h"
#include "scumm/imuse_digi/dimuse_sndmgr.h"
#include "scumm/imuse_digi/dimuse_tables.h"
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

struct IMuseDigiDispatch;
struct IMuseDigiTrack;
struct IMuseDigiStreamZone;

class IMuseDigital : public IMuseDigitalAbstract {
private:
	Common::Mutex _mutex;
	ScummEngine_v7 *_vm;
	Audio::Mixer *_mixer;

	IMuseDigiInternalMixer *_internalMixer;
	IMuseDigiGroupsHandler *_groupsHandler;
	IMuseDigiFadesHandler *_fadesHandler;
	IMuseDigiTriggersHandler *_triggersHandler;
	IMuseDigiFilesHandler *_filesHandler;

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
	char _emptyMarker[1];

	int _usecPerInt; // Microseconds between each callback (will be set to 50 Hz)
	int _callbackInterruptFlag;
	void diMUSEHeartbeat();

	void setDigMusicState(int stateId);
	void setDigMusicSequence(int seqId);
	void playDigMusic(const char *songName, const imuseDigTable *table, int attribPos, bool sequence);
	void setComiMusicState(int stateId);
	void setComiMusicSequence(int seqId);
	void playComiMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence);
	void playComiDemoMusic(const char *songName, const imuseComiTable *table, int attribPos, bool sequence);

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
	IMuseDigiStream _streams[MAX_STREAMS];
	IMuseDigiStream *_lastStreamLoaded;
	int _streamerBailFlag;

	int streamerInit();
	IMuseDigiStream *streamerAllocateSound(int soundId, int bufId, int maxRead);
	int streamerClearSoundInStream(IMuseDigiStream *streamPtr);
	int streamerProcessStreams();
	uint8 *streamerReAllocReadBuffer(IMuseDigiStream *streamPtr, int reallocSize);
	uint8 *streamerCopyBufferAbsolute(IMuseDigiStream *streamPtr, int offset, int size);
	int streamerSetIndex1(IMuseDigiStream *streamPtr, int offset);
	int streamerSetIndex2(IMuseDigiStream *streamPtr, int offset);
	int streamerGetFreeBuffer(IMuseDigiStream *streamPtr);
	int streamerSetSoundToStreamWithCurrentOffset(IMuseDigiStream *streamPtr, int soundId, int currentOffset);
	int streamerQueryStream(IMuseDigiStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int streamerFeedStream(IMuseDigiStream *streamPtr, uint8 *srcBuf, int sizeToFeed, int paused);
	int streamerFetchData(IMuseDigiStream *streamPtr);

	// Tracks
	IMuseDigiTrack _tracks[MAX_TRACKS];
	IMuseDigiTrack *_trackList;

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
	void tracksClear(IMuseDigiTrack *trackPtr);
	int tracksSetParam(int soundId, int opcode, int value);
	int tracksGetParam(int soundId, int opcode);
	int tracksLipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height);
	int tracksSetHook(int soundId, int hookId);
	int tracksGetHook(int soundId);
	void tracksDeinit();

	// Dispatch
	IMuseDigiDispatch _dispatches[MAX_DISPATCHES];
	IMuseDigiStreamZone _streamZones[MAX_STREAMZONES];
	uint8 *_dispatchBuffer;
	int _dispatchSize;
	uint8 *_dispatchSmallFadeBufs;
	uint8 *_dispatchLargeFadeBufs;
	int _dispatchFadeSize;
	int _dispatchLargeFadeFlags[LARGE_FADES];
	int _dispatchSmallFadeFlags[SMALL_FADES];
	int _dispatchFadeStartedFlag;
	int _dispatchBufferedHookId;
	int _dispatchJumpFadeSize;
	int _dispatchCurStreamBufSize;
	int _dispatchCurStreamCriticalSize;
	int _dispatchCurStreamFreeSpace;
	int _dispatchCurStreamPaused;

	int dispatchInit();
	IMuseDigiDispatch *dispatchGetDispatchByTrackId(int trackId);
	void dispatchSaveLoad(Common::Serializer &ser);
	int dispatchRestoreStreamZones();
	int dispatchAlloc(IMuseDigiTrack *trackPtr, int groupId);
	int dispatchRelease(IMuseDigiTrack *trackPtr);
	int dispatchSwitchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag);
	void dispatchProcessDispatches(IMuseDigiTrack *trackPtr, int feedSize, int sampleRate);
	void dispatchPredictFirstStream();
	int dispatchGetNextMapEvent(IMuseDigiDispatch *dispatchPtr);
	int dispatchConvertMap(uint8 *rawMap, uint8 *destMap);
	void dispatchPredictStream(IMuseDigiDispatch *dispatch);
	void dispatchParseJump(IMuseDigiDispatch *dispatchPtr, IMuseDigiStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent);
	IMuseDigiStreamZone *dispatchAllocStreamZone();
	uint8 *dispatchAllocateFade(int *fadeSize, const char *function);
	void dispatchDeallocateFade(IMuseDigiDispatch *dispatchPtr, const char *function);
	void dispatchValidateFade(IMuseDigiDispatch *dispatchPtr, int *dispatchSize, const char *function);
	int dispatchUpdateFadeMixVolume(IMuseDigiDispatch *dispatchPtr, int remainingFade);

	// Wave (mainly a wrapper for Tracks functions)
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
	int waveFeedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused);
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

public:
	IMuseDigital(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps);
	~IMuseDigital() override;

	// Wrapper functions used by the main engine

	void startSound(int sound) override { error("IMuseDigital::startSound(int) should be never called"); }
	void setMusicVolume(int vol) override {}
	void stopSound(int sound) override;
	void stopAllSounds() override;
	int getSoundStatus(int sound) const override { return 0; }
	int isSoundRunning(int soundId); // Needed because getSoundStatus is a const function, and I needed a workaround
	int startVoice(int soundId, Audio::AudioStream *input) override { return 0; }
	int startVoice(int soundId, const char *soundName, byte speakingActorId) override;
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

	bool isUsingV2Engine() override;

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
	int diMUSEFadeParam(int soundId, int opcode, int destValue, int fadeLength);
	int diMUSESetHook(int soundId, int hookId);
	int diMUSESetTrigger(int soundId, int marker, int opcode, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n);
	int diMUSEStartStream(int soundId, int priority, int groupId);
	int diMUSESwitchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1);
	int diMUSEProcessStreams();
	int diMUSEQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
	int diMUSEFeedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused);
	int diMUSELipSync(int soundId, int syncId, int msPos, int32 *width, int32 *height);
	int diMUSESetMusicGroupVol(int volume);
	int diMUSESetSFXGroupVol(int volume);
	int diMUSESetVoiceGroupVol(int volume);
	void diMUSEUpdateGroupVolumes();
	int diMUSEInitializeScript();
	void diMUSERefreshScript();
	int diMUSESetState(int soundId);
	int diMUSESetSequence(int soundId);
	int diMUSESetAttribute(int attrIndex, int attrVal);

	// Utils
	int addTrackToList(IMuseDigiTrack **listPtr, IMuseDigiTrack *listPtr_Item);
	int removeTrackFromList(IMuseDigiTrack **listPtr, IMuseDigiTrack *itemPtr);
	int addStreamZoneToList(IMuseDigiStreamZone **listPtr, IMuseDigiStreamZone *listPtr_Item);
	int removeStreamZoneFromList(IMuseDigiStreamZone **listPtr, IMuseDigiStreamZone *itemPtr);
	int clampNumber(int value, int minValue, int maxValue);
	int clampTuning(int value, int minValue, int maxValue);
	int checkHookId(int *trackHookId, int sampleHookId);

	// CMDs
	int cmdsHandleCmd(int cmd, int b, uintptr c, uintptr d, uintptr e, uintptr f,
		int g, int h, int i, int j, int k,
		int l, int m, int n, int o);

	// Script
	int scriptTriggerCallback(char *marker);
};

} // End of namespace Scumm

#endif
