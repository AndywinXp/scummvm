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

#include "common/endian.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lstring.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/sounds.h"

namespace Grim {

static const int32 kSoundTag = (int32)MKTAG(' ', 'D', 'N', 'S');
static const int kSfxSoundMode = 1;
static const int kDefaultSoundVolume = 100;

void Lua_V2::PlaySound() {
	lua_Object nameObj = lua_getparam(1);
	lua_Object volumeObj = lua_getparam(2);

	if (!lua_isstring(nameObj))
		return;

	LString soundName = lua_getstring(nameObj);

	LStringUtil::stringToLower(soundName);

	if (lua_isnumber(volumeObj))
		Sounds::playSound(soundName.c_str(), luaFtol(lua_getnumber(volumeObj)));
	else
		Sounds::playSound(soundName.c_str(), kDefaultSoundVolume);
}

void Lua_V2::StopSound() {
	lua_Object soundObj = lua_getparam(1);

	if (lua_isuserdata(soundObj) && lua_tag(soundObj) == kSoundTag)
		Sounds::stopSoundPlayback((uint)lua_getuserdata(soundObj));
}

void Lua_V2::StopAllSounds() {
	for (uint i = 0; i < _lockedSounds.size(); i++)
		Sounds::stopSound(_lockedSounds[i]);

	_lockedSounds.clear();
	Sounds::stopAllSounds();
}

void Lua_V2::LoadSound() {
	lua_Object nameObj = lua_getparam(1);

	if (!lua_isstring(nameObj))
		return;

	LString soundName = lua_getstring(nameObj);

	LStringUtil::stringToLower(soundName);

	uint soundHandle = Sounds::loadSound(soundName.c_str(), kSfxSoundMode);

	if (soundHandle != 0) {
		_lockedSounds.push_back(soundHandle);
		lua_pushusertag((int32)soundHandle, kSoundTag);
	} else {
		lua_pushnil();
	}
}

void Lua_V2::FreeSound() {
	lua_Object soundObj = lua_getparam(1);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	uint soundHandle = (uint)lua_getuserdata(soundObj);

	for (uint i = 0; i < _lockedSounds.size(); i++) {
		if (_lockedSounds[i] == soundHandle) {
			_lockedSounds.remove_at(i);

			break;
		}
	}

	Sounds::stopSound(soundHandle);
}

void Lua_V2::PlayLoadedSound() {
	lua_Object soundObj = lua_getparam(1);
	lua_Object loopObj = lua_getparam(2);
	lua_Object volumeObj = lua_getparam(3);
	lua_Object releaseObj = lua_getparam(4);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	uint soundHandle = (uint)lua_getuserdata(soundObj);
	bool loop = lua_isnil(loopObj) == 0;
	bool release = lua_isnil(releaseObj) != 0;
	int volume = kDefaultSoundVolume;

	if (lua_isnumber(volumeObj))
		volume = luaFtol(lua_getnumber(volumeObj));

	Sounds::startSound(soundHandle, release ? 1 : 0, loop, volume);

	if (release && !loop)
		Sounds::unregisterSound((uint16)soundHandle);
}

void Lua_V2::IsSoundPlaying() {
	lua_Object soundObj = lua_getparam(1);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	if (Sounds::isSoundPlayingNoFiniteLoop((uint)lua_getuserdata(soundObj)))
		lua_pushnumber(1.0f);
}

void Lua_V2::GetGroupVolume() {
	lua_Object groupObj = lua_getparam(1);

	if (lua_isnumber(groupObj)) {
		int volume = Sounds::getCategoryVolume(luaFtol(lua_getnumber(groupObj)));

		lua_pushnumber((float)volume);
	}
}

void Lua_V2::SetGroupVolume() {
	lua_Object groupObj = lua_getparam(1);
	lua_Object volumeObj = lua_getparam(2);

	if (!lua_isnumber(groupObj))
		return;

	int group = luaFtol(lua_getnumber(groupObj));
	int volume = kDefaultSoundVolume;

	if (lua_isnumber(volumeObj))
		volume = luaFtol(lua_getnumber(volumeObj));

	Sounds::setCategoryVolume(group, volume);
}

void Lua_V2::GetSoundVolume() {
	lua_Object soundObj = lua_getparam(1);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	int volume = 0;

	if (Sounds::getSoundVolume((uint)lua_getuserdata(soundObj), &volume))
		lua_pushnumber((float)volume);
}

void Lua_V2::SetSoundVolume() {
	lua_Object soundObj = lua_getparam(1);
	lua_Object volumeObj = lua_getparam(2);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	uint soundHandle = (uint)lua_getuserdata(soundObj);
	int volume = kDefaultSoundVolume;

	if (lua_isnumber(volumeObj))
		volume = luaFtol(lua_getnumber(volumeObj));

	Sounds::setSoundVolume(soundHandle, volume);
}

void Lua_V2::EnableAudioGroup() {
	lua_Object groupObj = lua_getparam(1);
	lua_Object enabledObj = lua_getparam(2);

	if (!lua_isnumber(groupObj))
		return;

	int group = luaFtol(lua_getnumber(groupObj));
	bool enabled = lua_isnil(enabledObj) == 0;

	Sounds::setCategoryEnabled(group, enabled);
}

void Lua_V2::EnableVoiceFX() {
	lua_Object enabledObj = lua_getparam(1);

	// The disasm passes the stack slot with only its low byte written
	Sounds::setVoiceFxEnabled(lua_isnil(enabledObj) ? 0 : 1);
}

void Lua_V2::SetAudioMixAhead() {
	lua_Object mixAheadObj = lua_getparam(1);

	if (lua_isnumber(mixAheadObj)) {
		int16 mixAhead = (int16)luaFtol(lua_getnumber(mixAheadObj));

		Sounds::setAudioMixAhead(&mixAhead, true);
	}
}

void Lua_V2::PlaySoundFrom() {
	lua_Object nameObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object volumeObj = lua_getparam(5);

	if (!lua_isstring(nameObj))
		return;

	LString soundName = lua_getstring(nameObj);

	LStringUtil::stringToLower(soundName);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);
	int volume = kDefaultSoundVolume;

	if (lua_isnumber(volumeObj))
		volume = luaFtol(lua_getnumber(volumeObj));

	Sounds::playSoundFrom(soundName.c_str(), x, y, z, (int16)volume);
}

void Lua_V2::PlayLoadedSoundFrom() {
	lua_Object soundObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object loopObj = lua_getparam(5);
	lua_Object volumeObj = lua_getparam(6);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	uint soundHandle = (uint)lua_getuserdata(soundObj);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);
	bool loop = lua_isnil(loopObj) == 0;
	int volume = kDefaultSoundVolume;

	if (lua_isnumber(volumeObj))
		volume = luaFtol(lua_getnumber(volumeObj));

	Sounds::playLoadedSoundFrom(soundHandle, x, y, z, loop, (int16)volume);
}

void Lua_V2::SetReverb() {
	static const int reverbPresets[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25
	};

	lua_Object environmentObj = lua_getparam(1);
	lua_Object paramObjs[5];
	float params[5];

	for (int i = 0; i < 5; i++) {
		paramObjs[i] = lua_getparam(i + 2);
		params[i] = -1.0f;
	}

	if (!lua_isnumber(environmentObj))
		return;

	int environment = luaFtol(lua_getnumber(environmentObj));
	int preset;

	if (environment == 60)
		preset = 26;
	else if (environment == 70)
		preset = 27;
	else if (environment < 0 || environment > 25)
		return;
	else
		preset = reverbPresets[environment];

	for (int i = 0; i < 5; i++) {
		if (lua_isnumber(paramObjs[i]))
			params[i] = lua_getnumber(paramObjs[i]);
	}

	Sounds::setReverb(preset, &params[0], &params[1], &params[2], &params[3], &params[4]);
}

void Lua_V2::UpdateSoundPosition() {
	lua_Object soundObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);

	if (!lua_isuserdata(soundObj) || lua_tag(soundObj) != kSoundTag)
		return;

	uint soundHandle = (uint)lua_getuserdata(soundObj);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);

	Sounds::setSoundPosition(soundHandle, x, y, z);
}

void Lua_V2::ImSetState() {
	lua_Object stateObj = lua_getparam(1);

	if (lua_isnumber(stateObj))
		EmiEngine::setPendingMusicState(luaFtol(lua_getnumber(stateObj)));
}

void Lua_V2::ImSelectSet() {
	lua_Object setObj = lua_getparam(1);

	if (lua_isnumber(setObj))
		Sounds::selectMusicSet((int16)luaFtol(lua_getnumber(setObj)));
}

void Lua_V2::ImStateHasLooped() {
	lua_Object musicObj = lua_getparam(1);

	if (!lua_isnumber(musicObj))
		return;

	uint musicId = (uint)luaFtol(lua_getnumber(musicObj));
	bool looped = false;
	bool ended = false;

	// The disasm leaves both flags uninitialised when the music is not playing
	Sounds::consumeMusicFlags(musicId, looped, ended);

	if (looped)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::ImStateHasEnded() {
	lua_Object musicObj = lua_getparam(1);

	if (!lua_isnumber(musicObj))
		return;

	uint musicId = (uint)luaFtol(lua_getnumber(musicObj));
	bool looped = false;
	bool ended = false;

	// The disasm leaves both flags uninitialised when the music is not playing
	Sounds::consumeMusicFlags(musicId, looped, ended);

	if (ended)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::ImPushState() {
	lua_Object musicObj = lua_getparam(1);
	lua_Object flagObj = lua_getparam(2);

	if (!lua_isnumber(musicObj))
		return;

	bool flag = lua_isnil(flagObj) == 0;

	Sounds::pushMusicState(luaFtol(lua_getnumber(musicObj)), flag);
}

void Lua_V2::ImPopState() {
	lua_Object flagObj = lua_getparam(1);
	bool flag = lua_isnil(flagObj) == 0;

	Sounds::popMusicState(flag);
}

void Lua_V2::ImFlushStack() {
	Sounds::flushMusicStateStack();
}

void Lua_V2::ImGetMillisecondPosition() {
	lua_Object musicObj = lua_getparam(1);

	if (!lua_isnumber(musicObj))
		return;

	uint musicId = (uint)luaFtol(lua_getnumber(musicObj));
	int position = Sounds::getMusicPositionMs(musicId);

	lua_pushnumber((float)position);
}

} // end of namespace Grim
