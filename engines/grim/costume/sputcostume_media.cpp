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

#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/lua/lua.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

static const int soundKeyValues[3] = { 0, 1, 2 };
static const char *const soundKeyNames[3] = { "Play", "Stop", "Stop Looping" };
static const CosComponentNames soundKeyDesc = { 3, soundKeyValues, soundKeyNames, nullptr, nullptr };

static const int demoSoundKeyValues[2] = { 0, 1 };
static const char *const demoSoundKeyNames[2] = { "Play", "Stop" };
static const CosComponentNames demoSoundKeyDesc = { 2, demoSoundKeyValues, demoSoundKeyNames, nullptr, nullptr };

static const int smushKeyValues[2] = { 0, 1 };
static const char *const smushKeyNames[2] = { "Play", "Stop" };
static const CosComponentNames smushKeyDesc = { 2, smushKeyValues, smushKeyNames, nullptr, nullptr };

void BitmapHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	ResRef *ref = static_cast<ResRef *>(instData);

	if (!ref->valid) {
		warning("BitmapHandler::change(): Bad Bitmap Reference");
		return;
	}

	StdBitmapStruct *bitmap = static_cast<StdBitmapStruct *>(ref->handle);

	bitmap->curFrame = keyValue - 1;
}

int BitmapHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	char name[32];

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = comp->name;

	Common::strlcpy(name, args, 32);

	char *bitmapName = strtok(name, ",");

	if (!bitmapName) {
		warning("BitmapHandler::request(): empty bitmap name");
		return 0;
	}

	StdBitmapStruct *bitmap = static_cast<StdBitmapStruct *>(_sputCostume->_sputResource->alloc(kSputTagBknd, bitmapName));

	if (!bitmap)
		return 0;

	Common::strlcpy(bitmap->name, bitmapName, 32);

	ResRef *res = new ResRef();

	_sputCostume->_res->setReference(res, bitmap);

	int numFrames = bitmap->numFrames;
	int *nameIds = new int[numFrames + 1];

	nameIds[0] = 0;

	if (_sputCostume->_lecBitmaps)
		delete[] _sputCostume->_lecBitmaps;

	_sputCostume->_lecBitmaps = new CosStateBitmapDesc[numFrames + 1]();

	for (int i = 0; i < numFrames; i++) {
		StdVBuffer *frame = bitmap->frames[i];

		nameIds[i + 1] = i + 1;

		_sputCostume->_lecBitmaps[i].pixels = frame->pixels;
		_sputCostume->_lecBitmaps[i].width = frame->tex.width;
		_sputCostume->_lecBitmaps[i].height = frame->tex.height;
		_sputCostume->_lecBitmaps[i].reserved12 = 1;
	}

	const int *bitmapExtra = nullptr;

	if (bitmap->flags & 2)
		bitmapExtra = reinterpret_cast<const int *>(&bitmap->palette);

	CosComponentNames keyDesc = { numFrames + 1, nameIds, nullptr, bitmapExtra ? _sputCostume->_lecBitmaps : nullptr, bitmapExtra };
	int result = _sputCostume->_cos.registerComponent(costume, args, res, fourcc, parentId, parentFourcc, &keyDesc);

	delete[] nameIds;

	if (result) {
		_sputCostume->_lastBitmap = bitmap;
		return 1;
	}

	_sputCostume->_lastBitmap = nullptr;

	return 0;
}

void BitmapHandler::freeComponent(int costume, int fourcc, void *compRes) {
	ResRef *res = static_cast<ResRef *>(compRes);

	_sputCostume->_lastBitmap = compRes;
	_sputCostume->_res->removeReference(res);
	delete res;
}

// The component name is "bitmap,layer", it is kept whole in the bitmap structure
static int getAnimationLayer(const StdBitmapStruct *bitmap) {
	char name[32];

	Common::strlcpy(name, bitmap->name, 32);
	strtok(name, ",");

	const char *layer = strtok(nullptr, ",");

	return layer && atoi(layer) > 0 ? 1 : 0;
}

void AnimationHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	ResRef *ref = static_cast<ResRef *>(instData);

	if (!ref->valid) {
		warning("AnimationHandler::change(): Bad Bitmap Reference");
		return;
	}

	StdBitmapStruct *bitmap = static_cast<StdBitmapStruct *>(ref->handle);

	_sputCostume->_sputRender->addAnimationBitmap(bitmap, getAnimationLayer(bitmap));
	bitmap->curFrame = keyValue - 1;
}

int AnimationHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	if (!BitmapHandler::request(costume, fourcc, parentId, parentFourcc, comp))
		return 0;

	StdBitmapStruct *bitmap = static_cast<StdBitmapStruct *>(_sputCostume->_lastBitmap);

	Common::strlcpy(bitmap->name, comp->name, 32);
	bitmap->curFrame = -1;
	_sputCostume->_sputRender->addAnimationBitmap(bitmap, getAnimationLayer(bitmap));

	return 1;
}

void AnimationHandler::freeComponent(int costume, int fourcc, void *compRes) {
	// The EXE looks the reference up again after freeing it and never finds it
	BitmapHandler::freeComponent(costume, fourcc, compRes);
}

void AnimationHandler::stop(void *instData) {
	ResRef *ref = static_cast<ResRef *>(instData);

	if (!ref->valid) {
		warning("AnimationHandler::stop(): Bad Bitmap Reference");
		return;
	}

	_sputCostume->_sputRender->removeAnimationBitmap(ref->handle);
}

int AnimationHandler::adjust(void **compRes, char *args, int argsSize) {
	// TODO: grimPlat_InputDialog("Enter a 1 to make this an overlay...")
	return 0;
}

void SoundHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	SoundCompRes *res = static_cast<SoundCompRes *>(instData);

	if (keyValue < 0 || keyValue >= 3) {
		warning("SoundHandler::change(): invalid key %d", keyValue);
		return;
	}

	int action = soundKeyValues[keyValue];

	if (action || g_grim->getImuse()->soundsIsSoundPlaying(res->name)) {
		if (action == 1) {
			if (fourcc == kSputCostumeTagResidentSound)
				g_grim->getImuse()->soundsStopSound(res->name);
			else if (fourcc == kSputCostumeTagStreamedSound)
				g_grim->getImuse()->soundStopVoiceStream();
		} else if (action == 2) {
			g_grim->getImuse()->iMUSESetHook(res->name, 0x80);
		}
	} else if (fourcc == kSputCostumeTagResidentSound) {
		SputRoom *sputRoom = _sputCostume->_sputRoom;
		Room *room = sputRoom->getCurrentRoom();
		int soundId = g_grim->getImuse()->soundsLoadStartSound(res->name, res->priority, 1);

		if (room) {
			float maxDist;
			float panFactor;
			float maxRadius;
			byte roomMinVolume;
			byte roomMaxVolume;

			sputRoom->getSoundParameters(room, &maxDist, &roomMinVolume, &roomMaxVolume, &panFactor, &maxRadius);

			int userData = _sputCostume->_cos.instGetUserData(instHandle);
			SputActor *actor = &_sputCostume->_actors[userData];
			byte volume = res->volume;

			if (roomMaxVolume < volume)
				volume = roomMaxVolume;

			int minVolume = roomMinVolume;

			if (minVolume <= res->minVolume)
				minVolume = res->minVolume;

			sputRoom->setSoundPosition((const char *)(intptr)soundId, actor->actorPos, minVolume, volume, maxDist, maxRadius);
		}
	} else if (fourcc == kSputCostumeTagStreamedSound) {
		g_grim->getImuse()->soundStartVoiceStream(res->name, res->volume, res->pan);
		g_grim->getImuse()->soundsSetParam(res->name, IMUSE_P_GROUP, 1);
		g_grim->getImuse()->soundsSetParam(res->name, IMUSE_P_PRIORITY, res->priority);
	}
}

int SoundHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	char name[32];
	byte volume = 127;
	int8 pan = 64;
	int priority = 100;
	int minVolume = 1;

	memset(name, 0, sizeof(name));

	if (parentId)
		return 0;

	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = comp->name;

	Common::strlcpy(name, args, 32);
	strtok(name, ",");

	char *volumePointer = strtok(nullptr, ",");

	if (volumePointer) {
		volume = (byte)atoi(volumePointer);

		char *tokens[3];

		for (int i = 0; i < 3; i++) {
			tokens[i] = strtok(nullptr, ",");

			if (!tokens[i])
				warning("SoundHandler::request(): missing sound parameter in %s", args);
		}

		if (tokens[0])
			pan = (int8)atoi(tokens[0]);

		if (tokens[1])
			priority = atoi(tokens[1]);

		if (tokens[2])
			minVolume = atoi(tokens[2]);
	}

	SoundCompRes *res = new SoundCompRes();
	const char *soundName = name;

	if (!name[0])
		soundName = args;

	Common::strlcpy(res->name, soundName, 32);
	res->pan = pan;
	res->minVolume = minVolume;
	res->volume = volume;
	res->priority = priority;

	return _sputCostume->_cos.registerComponent(costume, args, res, fourcc, nullptr, parentFourcc, &soundKeyDesc) != 0;
}

void SoundHandler::freeComponent(int costume, int fourcc, void *compRes) {
	SoundCompRes *res = static_cast<SoundCompRes *>(compRes);

	delete res;
}

void SoundHandler::stop(void *instData) {
	SoundCompRes *res = static_cast<SoundCompRes *>(instData);

	g_grim->getImuse()->soundsStopSound(res->name);
}

int SoundHandler::adjust(void **compRes, char *args, int argsSize) {
	SoundCompRes *res = static_cast<SoundCompRes *>(*compRes);
	int values[4];

	values[0] = res->volume;
	values[1] = res->pan;
	values[2] = res->priority;
	values[3] = res->minVolume;

	int accepted = 0; // TODO: grimPlat_ControlPanel("Maximum Volume", "Pan", "Priority", "Minimum Volume")

	if (!accepted || values[0] < values[3])
		return 0;

	char *token = strtok(args, ",");

	if (!token) {
		warning("SoundHandler::adjust(): empty component name");
		return 0;
	}

	res->volume = (byte)CLIP(values[0], 0, 127);
	res->pan = (int8)CLIP(values[1], 0, 127);
	res->priority = CLIP(values[2], 0, 127);
	res->minVolume = CLIP(values[3], 0, 127);

	char text[32];

	snprintf(text, sizeof(text), "%s,%d,%d,%d,%d", token, res->volume, res->pan, res->priority, res->minVolume);
	Common::strlcpy(token, text, 32);

	return 1;
}

void DemoSoundHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	DemoSoundCompRes *res = static_cast<DemoSoundCompRes *>(instData);

	if (keyValue < 0 || keyValue >= 2) {
		warning("DemoSoundHandler::change(): invalid key %d", keyValue);
		return;
	}

	if (demoSoundKeyValues[keyValue]) {
		if (fourcc == kSputCostumeTagResidentSound)
			g_grim->getImuse()->soundsStopSound(res->name);
		else if (fourcc == kSputCostumeTagStreamedSound)
			g_grim->getImuse()->soundStopVoiceStream();
	} else if (fourcc == kSputCostumeTagResidentSound) {
		g_grim->getImuse()->soundsLoadStartSound(res->name, res->priority, res->group);
		g_grim->getImuse()->soundsSetParam(res->name, IMUSE_P_VOLUME, res->volume);
		g_grim->getImuse()->soundsSetParam(res->name, IMUSE_P_PAN, res->pan);
	} else if (fourcc == kSputCostumeTagStreamedSound) {
		g_grim->getImuse()->soundStartVoiceStream(res->name, res->volume, res->pan);
		g_grim->getImuse()->soundsSetParam(res->name, IMUSE_P_GROUP, res->group);
		g_grim->getImuse()->soundsSetParam(res->name, IMUSE_P_PRIORITY, res->priority);
	}
}

int DemoSoundHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	char name[32];
	byte volume = 127;
	int8 pan = 64;
	int priority = 100;
	int group = 1;

	memset(name, 0, sizeof(name));

	if (parentId) {
		warning("Sounds must be children of the costume");
		return 0;
	}

	// TODO: grimPlat_OpenFileDialog branch (args == NULL)
	const char *args = comp->name;

	Common::strlcpy(name, args, 32);
	strtok(name, ",");

	char *volumePointer = strtok(nullptr, ",");

	if (volumePointer) {
		volume = (byte)atoi(volumePointer);

		char *tokens[3];

		for (int i = 0; i < 3; i++) {
			tokens[i] = strtok(nullptr, ",");

			if (!tokens[i])
				warning("DemoSoundHandler::request(): missing sound parameter in %s", args);
		}

		if (tokens[0])
			pan = (int8)atoi(tokens[0]);

		if (tokens[1])
			priority = atoi(tokens[1]);

		if (tokens[2])
			group = atoi(tokens[2]);
	}

	DemoSoundCompRes *res = new DemoSoundCompRes();
	const char *soundName = name;

	if (!name[0])
		soundName = args;

	memset(res, 0, sizeof(DemoSoundCompRes));
	Common::strlcpy(res->name, soundName, 32);
	res->pan = pan;
	res->group = group;
	res->volume = volume;
	res->priority = priority;

	return _sputCostume->_cos.registerComponent(costume, args, res, fourcc, nullptr, parentFourcc, &demoSoundKeyDesc) != 0;
}

void DemoSoundHandler::freeComponent(int costume, int fourcc, void *compRes) {
	delete static_cast<DemoSoundCompRes *>(compRes);
}

int DemoSoundHandler::adjust(void **compRes, char *args, int argsSize) {
	// TODO: grimPlat_ControlPanel dialog
	return 0;
}

void SmushHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	if (!keyValue)
		_sputCostume->_grim->playMovie(static_cast<const char *>(instData), 0, 0, -1, -1);
}

void DemoSmushHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	char fileName[64];

	snprintf(fileName, sizeof(fileName), "MOVIES\\%s", static_cast<const char *>(instData));

	if (!keyValue)
		_sputCostume->_grim->playMovie(fileName, 0, 0, -1, -1);
}

int SmushHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	// TODO: grimPlat_SelectFilesDialog branch (args == NULL)
	const char *args = comp->name;
	size_t size = strlen(args) + 1;
	char *name = new char[size];

	Common::strlcpy(name, args, size);

	return _sputCostume->_cos.registerComponent(costume, args, name, fourcc, parentId, parentFourcc, &smushKeyDesc) != 0;
}

void SmushHandler::freeComponent(int costume, int fourcc, void *compRes) {
	char *name = static_cast<char *>(compRes);

	delete[] name;
}

void SmushHandler::saveLoad(int instHandle, int compIndex, int save, void *instData, Common::WriteStream *write, Common::ReadStream *read) {
	_sputCostume->stub();
}

void IMuseHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	_sputCostume->nullSub1();
}

int IMuseHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	if (parentId)
		return 0;

	const char *name = "iMuse Sequence";

	if (fourcc != kSputCostumeTagSequence)
		name = "iMuse State";

	return _sputCostume->_cos.registerComponent(costume, name, (void *)(intptr)1, fourcc, nullptr, parentFourcc, nullptr) != 0;
}

void IMuseHandler::freeComponent(int costume, int fourcc, void *compRes) {
	_sputCostume->stub();
}

void LuaVarHandler::change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {
	float number = (float)keyValue;

	lua_pushnumber(number);
	lua_setglobal(static_cast<const char *>(instData));
}

int LuaVarHandler::request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) {
	char name[32];

	memset(name, 0, sizeof(name));

	if (parentId)
		return 0;

	// TODO: grimPlat_InputDialog branch (args == NULL)
	Common::strlcpy(name, comp->name, 32);

	char *varName = new char[32];

	Common::strlcpy(varName, name, 32);

	return _sputCostume->_cos.registerComponent(costume, varName, varName, fourcc, nullptr, parentFourcc, nullptr) != 0;
}

void LuaVarHandler::freeComponent(int costume, int fourcc, void *compRes) {
	char *varName = static_cast<char *>(compRes);

	delete[] varName;
}

void LuaVarHandler::stop(void *instData) {
	lua_pushnil();
	lua_setglobal(static_cast<const char *>(instData));
}

int LuaVarHandler::adjust(void **compRes, char *args, int argsSize) {
	char *varName = static_cast<char *>(*compRes);
	char text[32];

	memset(text, 0, sizeof(text));

	int accepted = 0; // TODO: grimPlat_InputDialog("Change variable properties", "Variable name")

	if (!accepted)
		return 0;

	Common::strlcpy(varName, text, 32);
	Common::strlcpy(args, text, argsSize);

	return 1;
}

} // End of namespace Grim
