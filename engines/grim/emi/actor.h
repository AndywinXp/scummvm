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

#include "engines/grim/emi/cvector.h"
#include "engines/grim/emi/extra_v2.h"
#include "engines/grim/emi/irect.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/objectcorners_v2.h"
#include "engines/grim/emi/rnode.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/shadow_v2.h"

namespace Grim {

class ANode;
class Camera;
class CDrawContext;
class CDCVertexArray;
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

	explicit Actor(const char *newActorName);
	~Actor() override;

	static const float kDegreesToRadians;
	static IRect offscreenTextRect;

	static int allocateActor(const char *newActorName);
	static void freeActor(int actorId);
	void allocateActorExtra();
	bool putInSet(const char *roomName);
	static bool startup(CDrawContext *drawContext);
	static void shutdown();
	static void freeAllActors();
	static int setSelectedActor(int actorId);
	static Actor *getSelectedActor();
	static Actor *actorDeref(int actorId);
	static int getActorHandle(Actor *actor);

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

	static void updateAllActors(float deltaTime);
	void update(float deltaTime);
	void followPath(float deltaTime);
	void updateWalk(float deltaTime);
	bool alignToPuck(Puck_v2 *puck);
	void turn(float degrees);
	void forward();
	static void updateSpeechOnly(float deltaTime);
	void updateActorSpeech(float deltaTime);
	void sayLine(const char *text, int volume, int pan, int x, int y);
	TextEvent *createActorText(const LString &text, int x, int y);
	void shutUp(bool stopNow);
	bool isVoicePlaying();
	static int getNumActorsTalking();
	static int setSpeechMode(int newSpeechMode);
	static int getSpeechMode();
	static int setActorTextRate(int newTextRate);
	static int getActorTextRate();

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
	static bool testCollision(float startX, float startY, float startZ, float endX, float endY, float endZ, float originX, float originY, float originZ, const float *box, const float *transform);
	static int findClosestCandidate(float startX, float startY, float startZ, float goalX, float goalY, float goalZ, const ObjectCorners_v2 *corners);
	static bool updatePositions();
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
	static void saveActors(tSaveRestoreAccess *access);
	static void restoreActors(tSaveRestoreAccess *access);

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
	static CVector _hitPoint;
	static CVector _collisionOldPosition;
	static CVector _collisionNewPosition;

	static int _nextId;
	static Common::List<Actor *> _actors;
	static Actor *_selectedActor;
	static Actor *_speakers[kNumSpeakers];
	static int _numSpeakers;
	static int _speechMode;
	static int _actorTextRate;
	static Actor *_collisionActors[kMaxCollisionActors];
	static int _collisionCount;
	static CDrawContext *_drawContext;
	static CDCVertexArray *_shadowVertices;
};

} // End of namespace Grim

#endif
