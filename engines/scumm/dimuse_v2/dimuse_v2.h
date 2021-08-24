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

class DiMUSE_v2 : public DiMUSE {
private:

	int _callbackFps;		// value how many times callback needs to be called per second

	Common::Mutex _mutex;
	ScummEngine_v7 *_vm;
	Audio::Mixer *_mixer;

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
	void pause(bool pause) override {};
	void parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h) override {};
	void refreshScripts() override {};
	void flushTracks() override {};

	int32 getCurMusicPosInMs() override { return 0; };
	int32 getCurVoiceLipSyncWidth() override { return 0; };
	int32 getCurVoiceLipSyncHeight() override { return 0; };
	int32 getCurMusicLipSyncWidth(int syncId) override { return 0; };
	int32 getCurMusicLipSyncHeight(int syncId) override { return 0; };
	int32 getSoundElapsedTimeInMs(int soundId) override { return 0; };
};

} // End of namespace Scumm

#endif
