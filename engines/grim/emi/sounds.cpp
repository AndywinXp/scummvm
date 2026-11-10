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

#include "engines/grim/emi/sounds.h"

namespace Grim {

int16 Sounds::audioMixAhead = 0x0110;

LResource *Sounds::createWaveSoundResource() {
	// TODO
	return nullptr;
}

uint Sounds::loadSound(const char *soundName, int mode) {
	// TODO
	return 0;
}

void Sounds::startSound(uint soundHandle, int startMode, bool loop, int volume) {
	// TODO
}

void Sounds::stopSound(uint soundHandle) {
	// TODO
}

void Sounds::stopSoundPlayback(uint soundHandle) {
	// TODO
}

void Sounds::releaseSoundWhenDone(uint soundHandle) {
	// TODO
}

bool Sounds::isSoundPlaying(uint soundHandle) {
	// TODO
	return false;
}

bool Sounds::isSoundPlayingNoFiniteLoop(uint soundHandle) {
	// TODO
	return false;
}

int Sounds::getSoundPosition(uint soundHandle) {
	// TODO
	return 0;
}

int Sounds::getSoundLength(uint soundHandle) {
	// TODO
	return 0;
}

LipData *Sounds::getSoundLipData(uint soundHandle) {
	// TODO
	return nullptr;
}

bool Sounds::isSoundPaused(uint soundHandle) {
	// TODO
	return false;
}

void Sounds::pauseSound(uint soundHandle) {
	// TODO
}

void Sounds::resumeSound(uint soundHandle) {
	// TODO
}

void Sounds::setAudioMixAhead(const int16 *mixAhead, bool store) {
	// TODO
}

bool Sounds::soundsUpdateThrottled(const bool *force) {
	// TODO
	return false;
}

void Sounds::setSoundListener(CNode *listener) {
	// TODO
}

} // End of namespace Grim
