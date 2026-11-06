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
	RdSprite *pSprite;
	int bVisible;
	int resolveParentOffset;
	RdThing *pThing;
	ModelInst *pParentModel;
	int parentMeshIdx;
	RdColormap *pColormap;
	int refCount;
	ResRef aTrackRefs[6];
	int trackUserArg;
	ModelInst *pNext;
};

struct MeshInstData {
	ModelInst *pModelInst;
	int meshIndex;
	int bVisible;
};

struct MaterialInst {
	ModelInst *pModelInst;
	void *pCompRes;
	int key;
};

struct ColormapInst {
	ModelInst *pModelInst;
	ResRef ref;
};

struct KeyframeInst {
	ModelInst *pModelInst;
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
	RdSprite *pSprite;
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
	explicit ComponentHandler(SputCostume *pSputCostume) : _pSputCostume(pSputCostume) {}

protected:
	SputCostume *_pSputCostume;
};

class BitmapHandler : public ComponentHandler {
public:
	explicit BitmapHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
};

class ColormapHandler : public ComponentHandler {
public:
	explicit ColormapHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void *instance(int hInst, int compIndex, int bCreate) override;
};

class MaterialHandler : public ComponentHandler {
public:
	explicit MaterialHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void *instance(int hInst, int compIndex, int bCreate) override;
	void saveLoad(int hInst, int compIndex, int bSave, void *pInstData, Common::WriteStream *pWrite, Common::ReadStream *pRead) override;
};

class ThingHandler : public ComponentHandler {
public:
	explicit ThingHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void stop(void *pInstData) override;
	int adjust(void **ppCompRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int hInst, int compIndex, int bCreate) override;
};

class MeshHandler : public ComponentHandler {
public:
	explicit MeshHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void stop(void *pInstData) override;
	int adjust(void **ppCompRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int hInst, int compIndex, int bCreate) override;
};

class SpriteHandler : public ComponentHandler {
public:
	explicit SpriteHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	int adjust(void **ppCompRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int hInst, int compIndex, int bCreate) override;
};

class KeyframeHandler : public ComponentHandler {
public:
	explicit KeyframeHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void stop(void *pInstData) override;
	int adjust(void **ppCompRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
	void *instance(int hInst, int compIndex, int bCreate) override;
};

class SoundHandler : public ComponentHandler {
public:
	explicit SoundHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void stop(void *pInstData) override;
	int adjust(void **ppCompRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
};

class SmushHandler : public ComponentHandler {
public:
	explicit SmushHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void saveLoad(int hInst, int compIndex, int bSave, void *pInstData, Common::WriteStream *pWrite, Common::ReadStream *pRead) override;
};

class IMuseHandler : public ComponentHandler {
public:
	explicit IMuseHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
};

class LuaVarHandler : public ComponentHandler {
public:
	explicit LuaVarHandler(SputCostume *pSputCostume) : ComponentHandler(pSputCostume) {}

	void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) override;
	int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) override;
	void freeComponent(int hCostume, int fourcc, void *pCompRes) override;
	void stop(void *pInstData) override;
	int adjust(void **ppCompRes, char *args, int argsSize) override;
	bool hasAdjuster() const override { return true; }
};

} // End of namespace Grim

#endif
