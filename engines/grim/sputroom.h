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

#ifndef GRIM_SPUTROOM_H
#define GRIM_SPUTROOM_H

#include "common/scummsys.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class GrimEngine;
class LegacyImporter;
class RenderDroid;
class SputActorModule;
class SputRender;
class SputResource;
class StdBitmap;
class StdConffile;
class StdDisplay;
class StdLbm;
class WalkSystem;
struct SputActor;
struct SputObjectState;
struct StdVBuffer;

struct RoomCamera {
	RdVector3 position;
	RdVector3 interest;
	float roll;
	float fov;
	float nearClip;
	float farClip;
	RdCamera *prdCamera;
	RdThing *pThing;
	RdMatrix34 posMatrix;
};

// Only the demo has these. Its set files list the objects of the room and the art
// of each object in every setup, the retail ones create the object states from Lua
struct RoomObject {
	char name[32];
	RdVector3 position;
};

struct RoomObjectArt {
	char name[32];
	char aBitmapName[32];
	char aZBufferName[32];
};

struct RoomSetup {
	char aName[32];
	RoomCamera camera;
	char aBitmapName[32];
	char aZBufferName[32];
	ResRef bitmapRef;
	ResRef zbufferRef;
	SputObjectState *ObjectStates;
	RoomObjectArt *pObjectArt;
};

struct RoomLight {
	char aName[32];
	RdVector3 position;
	RdLight light;
};

struct RoomActorLink {
	int actorIndex;
	RoomActorLink *pPrev;
	RoomActorLink *pNext;
};

struct RoomActorNode {
	char name[128];
	RoomActorLink *pActors;
	RoomActorNode *pNext;
};

struct Room {
	char name[128];
	ResRef *apColormaps;
	int numcolormaps;
	int curColormap;
	char colormapName[32];
	RoomSetup *pSetups;
	uint numSetups;
	int curSetup;
	uint numObjects;
	RoomObject *pObjects;
	uint numLights;
	RoomLight aLights[64];
	RoomActorNode *pActorNode;
	ResRef roomRef;
	int structSize;
	byte minVol;
	byte maxVol;
	float maxDist;
	float panFactor;
	float maxRadius;
};

struct RoomSound {
	RoomSound *pNext;
	const char *soundName;
	RdVector3 pos;
	int minVol;
	int maxVol;
	float maxDist;
	float radius;
};

class SputRoom {
	friend class LegacyImporter;

public:
	SputRoom(RenderDroid *pRd, Res *pRes, SputResource *pSputResource, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, StdLbm *pStdLbm,
			StdConffile *pStdConffile, WalkSystem *pWalkSystem);

	void setSputRender(SputRender *pSputRender) { _pSputRender = pSputRender; }
	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }
	void setActors(SputActor *aActors) { _aActors = aActors; }
	void setSputActor(SputActorModule *pSputActor) { _pSputActor = pSputActor; }

	StdVBuffer *getSetupBackground(Room *pRoom, int setupIndex);
	StdVBuffer *getSetupZBackground(Room *pRoom, int setupIndex);
	void relocateSounds();
	void setSoundParameters(Room *pRoom, float maxDist, byte minVol, byte maxVol, float panFactor, float maxRadius);
	void getSoundParameters(Room *pRoom, float *maxDist, byte *minVol, byte *maxVol, float *panFactor, float *maxRadius);
	int setSoundPosition(const char *soundName, float *pPos, int minVol, int maxVol, float maxDist, float radius);
	void setCurrentRoom(Room *pRoom);
	Room *getCurrentRoom();
	const char *getCurrentRoomName();
	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);
	int initialize(SysStruct *pHS);
	StdVBuffer *loadZBuffer(const char *fileName);
	void freeZBuffer(StdVBuffer *&pZBuffer);
	Room *loadRoom(const char *fileName);
	void freeRoom(Room *&pRoom);
	int loadResources(Room *pRoom);
	int terminate();
	int startup();
	int shutdown();
	RoomLight *getLight(Room *pRoom, int index);
	int findLightByName(Room *pRoom, const char *name);
	SputObjectState *addObjectState(int setupIndex, int objectType, const char *bitmapName, const char *zbufferName, int copyMode);
	void removeObjectState(SputObjectState *pObj);
	void removeActor(const char *roomName, SputActor *pActor);
	void addActor(const char *roomName, SputActor *pActor);
	RoomActorLink *getRoomActors(const char *roomName);
	RoomActorNode *findActorNode(const char *roomName);

private:
	typedef int (SputRoom::*SectionFunc)();

	struct Section {
		char name[32];
		SectionFunc handler;
	};

	StdVBuffer *loadZBufferEntry(const char *fileName);
	void freeZBufferEntry(StdVBuffer *&pZBuffer);
	void newRoomEntry(Room *pRoom);
	int loadRoomEntry(const char *fileName, Room *pRoom);
	void unReference(Room *pRoom);
	void freeRoomEntry(Room *pRoom);
	void freeRoomSetups(Room *pRoom);
	void freeSetupEntry(Room *pRoom, RoomSetup *pSetup);
	void freeSoundTable();
	RoomSound *allocSoundTable(const char *soundName, float *pPos, int minVol, int maxVol, float maxDist, float radius);
	int setupSoundPosition(RoomSound *pSound);
	int readSections();
	int readSectionFunc(const char *name, SectionFunc pfnRead);
	int sectionNameCompare(const char *name);
	int loadSections(const char *fileName);
	int findSectionHandler(const char *name);
	int parseLights();
	int parseSetups();
	int readObjectsSection();
	void createObjectStates(Room *pRoom, RoomSetup *pSetup, int setupIndex);
	int parseColormaps();
	int parseSectors();
	RoomActorNode *allocActorNode(const char *roomName);
	void freeActorNode(RoomActorNode *pNode);

	RenderDroid *_pRd;
	Res *_pRes;
	SputResource *_pSputResource;
	StdDisplay *_pStdDisplay;
	StdBitmap *_pStdBitmap;
	StdLbm *_pStdLbm;
	StdConffile *_pStdConffile;
	WalkSystem *_pWalkSystem;
	SputRender *_pSputRender;
	GrimEngine *_pGrim;
	SputActorModule *_pSputActor;
	SputActor *_aActors;
	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;
	ResRef _currentRoomRef;
	char _currentRoomName[32];
	RoomSound *_pSoundList;
	RoomActorNode *_pActorNodeList;
	Room *_pLoadingRoom;
	int _bUpdateLoop;
	int _numSections;
	Section _aSections[32];
};

} // End of namespace Grim

#endif
