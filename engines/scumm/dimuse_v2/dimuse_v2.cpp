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
	_vm->getTimerManager()->installTimerProc(timer_handler, 1000000 / _callbackFps, this, "DiMUSE_v2");
}

DiMUSE_v2::~DiMUSE_v2() {
	_vm->getTimerManager()->removeTimerProc(timer_handler);
}

void DiMUSE_v2::callback() {
	Common::StackLock lock(_mutex, "DiMUSE_v2::callback()");
	debug(5, "DiMUSE_v2::callback()");
}

} // End of namespace Scumm
