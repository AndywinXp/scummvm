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

#ifndef GRIM_EMI_ACTOR_H
#define GRIM_EMI_ACTOR_H

#include "common/array.h"
#include "common/list.h"

#include "engines/grim/emi/math/cvector.h"
#include "engines/grim/emi/actor/extra_v2.h"
#include "engines/grim/emi/math/irect.h"
#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/resources/lresourcehandle.h"
#include "engines/grim/emi/actor/objectcorners_v2.h"
#include "engines/grim/emi/nodes/rnode.h"
#include "engines/grim/emi/scene/room_v2.h"
#include "engines/grim/emi/actor/shadow_v2.h"

namespace Grim {

class ANode;
class Camera;
class CDrawContext;
class CDCVertexArray;
class EmiEngine;
class Puck_v2;
class RPrimitiveInstance;
class TextEvent;
class WalkPlane_v2;

class Actor : public RNode {
public:
	enum {
		kNumChoreSlots = 16,
		kNumSpeakers = 4,
		kMaxCollisionActors = 256
	};

	Actor(EmiEngine *engine, const char *newActorName);
	~Actor() override;

	static const float kDegreesToRadians;

	static int allocateActor(EmiEngine *engine, const char *newActorName);
	static void freeActor(EmiEngine *engine, int actorId);
	void allocateActorExtra();
	bool putInSet(const char *roomName);
	static bool startup(EmiEngine *engine, CDrawContext *drawContext);
	static void shutdown(EmiEngine *engine);
	static void freeAllActors(EmiEngine *engine);
	static int setSelectedActor(EmiEngine *engine, int actorId);
	static Actor *getSelectedActor(EmiEngine *engine);
	static Actor *actorDeref(EmiEngine *engine, int actorId);
	static int getActorHandle(EmiEngine *engine, Actor *actor);

	void removeFinishedChores();
	uint *findChoreInstance(uint *outId, uint previousId);
	uint *startChore(uint *outId, const LString &choreName, const LString &choreSetName);
	void stopChores(bool skipLooping);
	void setChore(int slot, LString choreName, LString choreSetName);
	void setWalkChore(LString choreName, LString choreSetName);
	void setTurnChores(LString leftChoreName, LString leftChoreSetName, LString rightChoreName, LString rightChoreSetName);
	void setRestChore(LString choreName, LString choreSetName);
	void setMumbleChore(LString choreName, LString choreSetName);
	void setTalkChore(int index, LString choreName, LString choreSetName);
	void updateWalkChores();

	void attachNode(CNode *node);
	void detachNode(CNode *node);
	void registerANodes(CNode *node);
	void unregisterANodes(CNode *node);
	uint findShadowPrimitive(RPrimitiveInstance *instance);
	void removeShadowPrimitive(int index);

	static void updateAllActors(EmiEngine *engine, float deltaTime);
	void update(float deltaTime);
	void followPath(float deltaTime);
	void updateWalk(float deltaTime);
	bool alignToPuck(Puck_v2 *puck);
	void turn(float degrees);
	void forward();
	static void updateSpeechOnly(EmiEngine *engine, float deltaTime);
	void updateActorSpeech(float deltaTime);
	void sayLine(const char *text, int volume, int pan, int x, int y);
	TextEvent *createActorText(const LString &text, int x, int y);
	void shutUp(bool stopNow);
	bool isVoicePlaying();
	static int getNumActorsTalking(EmiEngine *engine);
	static int setSpeechMode(EmiEngine *engine, int newSpeechMode);
	static int getSpeechMode(EmiEngine *engine);
	static int setActorTextRate(EmiEngine *engine, int newTextRate);
	static int getActorTextRate(EmiEngine *engine);

	void enableWalkSystem(bool enable);
	void enablePuck(bool enable);
	void setActorPosition(const float *position);
	void setActorRotation(const float *rotation, bool snap);
	void setReflection(float angle);
	void setWalkDominate(bool dominate);
	void leaveSector(WalkPlane_v2 *plane);
	void actorWalkTo(float *target, const float *goal, Actor *other);
	int buildAvoidingPath(Actor *other, float *path, int count, float *outPath);

	static void getOuttaTheBox(float *point, const float *box, const float *transform);
	static bool testCollision(EmiEngine *engine, float startX, float startY, float startZ, float endX, float endY, float endZ, float originX, float originY, float originZ, const float *box, const float *transform);
	static int findClosestCandidate(float startX, float startY, float startZ, float goalX, float goalY, float goalZ, const ObjectCorners_v2 *corners);
	static bool updatePositions(EmiEngine *engine);
	bool resolveCollisions();

	int getSortOrder(int setupIndex);
	bool calcOnScreen(Camera *camera);
	int drawActor(Camera *camera);
	void activateShadow(bool active, Shadow_v2 *shadow);
	int findShadowIndex(Shadow_v2 *shadow);
	int drawShadow(Camera *camera, Shadow_v2 *shadow);

	bool setActorHead(LString jointName, float *params);
	bool setHeadLimits(float horizontalLimit, float upLimit, float downLimit);
	bool canActorSeePoint(const float *point, float *out);
	bool canActorSeeActor(Actor *other, float *out);
	void setLookAtJoints(float deltaTime);

	bool save(tSaveRestoreAccess *access);
	bool restore(tSaveRestoreAccess *access);
	static void saveActors(EmiEngine *engine, tSaveRestoreAccess *access);
	static void restoreActors(EmiEngine *engine, tSaveRestoreAccess *access);

	unsigned int actorFlags;
	unsigned int mode;
	CVector prevPos;
	float reserved312;
	bool onScreen;
	bool inOverworld;
	int drawnRect[4];
	LResourceHandle<Room_v2> room;
	int sortOrder;
	int parentActorId;
	char parentNodeName[CNode::kNameSize];
	Common::List<uint> choreIds;
	Common::List<ANode *> animNodes;
	Extra_v2 *extra;
	Common::Array<Shadow_v2 *> shadows;
	Common::Array<RPrimitiveInstance *> shadowMeshes;
	uint choreSlotIds[kNumChoreSlots];
	LString choreSlotNames[kNumChoreSlots];
	LString choreSlotSetNames[kNumChoreSlots];
	int id;
	LString actorName;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
