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

#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/lua_v1_opcodes/lua_v1.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_lbm.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

static int instanceHookThunk(int instance, int create) {
	return g_grim->getSputCostume()->instanceHook(instance);
}

static int saveHookThunk(const char *path, int costume, const char *header) {
	return g_grim->getSputCostume()->saveHook(path, costume, header);
}

void SputCostume::puppetHookCallback(void *userArg, int trackNum, int markerType) {
	g_grim->getSputCostume()->puppetHook(static_cast<ModelInst *>(userArg), trackNum, markerType);
}

SputCostume::SputCostume(Res *res, RenderDroid *rd, SputResource *sputResource, SputRender *sputRender, SputRoom *sputRoom, StdLbm *stdLbm, WuRegistry *wuRegistry) :
		_res(res), _rd(rd), _sputResource(sputResource), _sputRender(sputRender), _sputRoom(sputRoom), _stdLbm(stdLbm),
		_wuRegistry(wuRegistry), _grim(nullptr),
		_sputActor(nullptr), _actors(nullptr), _hs(nullptr), _initialized(0), _started(0), _shadowColormap(nullptr), _lecBitmaps(nullptr),
		_lastBitmap(nullptr), _oldInstanceHook(nullptr), _oldSaveHook(nullptr),
		_thingHandler(this), _colormapHandler(this), _meshHandler(this), _materialHandler(this), _keyframeHandler(this), _demoKeyframeHandler(this),
		_bitmapHandler(this), _animationHandler(this), _soundHandler(this), _demoSoundHandler(this), _smushHandler(this), _demoSmushHandler(this), _spriteHandler(this), _imuseHandler(this), _luaVarHandler(this) {
	memset(_keyframePath, 0, sizeof(_keyframePath));
	memset(_colormapPath, 0, sizeof(_colormapPath));
	memset(_materialPath, 0, sizeof(_materialPath));
	memset(_modelPath, 0, sizeof(_modelPath));
	memset(_soundPath, 0, sizeof(_soundPath));
	memset(_smushPath, 0, sizeof(_smushPath));
	memset(_bitmapPath, 0, sizeof(_bitmapPath));
}

void SputCostume::setShadowColor(int red, int green, int blue) {
	byte greenByte = (byte)green;
	uint16 color = (uint16)(((uint)blue >> 3) | (32 * ((((uint)red >> 3) << 6) | ((uint)green >> 2))));

	_sputRender->colorCorrect(&color, 1);

	if (!_shadowColormap && (_grim->getGameFlags() & ADGF_DEMO)) {
		// The demo has no shadow colormap, its shadows are black
		_shadowColormap = new RdColormap();

		memset(_shadowColormap, 0, sizeof(RdColormap));
		_shadowColormap->rgb16 = new byte[0x8000]();
	} else if (!_shadowColormap) {
		char fileName[] = "shadow.cmp";

		_shadowColormap = _rd->_colormap.load(fileName);
	}

	if (!_shadowColormap || !_shadowColormap->rgb16) {
		warning("SputCostume::setShadowColor(): the shadow colormap has no RGB16 table");
		return;
	}

	uint16 *rgb16 = (uint16 *)_shadowColormap->rgb16;

	for (int i = 0; i < 0x4000; i++)
		rgb16[i] = color;

	for (int i = 0; i < 768; i += 3) {
		_shadowColormap->colors[i] = (byte)red;
		_shadowColormap->colors[i + 1] = greenByte;
		_shadowColormap->colors[i + 2] = (byte)blue;
	}
}

void SputCostume::printCostumeInfo(CostumeNode *node) {
	if (!node)
		return;

	CostumeNode *current = node;

	do {
		int numChores = _cos.instCountChores(current->costumeInstance);

		for (int i = 0; i < numChores; i++)
			_cos.instChoreStatus(current->costumeInstance, i);

		current = current->next;
	} while (current);

	for (ModelInst *modelInst = node->headList; modelInst; modelInst = modelInst->next) {
		if (!modelInst->visible)
			continue;

		if (!modelInst->thing || modelInst->thing->type != 1) {
			warning("SputCostume::printCostumeInfo(): the visible thing is not a model");
			continue;
		}

		RdModel3 *model3 = modelInst->thing->data.model3;

		for (uint i = 0; i < model3->numHNodes; i++)
			strstr(model3->hNodes[i].name, "skirt");
	}
}

void SputCostume::freeCostumeNodes(CostumeNode *node, int freeInstance) {
	CostumeNode *current = node;

	while (current) {
		CostumeNode *next = current->next;

		if (freeInstance)
			_cos.freeCosInstance(current->costumeInstance);

		if (current->rCostume.valid)
			_res->removeReference(&current->rCostume);

		delete current;
		current = next;
	}
}

CostumeNode *SputCostume::popCostume(CostumeNode *node, int freeInstance) {
	CostumeNode *next = node;

	if (node) {
		next = node->next;
		node->next = nullptr;
		_cos.instStopChores(node->costumeInstance);
		freeCostumeNodes(node, freeInstance);
	}

	return next;
}

CostumeNode *SputCostume::setCostume(CostumeNode *node, int costume, int instance) {
	CostumeNode *next = popCostume(node, 1);

	return pushCostume(next, costume, instance);
}

CostumeNode *SputCostume::pushCostume(CostumeNode *prev, int costume, int instance) {
	CostumeNode *node = _sputActor->allocCostumeNode();

	node->next = prev;

	if (instance) {
		node->costumeInstance = instance;
	} else {
		int parentInstance = 0;

		if (prev)
			parentInstance = prev->costumeInstance;

		node->costumeInstance = _cos.createCosInstance(costume, parentInstance);
		_res->setReference(&node->rCostume, (void *)(intptr)costume);
	}

	node->headThing = getMainThing(node->costumeInstance);

	if (node->headThing && prev && prev->headThing == node->headThing) {
		node->headJoint[0] = prev->headJoint[0];
		node->headJoint[1] = prev->headJoint[1];
		node->headJoint[2] = prev->headJoint[2];
		node->headTurnRate = prev->headTurnRate;
		node->headYawLimit = prev->headYawLimit;
		node->headPitchLimit = prev->headPitchLimit;
		node->headLookRef = prev->headLookRef;
		node->headList = prev->headList;
	}

	int numComponents = _cos.instCountComponents(node->costumeInstance);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;
		ModelInst *modelInst = static_cast<ModelInst *>(_cos.instGetInstanceData(node->costumeInstance, i, &fourcc));

		if ((fourcc == kSputCostumeTagMainThing || fourcc == kSputCostumeTagThing || fourcc == kSputCostumeTagSprite) && modelInst && modelInst->refCount == 1) {
			modelInst->next = node->headList;
			node->headList = modelInst;
		}
	}

	return node;
}

RdThing *SputCostume::getMainThing(int inst) {
	int numComponents = _cos.instCountComponents(inst);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;
		ModelInst *modelInst = static_cast<ModelInst *>(_cos.instGetInstanceData(inst, i, &fourcc));

		if (fourcc == kSputCostumeTagMainThing) {
			if (!modelInst) {
				warning("SputCostume::getMainThing(): the main thing has no instance data");
				return nullptr;
			}

			return modelInst->thing;
		}
	}

	return nullptr;
}

void SputCostume::update(CostumeNode *node, float delta) {
	double scaled = (double)delta * 1000.0;
	double rounded;

	if (scaled > 0.0)
		rounded = 0.5 + scaled;
	else
		rounded = scaled - 0.5;

	int deltaMs = 0;

	if (rounded > -2147483649.0 && rounded < 2147483648.0)
		deltaMs = (int)rounded;
	else
		warning("SputCostume::update(): the time delta is out of range");

	float seconds = (float)((double)(uint)deltaMs * (double)0.001f);

	if (!node)
		return;

	RdThing *headThing = node->headThing;
	CostumeNode *current = node;

	do {
		if (current->headThing != headThing)
			break;

		_cos.instUpdateChores(current->costumeInstance, deltaMs);
		current = current->next;
	} while (current);

	for (ModelInst *modelInst = node->headList; modelInst; modelInst = modelInst->next) {
		RdPuppet *puppet = modelInst->thing->puppet;

		if (puppet)
			_rd->_puppet.updateTracks(puppet, seconds);
	}
}

int SputCostume::initialize(SysStruct *hs) {
	_hs = hs;

	int result = _cos.initialize(hs);

	_initialized = result == 0;

	return result != 0;
}

void SputCostume::resetCostume(int inst) {
	_cos.instStopChores(inst);

	int numComponents = _cos.instCountComponents(inst);

	for (int i = 0; i < numComponents; i++) {
		int fourcc = 0;
		void *instData = _cos.instGetInstanceData(inst, i, &fourcc);

		if (fourcc == kSputCostumeTagMaterial) {
			static_cast<MaterialInst *>(instData)->key = -1;
		} else if (fourcc == kSputCostumeTagMainThing || fourcc == kSputCostumeTagThing) {
			ModelInst *modelInst = static_cast<ModelInst *>(instData);

			modelInst->visible = modelInst->parentModel == nullptr;

			for (int track = 0; track < _rd->_puppet.getNumTracks(); track++)
				_rd->_puppet.removeTrack(modelInst->thing->puppet, track);
		}
	}
}

void SputCostume::terminate() {
	_hs = nullptr;

	if (_initialized) {
		_initialized = 0;
		_cos.terminate();

		if (_shadowColormap)
			_rd->_colormap.destroy(_shadowColormap);
	}
}

int SputCostume::startup() {
	if (_started)
		return 1;

	_cos.startup();

	_cos.newStateBasedTag(kSputCostumeTagMainThing, "MAIN RDroid Thing (.3DO)", 0, &_thingHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagThing, "RDroid Thing (.3DO)", 0, &_thingHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagColormap, "RDroid Colormap", 0, &_colormapHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagMesh, "RDroid Thing Mesh", 0, &_meshHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagMaterial, "RDroid Animated Material", 0, &_materialHandler, 2);
	CosComponentHandler *keyframeHandler = &_keyframeHandler;

	if (_grim->getGameFlags() & ADGF_DEMO)
		keyframeHandler = &_demoKeyframeHandler;

	_cos.newStateBasedTag(kSputCostumeTagKeyframe, "RDroid Keyframe (.KEY)", 0, keyframeHandler, 2);
	_cos.newStateBasedTag(kSputCostumeTagBitmap, "General Bitmap", 0, &_bitmapHandler, 1);

	if (_grim->getGameFlags() & ADGF_DEMO) {
		_cos.newStateBasedTag(kSputCostumeTagStreamedSound, "iMuse Streamed Sound FX", 0, &_demoSoundHandler, 2);
		_cos.newStateBasedTag(kSputCostumeTagResidentSound, "iMuse Resident Sound FX", 0, &_demoSoundHandler, 2);
		_cos.newStateBasedTag(kSputCostumeTagSmush, "Smush Movie", 0, &_demoSmushHandler, 2);
		_cos.newStateBasedTag(kSputCostumeTagAnimation, "Animation", 0, &_animationHandler, 1);
		_cos.newIntTag(kSputCostumeTagSequence, "iMuse Sequence", 0, &_imuseHandler, 2);
		_cos.newIntTag(kSputCostumeTagState, "iMuse State", 0, &_imuseHandler, 2);
	} else {
		_cos.newStateBasedTag(kSputCostumeTagAnimation, "Animation (old,use Bitmap)", 0, &_bitmapHandler, 1);
		_cos.newStateBasedTag(kSputCostumeTagStreamedSound, "iMuse Streamed Sound FX", 0, &_soundHandler, 2);
		_cos.newStateBasedTag(kSputCostumeTagResidentSound, "iMuse Resident Sound FX", 0, &_soundHandler, 2);
		_cos.newStateBasedTag(kSputCostumeTagSmush, "Smush Movie", 0, &_smushHandler, 2);
		_cos.newStateBasedTag(kSputCostumeTagSprite, "Sprite", 0, &_spriteHandler, 2);
		_cos.newIntTag(kSputCostumeTagSequence, "iMuse Sequence", 0, &_imuseHandler, 2);
		_cos.newIntTag(kSputCostumeTagState, "iMuse State", 0, &_imuseHandler, 2);
		_cos.newIntTag(kSputCostumeTagLuaVar, "Lua Variable", 0, &_luaVarHandler, 1);
	}

	_oldInstanceHook = _cos.registerInstanceHook(instanceHookThunk);
	_oldSaveHook = _cos.registerSaveHook(saveHookThunk);

	_lua->registerCostumeOpcodes();

	_started = 1;

	getDataPath("KeyframePath", _keyframePath, "Animation");
	getDataPath("ColormapPath", _colormapPath, "Colormaps");
	getDataPath("MaterialPath", _materialPath, "Materials");
	getDataPath("ModelPath", _modelPath, "Models");
	getDataPath("SoundPath", _soundPath, "SFX");
	getDataPath("SmushPath", _smushPath, "Movies");
	getDataPath("BitmapPath", _bitmapPath, "Objects");

	return 0;
}

int SputCostume::shutdown() {
	writeRegistryValue("KeyframePath", _keyframePath);
	writeRegistryValue("BitmapPath", _bitmapPath);
	writeRegistryValue("ColormapPath", _colormapPath);
	writeRegistryValue("MaterialPath", _materialPath);
	writeRegistryValue("ModelPath", _modelPath);
	writeRegistryValue("SoundPath", _soundPath);
	writeRegistryValue("SmushPath", _smushPath);

	_oldInstanceHook = _cos.registerInstanceHook(_oldInstanceHook);
	_oldSaveHook = _cos.registerSaveHook(_oldSaveHook);

	if (_lecBitmaps) {
		delete[] _lecBitmaps;
		_lecBitmaps = nullptr;
	}

	int result = _cos.shutdown();

	_started = 0;

	return result;
}

int SputCostume::saveHook(const char *path, int costume, const char *header) {
	const char *extension = _stdLbm->getExtension(path);

	if (!extension) {
		warning("SputCostume::saveHook(): %s has no extension", path);
		return 0;
	}

	Common::String fileName = Common::String(path, extension) + "lua";
	Common::DumpFile file;

	if (!file.open(fileName.c_str())) {
		warning("SputCostume::saveHook(): Unable to open chore list file. Check to make sure that the destination file is not read-only (%s)", fileName.c_str());
		return 0;
	}

	if (!header) {
		warning("SputCostume::saveHook(): null header");
		header = "(null)";
	}

	file.writeString(Common::String::format("-- Chore numbering for %s.\n", _stdLbm->stripPath(path)));
	file.writeString(Common::String::format("-- %s\n\n", header));

	char baseName[32];

	_stdLbm->copyBaseName(baseName, 32, path);

	int inst = _cos.createCosInstance(costume, 0);
	int numChores = _cos.instCountChores(inst);

	for (int i = 0; i < numChores; i++) {
		const char *choreName = _cos.instChoreName(inst, i);

		if (!choreName)
			choreName = "";

		file.writeString(Common::String::format("%s_%s\t\t=\t%d\n", baseName, choreName, i));
	}

	file.close();
	_cos.freeCosInstance(inst);

	return 1;
}

int SputCostume::instanceHook(int inst) {
	_cos.instSetUserData(inst, 0);

	return 1;
}

void SputCostume::puppetHook(ModelInst *model, int trackNum, int markerType) {
	if (!model)
		return;

	_lua->doCostumeMarkerCallback(&_actors[_cos.instGetUserData(model->trackUserArg)], markerType);

	if (!markerType) {
		if (trackNum < 0 || trackNum >= 6) {
			warning("SputCostume::puppetHook(): invalid track %d", trackNum);
			return;
		}

		_res->removeReference(&model->trackRefs[trackNum]);
	}
}

void SputCostume::fadeKeysIn(int inst, int choreIndex, int timeMs) {
	int costumeHandle = 0;

	_cos.instGetCostume(inst, &costumeHandle);

	CosCostume *costume = _cos.ptrFromHandle(costumeHandle);

	if (!costume) {
		warning("SputCostume::fadeKeysIn(): invalid costume instance %d", inst);
		return;
	}

	if (choreIndex < 0 || choreIndex >= costume->numChores) {
		warning("SputCostume::fadeKeysIn(): invalid chore %d", choreIndex);
		return;
	}

	CosChore *chore = &costume->chores[choreIndex];

	for (int i = 0; i < chore->numTracks; i++) {
		int fourcc = 0;
		KeyframeInst *keyframeInst = static_cast<KeyframeInst *>(_cos.instGetInstanceData(inst, chore->tracks[i].compIndex, &fourcc));

		if (fourcc != kSputCostumeTagKeyframe || !keyframeInst)
			continue;

		RdPuppet *puppet = keyframeInst->modelInst->thing->puppet;
		int trackNum = 0;

		if (!keyframeInst->ref.valid)
			warning("SputCostume::fadeKeysIn(): Bad keyframe Reference");

		int numTracks = _rd->_puppet.getNumTracks();

		while (trackNum < numTracks && puppet->tracks[trackNum].keyframe != keyframeInst->ref.handle)
			trackNum++;

		if (trackNum == numTracks)
			continue;

		if (puppet->tracks[trackNum].fade == 1.0f)
			puppet->tracks[trackNum].fade = 0.0f;

		float time = (float)((double)timeMs * (double)0.001f);

		_rd->_puppet.fadeInTrack(puppet, trackNum, time);
	}
}

void SputCostume::fadeKeysOut(int inst, int choreIndex, int timeMs) {
	int costumeHandle = 0;

	_cos.instGetCostume(inst, &costumeHandle);

	CosCostume *costume = _cos.ptrFromHandle(costumeHandle);

	if (!costume) {
		warning("SputCostume::fadeKeysOut(): invalid costume instance %d", inst);
		return;
	}

	if (choreIndex < 0 || choreIndex >= costume->numChores) {
		warning("SputCostume::fadeKeysOut(): invalid chore %d", choreIndex);
		return;
	}

	CosChore *chore = &costume->chores[choreIndex];

	for (int i = 0; i < chore->numTracks; i++) {
		int fourcc = 0;
		KeyframeInst *keyframeInst = static_cast<KeyframeInst *>(_cos.instGetInstanceData(inst, chore->tracks[i].compIndex, &fourcc));

		if (fourcc != kSputCostumeTagKeyframe || !keyframeInst)
			continue;

		RdPuppet *puppet = keyframeInst->modelInst->thing->puppet;
		int trackNum = 0;

		if (!keyframeInst->ref.valid)
			warning("SputCostume::fadeKeysOut(): Bad keyframe Reference");

		int numTracks = _rd->_puppet.getNumTracks();

		while (trackNum < numTracks && puppet->tracks[trackNum].keyframe != keyframeInst->ref.handle)
			trackNum++;

		if (trackNum == numTracks)
			continue;

		float time = (float)((double)timeMs * (double)0.001f);

		_rd->_puppet.fadeOutTrack(puppet, trackNum, time);
	}
}

void SputCostume::getDataPath(const char *valueName, char *data, const char *defaultDirectory) {
	char name[128];

	Common::strlcpy(name, "Requesters\\", sizeof(name));
	Common::strlcat(name, valueName, sizeof(name));

	_wuRegistry->getStr(name, data, 128, "");

	if (*data)
		return;

	if (_wuRegistry->getStr("GrimDataDir", data, 128, "")) {
		Common::strlcat(data, "\\", 128);
		Common::strlcat(data, defaultDirectory, 128);
	}
}

void SputCostume::writeRegistryValue(const char *valueName, const char *data) {
	if (*data) {
		char name[128];

		Common::strlcpy(name, "Requesters\\", sizeof(name));
		Common::strlcat(name, valueName, sizeof(name));

		_wuRegistry->saveStr(name, data);
	}
}

void SputCostume::nullSub1() {
}

int SputCostume::stub() {
	return 1;
}

} // End of namespace Grim
