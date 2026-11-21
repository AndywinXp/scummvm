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

#include "common/textconsole.h"

#include "topgun/libraries/spritelibrary.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

bool TopgunEngine_v0::o_setClickScript(const byte *record) {
	warning("o_setClickScript UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::o_setSpriteQueue(uint16 spriteId, uint16 dataId) {
	warning("o_setSpriteQueue UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::o_playWave(uint16 waveId, const byte *params) {
	warning("o_playWave UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::o_playCDTrack(uint16 track) {
	warning("o_playCDTrack UNIMPLEMENTED");
}

void TopgunEngine_v0::o_fadeInOut(const byte *record) {
	warning("o_fadeInOut UNIMPLEMENTED");
}

void TopgunEngine_v0::o_bindKeyScript(const byte *record) {
	warning("o_bindKeyScript UNIMPLEMENTED");
}

void TopgunEngine_v0::o_stopCD() {
	warning("o_stopCD UNIMPLEMENTED");
}

bool TopgunEngine_v0::o_stopWave(uint16 waveId, const byte *block) {
	warning("o_stopWave UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::o_setScriptTimer(const byte *record) {
	warning("o_setScriptTimer UNIMPLEMENTED");
}

void TopgunEngine_v0::o_cancelScriptTimer(const byte *record) {
	warning("o_cancelScriptTimer UNIMPLEMENTED");
}

bool TopgunEngine_v0::o_randomRunScript(const byte *record) {
	warning("o_randomRunScript UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::o_roundRobinRunScript(byte *record) {
	warning("o_roundRobinRunScript UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::o_transferSprite(uint16 sourceSpriteId, uint16 destinationSpriteId, uint16 dataId, bool freeSource) {
	warning("o_transferSprite UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::o_setSpriteLevel(uint16 spriteId, uint16 level) {
	warning("o_setSpriteLevel UNIMPLEMENTED");
}

bool TopgunEngine_v0::transferSpriteContents(uint16 sourceSpriteId, uint16 destinationSpriteId, bool freeSource) {
	warning("transferSpriteContents UNIMPLEMENTED");
	return false;
}

bool TopgunEngine_v0::appendQueueBitmapsToSprite(uint16 destinationSpriteId, const SpriteInfo &sourceInfo) {
	warning("appendQueueBitmapsToSprite UNIMPLEMENTED");
	return false;
}

void TopgunEngine_v0::releaseSourceSpriteFrameRefs(uint16 spriteId) {
	warning("releaseSourceSpriteFrameRefs UNIMPLEMENTED");
}

void TopgunEngine_v0::destroySourceSpriteAfterTransfer(uint16 sourceSpriteId, const SpriteInfo &sourceInfo) {
	warning("destroySourceSpriteAfterTransfer UNIMPLEMENTED");
}

void TopgunEngine_v0::trimSpriteQueueBitmaps(uint16 spriteId) {
	warning("trimSpriteQueueBitmaps UNIMPLEMENTED");
}

} // End of namespace Topgun
