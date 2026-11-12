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

#ifndef GRIM_SPUTCOSTUME_COMPONENTS_H
#define GRIM_SPUTCOSTUME_COMPONENTS_H

#include "common/endian.h"
#include "common/scummsys.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"

namespace Grim {

enum SputCostumeTags {
	kSputCostumeTagMainThing = MKTAG('L', 'D', 'M', 'M'),
	kSputCostumeTagThing = MKTAG('L', 'D', 'O', 'M'),
	kSputCostumeTagColormap = MKTAG('p', 'a', 'm', 'c'),
	kSputCostumeTagMesh = MKTAG('H', 'S', 'E', 'M'),
	kSputCostumeTagMaterial = MKTAG(' ', 'T', 'A', 'M'),
	kSputCostumeTagKeyframe = MKTAG('F', 'Y', 'E', 'K'),
	kSputCostumeTagBitmap = MKTAG('D', 'N', 'K', 'B'),
	kSputCostumeTagAnimation = MKTAG('m', 'i', 'n', 'a'),
	kSputCostumeTagStreamedSound = MKTAG('s', 's', 'm', 'i'),
	kSputCostumeTagResidentSound = MKTAG('s', 'l', 'm', 'i'),
	kSputCostumeTagSmush = MKTAG('S', 'M', 'S', 'H'),
	kSputCostumeTagSprite = MKTAG('t', 'r', 'p', 's'),
	kSputCostumeTagSequence = MKTAG('e', 's', 'm', 'i'),
	kSputCostumeTagState = MKTAG('t', 's', 'm', 'i'),
	kSputCostumeTagLuaVar = MKTAG('v', 'a', 'u', 'l')
};

struct ModelInst {
	ResRef modelRef;
	RdSprite *sprite;
	int visible;
	int resolveParentOffset;
	RdThing *thing;
	ModelInst *parentModel;
	int parentMeshIdx;
	RdColormap *colormap;
	int refCount;
	ResRef trackRefs[6];
	int trackUserArg;
	ModelInst *next;
};

struct MeshInstData {
	ModelInst *modelInst;
	int meshIndex;
	int visible;
};

struct MaterialInst {
	ModelInst *modelInst;
	void *compRes;
	int key;
};

struct ColormapInst {
	ModelInst *modelInst;
	ResRef ref;
};

struct KeyframeInst {
	ModelInst *modelInst;
	ResRef ref;
	int priorityMasked;
	int priority;
};

struct ThingCompRes {
	ResRef ref;
	int resolveParentOffset;
};

struct KeyframeCompRes {
	ResRef ref;
	int priorityMasked;
	int priority;
};

struct SpriteCompRes {
	RdSprite *sprite;
	char name[32];
};

struct SoundCompRes {
	char name[32];
	byte volume;
	int8 pan;
	int priority;
	int minVolume;
};

class SputCostume;

class ComponentHandler : public CosComponentHandler {
public:
	explicit ComponentHandler(SputCostume *sputCostume) : _sputCostume(sputCostume) {}

protected:
	SputCostume *_sputCostume;
};

class BitmapHandler : public ComponentHandler {
public:
	explicit BitmapHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
};

// Only the demo has this one, the retail executable handles the animation tag as a bitmap
class AnimationHandler : public BitmapHandler {
public:
	explicit AnimationHandler(SputCostume *sputCostume) : BitmapHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void stop(void *instData) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
};

class ColormapHandler : public ComponentHandler {
public:
	explicit ColormapHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void *instance(int instHandle, int compIndex, int create) override;
};

class MaterialHandler : public ComponentHandler {
public:
	explicit MaterialHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void *instance(int instHandle, int compIndex, int create) override;
	void saveLoad(int instHandle, int compIndex, int save, void *instData, Common::WriteStream *write, Common::ReadStream *read) override;
};

class ThingHandler : public ComponentHandler {
public:
	explicit ThingHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void stop(void *instData) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int instHandle, int compIndex, int create) override;
};

class MeshHandler : public ComponentHandler {
public:
	explicit MeshHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void stop(void *instData) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int instHandle, int compIndex, int create) override;
};

class SpriteHandler : public ComponentHandler {
public:
	explicit SpriteHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int instHandle, int compIndex, int create) override;
};

class KeyframeHandler : public ComponentHandler {
public:
	explicit KeyframeHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void stop(void *instData) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int instHandle, int compIndex, int create) override;
};

// The demo puppets have 4 tracks and the tracks start from the beginning of the keyframe
class DemoKeyframeHandler : public KeyframeHandler {
public:
	explicit DemoKeyframeHandler(SputCostume *sputCostume) : KeyframeHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	void stop(void *instData) override;
};

class SoundHandler : public ComponentHandler {
public:
	explicit SoundHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void stop(void *instData) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
};

// The sounds of the demo have a group instead of a minimum volume and no room positioning
struct DemoSoundCompRes {
	char name[32];
	byte volume;
	int8 pan;
	int priority;
	int group;
};

class DemoSoundHandler : public ComponentHandler {
public:
	explicit DemoSoundHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
};

class SmushHandler : public ComponentHandler {
public:
	explicit SmushHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void saveLoad(int instHandle, int compIndex, int save, void *instData, Common::WriteStream *write, Common::ReadStream *read) override;
};

class DemoSmushHandler : public SmushHandler {
public:
	explicit DemoSmushHandler(SputCostume *sputCostume) : SmushHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
};

class IMuseHandler : public ComponentHandler {
public:
	explicit IMuseHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
};

class LuaVarHandler : public ComponentHandler {
public:
	explicit LuaVarHandler(SputCostume *sputCostume) : ComponentHandler(sputCostume) {}

	void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) override;
	int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) override;
	void freeComponent(int costume, int fourcc, void *compRes) override;
	void stop(void *instData) override;
	int adjust(void **compRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
};

} // End of namespace Grim

#endif
