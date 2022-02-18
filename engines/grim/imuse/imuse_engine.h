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

#if !defined(GRIM_IMUSE_H)
#define GRIM_IMUSE_H

#include "common/scummsys.h"
#include "common/mutex.h"
#include "common/serializer.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/imuse/imuse_defs.h"
#include "engines/grim/imuse/imuse_internalmixer.h"
#include "engines/grim/imuse/imuse_groups.h"
#include "engines/grim/imuse/imuse_fades.h"
#include "engines/grim/imuse/imuse_files.h"
#include "engines/grim/imuse/imuse_triggers.h"
#include "engines/grim/imuse/imuse_sndmgr.h"
#include "engines/grim/imuse/imuse_tables.h"

#include "audio/mixer.h"
#include "audio/decoders/raw.h"

namespace Audio {
class AudioStream;
class Mixer;
class QueuingAudioStream;
}

namespace Grim {

enum {
	kTalkSoundID = 10000
};

struct ImuseTable;
class SaveGame;

class Imuse {
private:
	Common::Mutex _mutex;
	Audio::Mixer *_mixer;

	IMuseDigiInternalMixer *_internalMixer;
	IMuseDigiGroupsHandler *_groupsHandler;
	IMuseDigiFadesHandler *_fadesHandler;
	IMuseDigiTriggersHandler *_triggersHandler;
	IMuseDigiFilesHandler *_filesHandler;

	int _callbackFps;
	static void timer_handler(void *refConf);
	void callback();

	bool _isEngineDisabled;
	bool _demo;
	bool _pause; // TODO REMOVE

	// These three are manipulated in the waveOut functions
	uint8 *_outputAudioBuffer;
	int _outputFeedSize;
	int _outputSampleRate;

	int _maxQueuedStreams; // maximum number of streams which can be queued before they are played

	int _currentSpeechVolume, _currentSpeechFrequency, _currentSpeechPan;
	int _curMixerMusicVolume, _curMixerSpeechVolume, _curMixerSFXVolume;

	const ImuseTable *_stateMusicTable;
	const ImuseTable *_seqMusicTable;

	int32 _attributes[228];	// internal attributes for each music file to store and check later
	int32 _nextSeqToPlay;
	int32 _curMusicState;
	int32 _curMusicSeq;
	int32 _curMusicCue;

	int _stopSequenceFlag;
	int _scriptInitializedFlag;
	char _emptyMarker[1];

	int _usecPerInt; // Microseconds between each callback (will be set to 50 Hz)
	int _callbackInterruptFlag;
	void diMUSEHeartbeat();

	void playMusic(const ImuseTable *table, int attribPos, bool sequence);

	// Script
	int scriptParse(int cmd, int a, int b);
	int scriptInit();
	int scriptTerminate();
	void scriptRefresh();
	void scriptSetState(int soundId);
	void scriptSetSequence(int soundId);
	int scriptSetAttribute(int attrIndex, int attrVal);

	// CMDs
	int _cmdsPauseCount;
	int _cmdsRunning50HzCount;
	int _cmdsRunning5HzCount;

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
	int cmdsSetParam(int soundId, int transitionType, int value);
	int cmdsGetParam(int soundId, int transitionType);
	int cmdsSetHook(int soundId, int hookId);
	int cmdsGetHook(int soundId);

	// Streamer
	IMuseDigiStream _streams[DIMUSE_MAX_STREAMS];
	IMuseDigiStream *_lastStreamLoaded;
	int _streamerBailFlag;

	int streamerInit();
	IMuseDigiStream *streamerAllocateSound(int soundId, int bufId, int32 maxRead);
	int streamerClearSoundInStream(IMuseDigiStream *streamPtr);
	int streamerProcessStreams();
	uint8 *streamerGetStreamBuffer(IMuseDigiStream *streamPtr, int size);
	uint8 *streamerGetStreamBufferAtOffset(IMuseDigiStream *streamPtr, int32 offset, int size);
	int streamerSetReadIndex(IMuseDigiStream *streamPtr, int offset);
	int streamerSetLoadIndex(IMuseDigiStream *streamPtr, int offset);
	int streamerGetFreeBufferAmount(IMuseDigiStream *streamPtr);
	int streamerSetSoundToStreamFromOffset(IMuseDigiStream *streamPtr, int soundId, int32 offset);
	int streamerQueryStream(IMuseDigiStream *streamPtr, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused);
	int streamerFeedStream(IMuseDigiStream *streamPtr, uint8 *srcBuf, int32 sizeToFeed, int paused);
	int streamerFetchData(IMuseDigiStream *streamPtr);

	// Tracks
	IMuseDigiTrack _tracks[DIMUSE_MAX_TRACKS];
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
	void tracksQueryStream(int soundId, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused);
	int tracksFeedStream(int soundId, uint8 *srcBuf, int32 sizeToFeed, int paused);
	void tracksClear(IMuseDigiTrack *trackPtr);
	int tracksSetParam(int soundId, int transitionType, int value);
	int tracksGetParam(int soundId, int transitionType);
	int tracksSetHook(int soundId, int hookId);
	int tracksGetHook(int soundId);
	IMuseDigiTrack *tracksReserveTrack(int priority);
	void tracksDeinit();

	// Dispatch
	IMuseDigiDispatch _dispatches[DIMUSE_MAX_DISPATCHES];
	IMuseDigiStreamZone _streamZones[DIMUSE_MAX_STREAMZONES];
	uint8 *_dispatchBuffer;
	uint8 _ftCrossfadeBuffer[30000]; // Used by FT & DIG demo
	int32 _dispatchSize;
	uint8 *_dispatchSmallFadeBufs;
	uint8 *_dispatchLargeFadeBufs;
	int32 _dispatchFadeSize;
	int _dispatchLargeFadeFlags[DIMUSE_LARGE_FADES];
	int _dispatchSmallFadeFlags[DIMUSE_SMALL_FADES];
	int _dispatchFadeStartedFlag;
	int _dispatchBufferedHookId;
	int32 _dispatchJumpFadeSize;
	int32 _dispatchCurStreamBufSize;
	int32 _dispatchCurStreamCriticalSize;
	int32 _dispatchCurStreamFreeSpace;
	int _dispatchCurStreamPaused;

	int dispatchInit();
	IMuseDigiDispatch *dispatchGetDispatchByTrackId(int trackId);
	void dispatchSaveLoad(Common::Serializer &ser);
	int dispatchRestoreStreamZones();
	int dispatchAllocateSound(IMuseDigiTrack *trackPtr, int groupId);
	int dispatchRelease(IMuseDigiTrack *trackPtr);
	int dispatchSwitchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag);
	void dispatchProcessDispatches(IMuseDigiTrack *trackPtr, int feedSize, int sampleRate);
	void dispatchPredictFirstStream();
	int dispatchNavigateMap(IMuseDigiDispatch *dispatchPtr);
	int dispatchGetMap(IMuseDigiDispatch *dispatchPtr);
	int dispatchConvertMap(uint8 *rawMap, uint8 *destMap);
	int32 *dispatchGetNextMapEvent(int32 *mapPtr, int32 soundOffset, int32 *mapEvent);
	void dispatchPredictStream(IMuseDigiDispatch *dispatchPtr);
	int32 *dispatchCheckForJump(int32 *mapPtr, IMuseDigiStreamZone *strZnPtr, int &candidateHookId);
	void dispatchPrepareToJump(IMuseDigiDispatch *dispatchPtr, IMuseDigiStreamZone *strZnPtr, int32 *jumpParamsFromMap, int calledFromGetNextMapEvent);
	void dispatchStreamNextZone(IMuseDigiDispatch *dispatchPtr, IMuseDigiStreamZone *strZnPtr);
	IMuseDigiStreamZone *dispatchAllocateStreamZone();
	uint8 *dispatchAllocateFade(int32 &fadeSize, const char *functionName);
	void dispatchDeallocateFade(IMuseDigiDispatch *dispatchPtr, const char *functionName);
	int dispatchGetFadeSize(IMuseDigiDispatch *dispatchPtr, int fadeLength);
	int dispatchUpdateFadeMixVolume(IMuseDigiDispatch *dispatchPtr, int32 remainingFade);
	int dispatchUpdateFadeSlope(IMuseDigiDispatch *dispatchPtr);

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
	int waveSetParam(int soundId, int transitionType, int value);
	int waveGetParam(int soundId, int transitionType);
	int waveSetHook(int soundId, int hookId);
	int waveGetHook(int soundId);
	int waveStartStream(int soundId, int priority, int groupId);
	int waveSwitchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1);
	int waveProcessStreams();
	void waveQueryStream(int soundId, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused);
	int waveFeedStream(int soundId, uint8 *srcBuf, int32 sizeToFeed, int paused);

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
	void waveOutWrite(uint8 **audioBuffer, int &feedSize, int &sampleRate);
	int waveOutDeinit();
	void waveOutCallback();
	byte waveOutGetStreamFlags();

public:
	Imuse(int fps, bool demo, Audio::Mixer *mixer);
	~Imuse();

	// Wrapper functions used by the main engine

	//void startSound(int sound) override { error("Imuse::startSound(int) should be never called"); }
	//void setMusicVolume(int vol) override {}
	//void stopSound(int sound) override;
	//void stopAllSounds() override;
	//int getSoundStatus(int sound) const override { return 0; }
	int isSoundRunning(int soundId); // Needed because getSoundStatus is a const function, and I needed a workaround
	int startVoice(int soundId, const char *soundName, byte speakingActorId);
	void restoreState(SaveGame *savedState);
	void saveState(SaveGame *savedState);
	void resetState();
	void setAudioNames(int32 num, char *names);
	int  startSfx(int soundId, int priority) ;
	void setPriority(int soundId, int priority);
	void setVolume(int soundId, int volume);
	void setPan(int soundId, int pan);
	void setFrequency(int soundId, int frequency);
	int  getCurSpeechVolume() const;
	int  getCurSpeechPan() const;
	int  getCurSpeechFrequency() const;
	void pause(bool pause);
	void refreshScripts();
	void flushTracks();
	void disableEngine();
	bool isEngineDisabled();
	void stopSMUSHAudio();

	int32 getCurMusicPosInMs();
	int32 getCurVoiceLipSyncWidth();
	int32 getCurVoiceLipSyncHeight();
	int32 getCurMusicLipSyncWidth(int syncId);
	int32 getCurMusicLipSyncHeight(int syncId);
	void getSpeechLipSyncInfo(int32 &width, int32 &height);
	void getMusicLipSyncInfo(int syncId, int32 &width, int32 &height);
	int32 getSoundElapsedTimeInMs(int soundId);

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
	int diMUSEFadeParam(int soundId, int transitionType, int destValue, int fadeLength);
	int diMUSESetHook(int soundId, int hookId);
	int diMUSESetTrigger(int soundId, int marker, int transitionType, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n);
	int diMUSEStartStream(int soundId, int priority, int groupId);
	int diMUSESwitchStream(int oldSoundId, int newSoundId, int fadeDelay, int fadeSyncFlag2, int fadeSyncFlag1);
	int diMUSEProcessStreams();
	void diMUSEQueryStream(int soundId, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused);
	int diMUSEFeedStream(int soundId, uint8 *srcBuf, int32 sizeToFeed, int paused);
	int diMUSESetMusicGroupVol(int volume);
	int diMUSESetSFXGroupVol(int volume);
	int diMUSESetVoiceGroupVol(int volume);
	void diMUSEUpdateGroupVolumes();
	int diMUSEInitializeScript();
	void diMUSERefreshScript();
	int diMUSESetState(int soundId);
	int diMUSESetSequence(int soundId);
	int diMUSESetCuePoint(int cueId);
	int diMUSESetAttribute(int attrIndex, int attrVal);

	// Utils
	int addTrackToList(IMuseDigiTrack **listPtr, IMuseDigiTrack *listPtr_Item);
	int removeTrackFromList(IMuseDigiTrack **listPtr, IMuseDigiTrack *itemPtr);
	int addStreamZoneToList(IMuseDigiStreamZone **listPtr, IMuseDigiStreamZone *listPtr_Item);
	int removeStreamZoneFromList(IMuseDigiStreamZone **listPtr, IMuseDigiStreamZone *itemPtr);
	int clampNumber(int value, int minValue, int maxValue);
	int clampTuning(int value, int minValue, int maxValue);
	int checkHookId(int &trackHookId, int sampleHookId);

	// CMDs
	int cmdsHandleCmd(int cmd, uint8 *ptr = nullptr,
		int a = -1, int b = -1, int c = -1, int d = -1, int e = -1,
		int f = -1, int g = -1, int h = -1, int i = -1, int j = -1,
		int k = -1, int l = -1, int m = -1, int n = -1);

	// Script
	int scriptTriggerCallback(char *marker);

	// Debugger utility functions
	void listStates();
	void listSeqs();
	void listCues();
	void listTracks();
	void listGroups();
};

extern Imuse *g_imuse;

} // End of namespace Grim

#endif
