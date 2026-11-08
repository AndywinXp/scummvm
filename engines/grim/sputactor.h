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

#ifndef GRIM_SPUTACTOR_H
#define GRIM_SPUTACTOR_H

#include "common/scummsys.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"
#include "engines/grim/std/std_display.h"

namespace Common {
class ReadStream;
class WriteStream;
}

namespace Grim {

class Cos;
class GrimEngine;
class LegacyImporter;
class Lua_V1;
class Puck;
class SputCostume;
class SputRender;
class SputResource;
class SputRoom;
class SputText;
class SputTime;
class Std3D;
class StdDisplay;
class StdMath;
class SysStruct;
class WalkPlane;
class WalkSystem;
struct ModelInst;
struct SputTextEvent;

struct ObjectCorners {
	RdVector3 aCorner[4];
	int aValid[4];
};

struct CostumeNode {
	int hCostumeInstance;
	RdThing *pHeadThing;
	ModelInst *pHeadList;
	int aHeadJoint[3];
	float headTurnRate;
	float headYawLimit;
	float headPitchLimit;
	float headLookRef;
	int walkChore;
	int turnLeftChore;
	int turnRightChore;
	int restChore;
	int aChore[16];
	int mumbleChore;
	ResRef rCostume;
	CostumeNode *pNext;
};

struct ActorShadow {
	int bActive;
	int side;
	RdVector3 normal;
	float planeD;
	RdVector3 lightPos;
	WalkPlane *apClipPlane[12];
};

struct Extra {
	RdVector3 lookCarrot;
	int bWalking;
	int turnDir;
	int resting;
	float walkRate;
	float turnRate;
	int field_20;
	float offsetYaw;
	float alignOff;
	RdVector3 walkTarget;
	RdVector3 goal;
	int numPathPoints;
	RdVector3 aPath[30];
	int pathIndex;
	Puck *puck;
	int inBoxes;
	int walkDominate;
	float reflectionAngle;
	int talking;
	int soundId;
	int talkTimerMs;
	char sayLineId[12];
	char voiceName[16];
	SputTextEvent *pSayText;
	int phoneme;
	int activeShadow;
	ActorShadow aShadows[6];
	int collisionMode;
	float collisionScale;
	int field_3F8;
	int clipPlaneActive;
	RdVector3 clipPlaneNorm;
	RdVector3 clipPlanePt;
	int talkColor;
	float timeScale;
	int subClipNode;
};

struct SputActor {
	ResRef *pColorMap;
	int flags;
	int mode;
	RdVector3 actorPos;
	RdVector3 prevPosition;
	RdVector3 actorRot;
	float scale;
	int matrixValid;
	RdMatrix34 orientMatrix;
	int onScreen;
	StdRect drawnRect;
	char curSetName[32];
	int bInUse;
	CostumeNode *pCostumeStack;
	Extra *pExtra;
};

struct LipSyncEntry {
	int phoneme;
	int mouthAnim;
};

class SputActorModule {
	friend class LegacyImporter;

public:
	SputActorModule(Res *pRes, RenderDroid *pRd, StdMath *pStdMath, StdDisplay *pStdDisplay, WalkSystem *pWalkSystem, SputResource *pSputResource,
			SputRoom *pSputRoom, SputRender *pSputRender, SputCostume *pSputCostume, SputText *pSputText, SputTime *pSputTime);

	void setGrimEngine(GrimEngine *pGrim) { _pGrim = pGrim; }
	void setLua(Lua_V1 *pLua) { _pLua = pLua; }
	void setStd3D(Std3D *pStd3D) { _pStd3D = pStd3D; }
	SputActor *getActors() { return _aActors; }
	SputActor *getSelectedActor() { return _pSelectedActor; }
	void clearShadowMaterial() { _pShadowMaterial = nullptr; }

	void nukeCostumeStacks();
	int initialize(SysStruct *pHS);
	int save(Common::WriteStream *pStream);
	int restore(Common::ReadStream *pStream);
	int getNumActorsTalking();
	int setCostume(SputActor *pActor, const char *costumeName, int hInstance);
	void setActorHead(SputActor *pActor, uint joint1, uint joint2, uint joint3, float maxAngle, float yawLimit, float pitchLimit);
	int canActorSeePoint(SputActor *pActor, float *pPoint, float *pDistance);
	int canActorSeeActor(SputActor *pActor, SputActor *pTarget, float *pDistance);
	void turn(SputActor *pActor, float dir);
	void forward(SputActor *pActor);
	void updateSpeechOnly(float delta);
	void updateActors(float delta);
	void updateActorsDemo(float delta);
	void updateCostumesMain(SputActor *pActor, float delta);
	void updateCostumesMainDemo(SputActor *pActor, float delta);
	void stopActorDemo(SputActor *pActor);
	void sayLineDemo(SputActor *pActor, const char *msgId);
	void shutUpDemo(SputActor *pActor);
	int isVoicePlayingDemo(SputActor *pActor);
	void updateCostumes2(SputActor *pActor, float delta);
	void updateActorSpeech(SputActor *pActor, float delta);
	int setSelectedActor(int handle);
	SputActor *actorDeref(int handle);
	int getActorHandle(SputActor *pActor);
	int allocateActor();
	void allocateActorExtra(SputActor *pActor);
	void enableWalkSystem(SputActor *pActor, int bEnable);
	void enablePuck(SputActor *pActor, int bEnable);
	void setActorPosition(SputActor *pActor, float *pPos);
	void setActorRotation(SputActor *pActor, float *pRot, int bFlipTurn);
	void setReflection(SputActor *pActor, float angle);
	void setWalkChore(SputActor *pActor, int choreIndex, CostumeNode *pNode);
	void setTurnChores(SputActor *pActor, int leftChore, int rightChore, CostumeNode *pNode);
	void setRestChore(SputActor *pActor, int choreIndex, CostumeNode *pNode);
	int pushCostume(SputActor *pActor, const char *costumeName, int hInstance);
	void popCostume(SputActor *pActor, int bFreeInstance);
	int matchPhonemeWithAnim(SputActor *pActor, int phoneme);
	static int compareAnimEntry(const void *pKey, const void *pEntry);
	int terminate();
	int startup();
	void shutdown();
	void updateCostumes3(SputActor *pActor);
	void updateShadow(SputActor *pActor);
	int updateLookAt(SputActor *pActor, Puck *pPuck);
	void updateCostumes4(SputActor *pActor);
	void actorWalkTo(SputActor *pActor, float *pPos, float *pGoal, SputActor *pTarget);
	int actorWalkToImpl(SputActor *pActor, SputActor *pTarget, float *pPath, int numPoints, float *pOutPath);
	void getOuttaTheBox(float *pPoint, const RdBound3 &box, RdMatrix34 pMatrix, float *pOrigin);
	int checkCollisions(float *pFrom, float *pTo, float *pCenter, const RdBound3 &box, RdMatrix34 pMatrix, float *pOffset);
	void findObjectCorners(RdModel3 *pModel, float margin, RdMatrix34 mat, float z, ObjectCorners *pOut);
	int testCollision(float *pFrom, float *pTo, float *pCenter, const RdBound3 &box, RdMatrix34 pMatrix);
	int findClosestCandidate(float *pFrom, float *pTo, ObjectCorners *pCorners);
	void actorPlayVoiceDrawText(SputActor *pActor, const char *msgId, int bVoiceStream, int volume, int pan, int x, int y);
	void shutUp(SputActor *pActor);
	void setSpeechMode(int mode);
	int getSpeechMode();
	void leaveSector(SputActor *pActor, WalkPlane *pSector);
	void updatePuckMovement(SputActor *pActor);
	void setWalkDominate(SputActor *pActor, int bDominate);
	void setActorConstrain(SputActor *pActor, int bConstrain);
	void fadeInChore(int hInst, int choreIndex, int fadeMs);
	void fadeOutChore(int hInst, int choreIndex, int fadeMs);
	int isVoicePlaying(SputActor *pActor);
	int actorHasShadow(SputActor *pActor);
	void activateActorShadow(SputActor *pActor, int shadowIdx, int bActive);
	void setShadowPlane(SputActor *pActor, const char *planeName);
	void setShadowPoint(SputActor *pActor, float x, float y, float z);
	void setShadowValid(SputActor *pActor, int shadowIdx, int side);
	void addShadowClipPlane(SputActor *pActor, int shadowIdx, const char *planeName);
	void resetShadows(SputActor *pActor);
	void loadShadowResources(SputActor *pActor);
	float (*getOrientMatrix(SputActor *pActor))[3];
	void drawToClean(SputActor *pActor);
	void updatePositions();
	void updatePosition(SputActor *pActor);
	int sweepIntoBox(float *pFrom, float *pTo, const RdBound3 &box, float *pOutPos);
	int countCostumes(SputActor *pActor);
	void setClipPlane(SputActor *pActor, float *pNormal, float *pPoint);
	void setClipPlaneActive(SputActor *pActor, int bActive);
	int getActorTextRate();
	void setActorTextRate(int rate);
	void setFrustrumCull(SputActor *pActor, int bCull);
	void freeActor(SputActor *pActor);
	void setWalkSystemFadeTime(int fadeMs);
	void initShadow(CostumeNode *pNode, ActorShadow *pShadow);
	void drawShadowPolygon(const float *pPoly, int numPoly, float *pMin, float *pMax, ActorShadow *pShadow);
	CostumeNode *allocCostumeNode();
	void initCostumeNode(CostumeNode *pNode);
	void drawCostume(CostumeNode *pNode, RdMatrix34 pMatrix);
	void drawCostumeImpl(CostumeNode *pNode, RdMatrix34 pMatrix);
	void applyPendingScale(SputActor *pActor, ModelInst *pList);
	void syncCostumeComponents(int hInst);
	void buildJointMatricesRec(ModelInst *pInst, RdMatrix34 pMatrix);
	void enumCostumeComponents(int hInst);

private:
	void publishActors();

	Res *_pRes;
	RenderDroid *_pRd;
	StdMath *_pStdMath;
	StdDisplay *_pStdDisplay;
	WalkSystem *_pWalkSystem;
	SputResource *_pSputResource;
	SputRoom *_pSputRoom;
	SputRender *_pSputRender;
	SputCostume *_pSputCostume;
	SputText *_pSputText;
	SputTime *_pSputTime;
	GrimEngine *_pGrim;
	Lua_V1 *_pLua = nullptr;
	Std3D *_pStd3D = nullptr;
	Cos *_pCos;

	SysStruct *_pHS;
	int _bInitialized;
	int _bStarted;

	SputActor *_aActors;
	int _maxActors;
	int _numActors;
	SputActor *_pSelectedActor;
	int _numActorsTalking;
	SputActor *_apTalkingActors[4];
	SputActor *_apCollisionActors[256];
	int _numCollisionActors;
	float _tmpPos0[3];
	float _tmpPos1[3];
	float _tmpPos2[3];
	int _speechMode;
	int _textRate;
	int _walkSystemFadeTime;
	LipSyncEntry _aLipSyncAnims[46];
	int _numLipSyncAnims;
	RdMaterial *_pShadowMaterial;
	RdMaterial _demoShadowMaterial;
	RdMatCel _demoShadowCel;
};

} // End of namespace Grim

#endif
