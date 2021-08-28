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

#if !defined(SCUMM_IMUSE_DIGI_H) && defined(ENABLE_SCUMM_7_8)
#define SCUMM_IMUSE_DIGI_H

#include "common/scummsys.h"
#include "common/mutex.h"
#include "common/serializer.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "scumm/scumm_v7.h"
#include "scumm/music.h"
#include "scumm/sound.h"

namespace Audio {
	class AudioStream;
	class Mixer;
}

namespace Scumm {

	struct imuseDigTable;
	struct imuseComiTable;
	class ScummEngine_v7;
	struct Track;

class DiMUSE : public MusicEngine {
private:

	int _callbackFps;		// value how many times callback needs to be called per second

	Common::Mutex _mutex;
	ScummEngine_v7 *_vm;
	Audio::Mixer *_mixer;

	static void timer_handler(void *refConf);
	void callback();

public:
	DiMUSE(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps) {};
	~DiMUSE() {};

	void startSound(int sound) override
	{ error("DiMUSE::startSound(int) should be never called"); }

	void setMusicVolume(int vol) override {}
	void stopSound(int sound) override {};
	void stopAllSounds() override {};
	virtual int getSoundStatus(int sound) const { return 0; };
	virtual int isSoundRunning(int sound) { return 0; };
	virtual int startVoice(int soundId, Audio::AudioStream *input) { return 0; };
	virtual int startVoice(int soundId, const char *soundName) { return 0; };
	virtual void saveLoadEarly(Common::Serializer &ser) {};
	virtual void resetState() {};
	virtual void setRadioChatterSFX(bool state) {};
	virtual void setAudioNames(int32 num, char *names) {};
	virtual int  startSfx(int soundId, int priority) { return 0; };
	virtual void setPriority(int soundId, int priority) {};
	virtual void setVolume(int soundId, int volume) {};
	virtual void setPan(int soundId, int pan) {};
	virtual void setFrequency(int soundId, int value) {};
	virtual int  getCurSpeechVolume() { return 0; };
	virtual int  getCurSpeechPan() { return 0; };
	virtual int  getCurSpeechFrequency() { return 0; };
	virtual void pause(bool pause) {};
	virtual void parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) {};
	virtual void refreshScripts() {};
	virtual void flushTracks() {};

	virtual int32 getCurMusicPosInMs() { return 0; };
	virtual int32 getCurVoiceLipSyncWidth() { return 0; };
	virtual int32 getCurVoiceLipSyncHeight() { return 0; };
	virtual int32 getCurMusicLipSyncWidth(int syncId) { return 0; };
	virtual int32 getCurMusicLipSyncHeight(int syncId) { return 0; };
	virtual int32 getSoundElapsedTimeInMs(int soundId) { return 0; };
};

} // End of namespace Scumm

#endif
