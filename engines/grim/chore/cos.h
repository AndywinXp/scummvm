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
	CosChoreKey *aKeys;
};

struct CosChore {
	char name[32];
	int numTracks;
	CosChoreTrack *aTracks;
	int length;
};

struct CosStateBitmapDesc {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
};

struct CosStateBitmap {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
};

struct CosComponentNames {
	int numNames;
	const int *aNameIds;
	const char *const *aNames;
	const CosStateBitmapDesc *aStateBitmaps;
	const int *aBitmapExtra;
};

struct CosComponent {
	char name[32];
	int classIdx;
	void *id;
	int depthIdx;
	int parent;
	int numNames;
	int *aNameIds;
	char **aNames;
	CosStateBitmap **apStateBitmaps;
};

struct CosCostume {
	char name[32];
	int field_20;
	int uniqueId;
	int numComponents;
	CosComponent *aComponents;
	int numChores;
	CosChore *aChores;
	int bFree;
};

struct CosChoreInstance {
	uint32 state;
	uint32 choreIndex;
	int time;
};

struct CosInstance {
	int costume;
	CosChoreInstance *aChoreInstances;
	int userData;
	void **aInstanceData;
	int bFree;
};

class CosComponentHandler {
public:
	virtual ~CosComponentHandler() {}

	virtual void change(int fourcc, void *pInstData, int op, int keyValue, int timeOffset, int nextKeyValue, int arg7, int hInst) {}
	virtual int request(int hCostume, int fourcc, void *pParentId, int parentFourcc, CosComponent *pComp) { return 0; }
	virtual void freeComponent(int hCostume, int fourcc, void *pCompRes) {}
	virtual void stop(void *pInstData) {}
	virtual int adjust(void **ppCompRes, char *args, int argsSize) { return 0; }
	virtual bool hasAdjuster() const { return false; }
	virtual void *instance(int hInst, int compIndex, int bCreate) { return nullptr; }
	virtual void saveLoad(int hInst, int compIndex, int bSave, void *pInstData, Common::WriteStream *pWrite, Common::ReadStream *pRead) {}
};

struct CosComponentClass {
	int fourcc;
	int type;
	char name[32];
	int param;
	CosComponentHandler *pHandler;
	int field_48[5];
};

typedef int (*CosInstanceHook)(int hInst, int bCreate);
typedef int (*CosSaveHook)(const char *path, int hCostume, const char *header);
typedef int (*CosSaveVerify)(const char *costumeName);
typedef int (*CosCompNotifier)(CosCostume *pCostume, CosComponent *pComp, int compIndex, int bAdded);
typedef int (*CosBitmapMunger)(const CosStateBitmapDesc *pDesc, int extra);

class Cos {
	friend class CosUtil;

public:
	Cos();

	int initialize(SysStruct *pHS);
	int terminate();
	int startup();
	int shutdown();

	int newCostume();
	int load(const char *path);
	int loadEntry(const char *path, int hCostume);
	int freeCostume(int hCostume);
	const char *getCostumeName(int hCostume);

	int createCosInstance(int hCostume, int userData);
	int freeCosInstance(int hInst);

	int newStateBasedTag(int fourcc, const char *name, int unused, CosComponentHandler *pHandler, int param);
	int newIntTag(int fourcc, const char *name, int unused, CosComponentHandler *pHandler, int param);
	int newFloatTag(int fourcc, const char *name, int unused, CosComponentHandler *pHandler, int param);
	int registerComponent(int hCostume, const char *name, void *id, int fourcc, void *pParentId, int parentFourcc, const CosComponentNames *pNames);
	int removeComponent(int hCostume, int fourcc, void *id);

	CosInstanceHook registerInstanceHook(CosInstanceHook pHook);
	CosSaveHook registerSaveHook(CosSaveHook pHook);
	CosSaveVerify registerSaveVerify(CosSaveVerify pVerify);
	CosCompNotifier registerCompNotifier(CosCompNotifier pNotifier);
	CosBitmapMunger registerBitmapMunger(CosBitmapMunger pMunger);

	void instRescue(int hInst, int hCostume);
	void *instGetInstanceData(int hInst, int compIndex, int *pFourcc);
	int instGetUserData(int hInst);
	void instSetUserData(int hInst, int userData);
	int instCountComponents(int hInst);
	CosComponent *instComponentInfo(int hInst, int compNum, int *pFourcc, void **pId, int *pParent);
	int instCountChores(int hInst);
	CosCostume *instGetCostume(int hInst, int *pHandle);
	const char *instChoreName(int hInst, int choreIndex);

	int instStopChores(int hInst);
	int instStopChore(int hInst, int choreIndex);
	int instPauseChores(int hInst);
	int instPauseChore(int hInst, int choreIndex);
	int instCompleteChore(int hInst, int choreIndex);
	int instCompleteChores(int hInst);
	int instPlayChore(int hInst, int choreIndex, int bLoop, int bAdvance);
	int instUpdateChore(int hInst, int choreIndex, int deltaMs);
	int instUpdateChores(int hInst, int deltaMs);
	int instSetChoreLoop(int hInst, int choreIndex, int bLoop);
	int instSetChoreAdv(int hInst, int choreIndex, int bAdvance);
	int instChoreStatus(int hInst, int choreIndex);

	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);
	int saveInstance(int hInst, Common::WriteStream *pStream);
	int restoreInstance(int hInst, Common::ReadStream *pStream);

	CosCostume *ptrFromHandle(int hCostume);
	void setChoreLength(CosCostume *pCostume, int choreNum, int length);
	int deleteComponent(int hCostume, int compIndex);
	int addComponent(int hCostume, CosComponent *pComp, int fourcc, void *pParentId, int parentFourcc);
	int countComponents(CosCostume *pCostume);
	CosComponent *componentInfo(CosCostume *pCostume, int compNum, int *pFourcc, void **pId, int *pParent, int *pType);

	static int compareFunction(const void *pA, const void *pB);

private:
	int handleToIndex(int handle);
	int indexToHandle(int index);
	int instHandleToIndex(int handle);
	int instIndexToHandle(int index);

	int allocCostumeSlot();
	int allocInstanceSlot();
	int allocTagSlot();
	CosComponentClass *newTag(int fourcc, const char *name, int param);

	void initCostume(CosCostume *pCostume);
	int initComponent(CosCostume *pCostume, CosComponent *pComp, const char *name, int classIdx, void *id, int parent);
	void setComponentNames(CosComponent *pComp, const CosComponentNames *pNames);
	void initChore(CosChore *pChore);
	void initChoreInstances(CosChore *aChores, CosChoreInstance *aInst, int count);

	void freeCostumeData(int hCostume);
	void freeComponent(int hCostume, CosComponentClass *pClass, CosComponent *pComp);
	void freeInstance(int index);

	int findClass(int fourcc, const char *name, int type);
	int findComponent(CosCostume *pCostume, int fourcc, int depthIdx, const char *name);
	int findComponentByParent(CosCostume *pCostume, int fourcc, int depthIdx, const char *name, int parent);

	int readLine(Common::SeekableReadStream *pFile, char *pBuffer, int size);
	int parseTags(Common::SeekableReadStream *pFile);
	int parseComponents(Common::SeekableReadStream *pFile, int hCostume);
	int parseChores(Common::SeekableReadStream *pFile, CosCostume *pCostume);
	int parseKeys(Common::SeekableReadStream *pFile, CosCostume *pCostume);

	int updateChoreInstance(CosCostume *pCostume, CosInstance *pInstance, int choreIndex, int deltaMs, int hInst);
	void changeComponent(int op, CosComponentClass *pClass, void *pInstData, CosChoreKey *pKey, int timeOffset, CosChoreKey *pNextKey, int arg7, int hInst);

	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;

	int _numCostumes;
	int _maxCostumes;
	CosCostume *_aCostumes;
	int _nextCostumeId;

	int _numClasses;
	int _maxClasses;
	CosComponentClass *_aClasses;

	int _numInstances;
	int _maxInstances;
	CosInstance *_aInstances;

	int *_aCompMap;
	int _compMapSize;
	int _aTagMap[100];

	char _lineBuffer[1024];
	int _lineNumber;

	CosInstanceHook _pInstanceHook;
	CosSaveHook _pSaveHook;
	CosSaveVerify _pSaveVerify;
	CosCompNotifier _pCompNotifier;
	CosBitmapMunger _pBitmapMunger;
};

} // End of namespace Grim

#endif
