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

#include "common/system.h"
#include "common/timer.h"

#include "scumm/actor.h"
#include "scumm/scumm_v7.h"
#include "scumm/sound.h"
#include "scumm/dimuse.h"
#include "scumm/dimuse_v2/dimuse_v2.h"
#include "scumm/dimuse_v1/dimuse_bndmgr.h"
#include "scumm/dimuse_v1/dimuse_codecs.h"
#include "scumm/dimuse_v1/dimuse_track.h"
#include "scumm/dimuse_v1/dimuse_tables.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "audio/decoders/raw.h"

namespace Scumm {

void DiMUSE_v2::timer_handler(void *refCon) {
	DiMUSE_v2 *diMUSE = (DiMUSE_v2 *)refCon;
	diMUSE->callback();
}

DiMUSE_v2::DiMUSE_v2(ScummEngine_v7 *scumm, Audio::Mixer *mixer, int fps)
	: _vm(scumm), _mixer(mixer), DiMUSE(scumm, mixer, fps) {
	assert(_vm);
	assert(mixer);
	_callbackFps = fps;
	cmds_init();
	
	_vm->getTimerManager()->installTimerProc(timer_handler, 1000000 / _callbackFps, this, "DiMUSE_v2");
}

DiMUSE_v2::~DiMUSE_v2() {
	_vm->getTimerManager()->removeTimerProc(timer_handler);
	cmds_deinit();
}

void DiMUSE_v2::callback() {
	//Common::StackLock lock(_mutex, "DiMUSE_v2::callback()");
	if (cmd_pauseCount)
		return;

	//debug(5, "DiMUSE_v2::callback()");
	if (!timer_intFlag) {
		timer_intFlag = 1;
		iMUSEHeartbeat();
		timer_intFlag = 0;
	}
}

void DiMUSE_v2::iMUSEHeartbeat() {
	// This is what happens:
	// - Usual audio stuff like fetching and playing sound (and everything 
	//   within waveapi_callback()) happens at a base 50Hz rate;
	// - Triggers and fades handling happens at a (somewhat hacky) 60Hz rate;
	// - Music gain reduction happens at a 10Hz rate.

	int soundId, foundGroupId, musicTargetVolume, musicEffVol, musicVol;

	int usecPerInt = timer_getUsecPerInt(); // Always returns 20000 microseconds (50 Hz)
	//waveapi_callback();

	/*
	if (cmd_hostIntHandler) {
		cmd_hostIntUsecCount += usecPerInt;
		while (cmd_runningHostCount >= cmd_hostIntUsecCount) {
			cmd_runningHostCount -= cmd_hostIntUsecCount;
			cmds_hostIntHandler();
		}
	}*/

	cmd_running60HzCount += usecPerInt;
	while (cmd_running60HzCount >= 16667) {
		cmd_running60HzCount -= 16667;
		//cmd_initDataPtr->num60hzIterations++;
		debug(5, "DiMUSE_v2::iMUSEHeartbeat(): 60Hz interval reached");
		fades_loop();
		triggers_loop();
	}

	cmd_running10HzCount += usecPerInt;
	if (cmd_running10HzCount < 100000)
		return;

	do {
		debug(5, "DiMUSE_v2::iMUSEHeartbeat(): 10Hz interval reached");
		// SPEECH GAIN REDUCTION 10Hz
		cmd_running10HzCount -= 100000;
		soundId = 0;
		musicTargetVolume = groups_setGroupVol(IMUSE_GROUP_MUSIC, -1);
		while (1) { // Check all tracks to see if there's a speech file playing
			soundId = wave_getNextSound(soundId);
			if (!soundId)
				break;

			foundGroupId = -1;
			if (files_getNextSound(soundId) == 2) {
				foundGroupId = wave_getParam(soundId, 0x400); // Check the groupId of this sound
			}

			if (foundGroupId == IMUSE_GROUP_SPEECH) {
				// Remember: when a speech file stops playing this block stops 
				// being executed, so musicTargetVolume returns back to its original value
				musicTargetVolume = (musicTargetVolume * 80) / 128;
				break;
			}
		}

		musicEffVol = groups_setGroupVol(IMUSE_GROUP_MUSICEFF, -1); // MUSIC EFFECTIVE VOLUME GROUP (used for gain reduction)
		musicVol = groups_setGroupVol(IMUSE_GROUP_MUSIC, -1); // MUSIC VOLUME SUBGROUP (keeps track of original music volume)

		if (musicEffVol < musicVol) { // If there is gain reduction already going on...
			musicEffVol += 3;
			if (musicEffVol >= musicTargetVolume) {
				if (musicVol <= musicTargetVolume) {
					musicVol = musicTargetVolume;
				}
			} else { // Bring up the effective music volume immediately when speech stops playing
				if (musicVol <= musicEffVol) {
					musicVol = musicEffVol;
				}
			}
		} else { // Bring music volume down to target volume with a -18 step if there's speech playing
		   // or else, just cap it to the target if it's out of range
			musicEffVol -= 18;
			if (musicEffVol > musicTargetVolume) {
				if (musicVol >= musicEffVol) {
					musicVol = musicEffVol;
				}
			} else {
				if (musicVol >= musicTargetVolume) {
					musicVol = musicTargetVolume;
				}
			}
		}
		groups_setGroupVol(IMUSE_GROUP_MUSICEFF, musicVol);
	} while (cmd_running10HzCount >= 100000);
}

void DiMUSE_v2::pause(bool p) {
	if (p) {
		debug(5, "DiMUSE_v2::pause(): pausing...");
		cmds_pause();
	} else {
		debug(5, "DiMUSE_v2::pause(): resuming...");
		cmds_resume();
	}
}

void DiMUSE_v2::parseScriptCmds(int cmd, int soundId, int sub_cmd, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m, int n, int o, int p) {
	switch (cmd) {
	case 0x1000:
		// SetState
		break;
	case 0x1001:
		// SetSequence
		break;
	case 0x1002:
		// SetCuePoint
		break;
	case 0x1003:
		// SetAttribute
		break;
	case 0x2000:
		// SetGroupSfxVolume
		break;
	case 0x2001:
		// SetGroupVoiceVolume
		break;
	case 0x2002:
		// SetGroupMusicVolume
		break;
	case 10: // StopAllSounds
	case 12: // SetParam
	case 14: // FadeParam
		cmds_handleCmds(cmd, soundId, sub_cmd, d, e, f, g, h, i, j, k, l, m, n, o);
		break;
	default:
		error("DiMUSE_v2::doCommand DEFAULT command %d", cmd);
	}
		
	
};

} // End of namespace Scumm
