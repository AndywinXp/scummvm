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

#ifndef GRIM_CHORE_COS_H
#define GRIM_CHORE_COS_H

#include "common/scummsys.h"

#include "engines/grim/std/std_sys.h"

namespace Common {
class ReadStream;
class SeekableReadStream;
class WriteStream;
}

namespace Grim {

class LegacyImporter;

enum CosTagTypes {
	kCosTagStateBased = 0,
	kCosTagInt = 1,
	kCosTagFloat = 2
};

enum CosChoreStates {
	kCosChoreStatePlaying = 1,
	kCosChoreStateLooping = 2,
	kCosChoreStateNoAdvance = 4
};

struct CosChoreKey {
	int time;
	int value;
};

struct CosChoreTrack {
	int compIndex;
	int length;
	int numKeys;
	CosChoreKey *keys;
};

struct CosChore {
	char name[32];
	int numTracks;
	CosChoreTrack *tracks;
	int length;
};

struct CosStateBitmapDesc {
	byte *pixels;
	int width;
	int height;
	int reserved12;
};

struct CosStateBitmap {
	byte *pixels;
	int width;
	int height;
	int reserved12;
	int mungerResult;
};

struct CosComponentNames {
	int numNames;
	const int *nameIds;
	const char *const *names;
	const CosStateBitmapDesc *stateBitmaps;
	const int *bitmapExtra;
};

struct CosComponent {
	char name[32];
	int classIdx;
	void *id;
	int depthIdx;
	int parent;
	int numNames;
	int *nameIds;
	char **names;
	CosStateBitmap **stateBitmaps;
};

struct CosCostume {
	char name[32];
	int reserved32;
	int uniqueId;
	int numComponents;
	CosComponent *components;
	int numChores;
	CosChore *chores;
	int free;
};

struct CosChoreInstance {
	uint32 state;
	uint32 choreIndex;
	int time;
};

struct CosInstance {
	int costume;
	CosChoreInstance *choreInstances;
	int userData;
	void **instanceData;
	int free;
};

class CosComponentHandler {
public:
	virtual ~CosComponentHandler() {}

	virtual void change(int fourcc, void *instData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int instHandle) {}
	virtual int request(int costume, int fourcc, void *parentId, int parentFourcc, CosComponent *comp) { return 0; }
	virtual void freeComponent(int costume, int fourcc, void *compRes) {}
	virtual void stop(void *instData) {}
	virtual int adjust(void **compRes, char *args, int argsSize) { return 0; }
	virtual bool hasAdjuster() const { return false; }
	virtual void *instance(int instHandle, int compIndex, int create) { return nullptr; }
	virtual void saveLoad(int instHandle, int compIndex, int save, void *instData, Common::WriteStream *write, Common::ReadStream *read) {}
};

struct CosComponentClass {
	int fourcc;
	int type;
	char name[32];
	int param;
	CosComponentHandler *handler;
	int reserved72[5];
};

typedef int (*CosInstanceHook)(int inst, int create);
typedef int (*CosSaveHook)(const char *path, int costume, const char *header);
typedef int (*CosSaveVerify)(const char *costumeName);
typedef int (*CosCompNotifier)(CosCostume *costume, CosComponent *comp, int compIndex, int added);
typedef int (*CosBitmapMunger)(const CosStateBitmapDesc *desc, int extra);

class Cos {
	friend class CosUtil;
	friend class LegacyImporter;

public:
	Cos();

	int initialize(SysStruct *hs);
	int terminate();
	int startup();
	int shutdown();

	int newCostume();
	int load(const char *path);
	int loadEntry(const char *path, int costumeHandle);
	int freeCostume(int costume);
	const char *getCostumeName(int costume);

	int createCosInstance(int costumeHandle, int userData);
	int freeCosInstance(int inst);

	int newStateBasedTag(int fourcc, const char *name, int unused, CosComponentHandler *handler, int param);
	int newIntTag(int fourcc, const char *name, int unused, CosComponentHandler *handler, int param);
	int newFloatTag(int fourcc, const char *name, int unused, CosComponentHandler *handler, int param);
	int registerComponent(int costumeHandle, const char *name, void *id, int fourcc, void *parentId, int parentFourcc, const CosComponentNames *names);
	int removeComponent(int costumeHandle, int fourcc, void *id);

	CosInstanceHook registerInstanceHook(CosInstanceHook hook);
	CosSaveHook registerSaveHook(CosSaveHook hook);
	CosSaveVerify registerSaveVerify(CosSaveVerify verify);
	CosCompNotifier registerCompNotifier(CosCompNotifier notifier);
	CosBitmapMunger registerBitmapMunger(CosBitmapMunger munger);

	void instRescue(int inst, int costume);
	void *instGetInstanceData(int inst, int compIndex, int *fourcc);
	int instGetUserData(int inst);
	void instSetUserData(int inst, int userData);
	int instCountComponents(int inst);
	CosComponent *instComponentInfo(int inst, int compNum, int *fourcc, void **id, int *parent);
	int instCountChores(int inst);
	CosCostume *instGetCostume(int inst, int *handle);
	const char *instChoreName(int inst, int choreIndex);

	int instStopChores(int inst);
	int instStopChore(int inst, int choreIndex);
	int instPauseChores(int inst);
	int instPauseChore(int inst, int choreIndex);
	int instCompleteChore(int inst, int choreIndex);
	int instCompleteChores(int inst);
	int instPlayChore(int inst, int choreIndex, int loop, int advance);
	int instUpdateChore(int inst, int choreIndex, int deltaMs);
	int instUpdateChores(int inst, int deltaMs);
	int instSetChoreLoop(int inst, int choreIndex, int loop);
	int instSetChoreAdv(int inst, int choreIndex, int advance);
	int instChoreStatus(int inst, int choreIndex);

	int save(Common::WriteStream *stream);
	int restore(Common::ReadStream *stream);
	int saveInstance(int inst, Common::WriteStream *stream);
	int restoreInstance(int inst, Common::ReadStream *stream);

	CosCostume *ptrFromHandle(int costume);
	void setChoreLength(CosCostume *costume, int choreNum, int length);
	int deleteComponent(int costumeHandle, int compIndex);
	int addComponent(int costume, CosComponent *comp, int fourcc, void *parentId, int parentFourcc);
	int countComponents(CosCostume *costume);
	CosComponent *componentInfo(CosCostume *costume, int compNum, int *fourcc, void **id, int *parent, int *type);

	static int compareFunction(const void *a, const void *b);

private:
	int handleToIndex(int handle);
	int indexToHandle(int index);
	int instHandleToIndex(int handle);
	int instIndexToHandle(int index);

	int allocCostumeSlot();
	int allocInstanceSlot();
	int allocTagSlot();
	CosComponentClass *newTag(int fourcc, const char *name, int param);

	void initCostume(CosCostume *costume);
	int initComponent(CosCostume *costume, CosComponent *comp, const char *name, int classIdx, void *id, int parent);
	void setComponentNames(CosComponent *comp, const CosComponentNames *names);
	void initChore(CosChore *chore);
	void initChoreInstances(CosChore *chores, CosChoreInstance *inst, int count);

	void freeCostumeData(int costumeHandle);
	void freeComponent(int costume, CosComponentClass *classInfo, CosComponent *comp);
	void freeInstance(int index);

	int findClass(int fourcc, const char *name, int type);
	int findComponent(CosCostume *costume, int fourcc, int depthIdx, const char *name);
	int findComponentByParent(CosCostume *costume, int fourcc, int depthIdx, const char *name, int parent);

	int readLine(Common::SeekableReadStream *file, char *buffer, int size);
	int parseTags(Common::SeekableReadStream *file);
	int parseComponents(Common::SeekableReadStream *file, int costumeHandle);
	int parseChores(Common::SeekableReadStream *file, CosCostume *costume);
	int parseKeys(Common::SeekableReadStream *file, CosCostume *costume);

	int updateChoreInstance(CosCostume *costume, CosInstance *instance, int choreIndex, int deltaMs, int inst);
	void changeComponent(int op, CosComponentClass *classInfo, void *instData, CosChoreKey *key, int timeOffset, CosChoreKey *nextKey, int arg7, int inst);

	SysStruct *_hs;
	int _initialized;
	int _started;

	int _numCostumes;
	int _maxCostumes;
	CosCostume *_costumes;
	int _nextCostumeId;

	int _numClasses;
	int _maxClasses;
	CosComponentClass *_classes;

	int _numInstances;
	int _maxInstances;
	CosInstance *_instances;

	int *_compMap;
	int _compMapSize;
	int _tagMap[100];

	char _lineBuffer[1024];
	int _lineNumber;

	CosInstanceHook _instanceHook;
	CosSaveHook _saveHook;
	CosSaveVerify _saveVerify;
	CosCompNotifier _compNotifier;
	CosBitmapMunger _bitmapMunger;
};

} // End of namespace Grim

#endif
