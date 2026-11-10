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

#include <math.h>

#include "common/str.h"
#include "common/textconsole.h"
#include "common/ustr.h"

#include "gui/error.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/anode.h"
#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdcvertexarray.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/cplane.h"
#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lipdata.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/puck_v2.h"
#include "engines/grim/emi/rmesh.h"
#include "engines/grim/emi/rmeshinstance.h"
#include "engines/grim/emi/rprimitiveinstance.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/textevent.h"
#include "engines/grim/emi/walkplane_v2.h"
#include "engines/grim/emi/walkplanecollection.h"
#include "engines/grim/emi/wpvertex.h"

namespace Grim {

const float Actor::kDegreesToRadians = 3.14159274f * 0.00555555569f;

int Actor::_nextId = 1;
Common::List<Actor *> Actor::_actors;
Actor *Actor::_selectedActor = nullptr;
Actor *Actor::_speakers[Actor::kNumSpeakers];
int Actor::_numSpeakers = 0;
int Actor::_speechMode = 2;
int Actor::_actorTextRate = 6;
IRect Actor::offscreenTextRect;
Actor *Actor::_collisionActors[Actor::kMaxCollisionActors];
int Actor::_collisionCount = 0;
CDrawContext *Actor::_drawContext = nullptr;
CDCVertexArray *Actor::_shadowVertices = nullptr;
CVector Actor::_hitPoint;
CVector Actor::_collisionOldPosition;
CVector Actor::_collisionNewPosition;

Actor::Actor(const char *newActorName) {
	prevPos.x = 0.0f;
	prevPos.y = 0.0f;
	prevPos.z = 0.0f;
	drawnRect[0] = 0;
	drawnRect[1] = 0;
	drawnRect[2] = 0;
	drawnRect[3] = 0;
	memset(choreSlotIds, 0, kNumChoreSlots * sizeof(uint));

	id = _nextId++;
	_actors.push_back(this);

	if (newActorName)
		actorName = newActorName;
	else
		warning("Actor::Actor(): the actor name is null");

	actorFlags = 0;
	mode = 0;
	reserved312 = 0.0f;
	onScreen = false;
	inOverworld = false;
	room.setResource(nullptr);
	sortOrder = 0;
	parentActorId = 0;
	memset(parentNodeName, 0, sizeof(parentNodeName));
	drawFlags |= 0x21000;

	for (int i = 0; i < kNumChoreSlots; i++) {
		choreSlotIds[i] = 0;
		choreSlotNames[i].clear();
		choreSlotSetNames[i].clear();
	}

	extra = nullptr;
	Common::sprintf_s(name, "Actor %d", id);
}

Actor::~Actor() {
	if (room.resource) {
		Room_v2 *currentRoom = nullptr;

		if (room.lock())
			currentRoom = room.typed;

		if (currentRoom)
			currentRoom->removeActor(this);
		else
			warning("Actor::~Actor(): the room could not be locked");

		room.unlock();
		room.unlock();
	}

	Common::List<uint>::iterator choreIt;

	for (choreIt = choreIds.begin(); choreIt != choreIds.end(); ++choreIt) {
		if (ChoreInstance::isInstanceValid(*choreIt)) {
			ChoreInstance *instance = ChoreInstance::getInstance(*choreIt);

			if (instance)
				delete instance;
		}
	}

	choreIds.clear();

	if (extra) {
		if (extra->puck) {
			delete extra->puck;
			extra->puck = nullptr;
		}

		delete extra;
		extra = nullptr;
	}

	Common::List<Actor *>::iterator actorIt = _actors.begin();

	while (actorIt != _actors.end()) {
		if (*actorIt == this)
			actorIt = _actors.erase(actorIt);
		else
			++actorIt;
	}

	// The disasm leaves the actor in the overworld list and in the selected/speaker slots
}

int Actor::allocateActor(const char *newActorName) {
	Actor *actor = new Actor(newActorName);

	return actor->id;
}

void Actor::freeActor(int actorId) {
	Actor *actor = actorDeref(actorId);

	if (actor)
		delete actor;
}

void Actor::allocateActorExtra() {
	extra = new Extra_v2();
}

bool Actor::putInSet(const char *roomName) {
	if (room.resource) {
		activateShadow(false, nullptr);

		for (int removedBefore = 0;; removedBefore++) {
			Room_v2 *currentRoom = nullptr;

			if (room.lock())
				currentRoom = room.typed;

			int removed = 0;

			if (currentRoom)
				removed = currentRoom->removeActor(this);
			else
				warning("Actor::putInSet(): the room could not be locked");

			bool removedAgain = removedBefore < removed;

			if (currentRoom)
				room.unlock();

			if (!removedAgain)
				break;

			room.unlock();
		}

		room.setResource(nullptr);
	}

	if (!roomName) {
		removeFinishedChores();

		return true;
	}

	room.resource = gpResourceManager->findResource(roomName);

	if (!room.resource) {
		room.resource = gpResourceManager->createResource(roomName);

		if (!room.resource)
			room.allocateResource();

		if (!room.resource) {
			warning("Actor::putInSet(): could not create the room resource '%s'", roomName);

			return false;
		}

		room.resource->resSetName(roomName);
		gpResourceManager->addResource(room.resource);
	}

	room.setResource(room.resource);

	if (!room.resource)
		return false;

	if (!room.lock())
		return false;

	Room_v2 *newRoom = room.typed;

	if (!newRoom)
		return false;

	newRoom->addActor(this);

	if (extra && extra->puck && extra->inBoxes) {
		extra->puck->reset();
		extra->puck->validate(newRoom->walkPlanes);
	}

	return true;
}

bool Actor::startup(CDrawContext *drawContext) {
	_drawContext = drawContext;

	if (drawContext)
		_shadowVertices = drawContext->createVertexArray(2000, nullptr, nullptr, nullptr);

	for (int i = 0; i < kNumSpeakers; i++)
		_speakers[i] = nullptr;

	LipData::sortMouthTable();

	return true;
}

void Actor::shutdown() {
	freeAllActors();

	if (_drawContext && _shadowVertices)
		delete _shadowVertices;

	_shadowVertices = nullptr;
	_drawContext = nullptr;
}

void Actor::freeAllActors() {
	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it)
		(*it)->detachFromParent();

	it = _actors.begin();

	while (it != _actors.end()) {
		Actor *actor = *it;

		++it;

		if (actor)
			delete actor;
	}

	_actors.clear();
}

int Actor::setSelectedActor(int actorId) {
	int previousId = -1;

	if (_selectedActor)
		previousId = _selectedActor->id;

	if (actorId >= 0)
		_selectedActor = actorDeref(actorId);

	return previousId;
}

Actor *Actor::getSelectedActor() {
	return _selectedActor;
}

Actor *Actor::actorDeref(int actorId) {
	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		if (*it && (*it)->id == actorId)
			return *it;
	}

	return nullptr;
}

int Actor::getActorHandle(Actor *actor) {
	if (actor)
		return actor->id;

	return 0;
}

void Actor::removeFinishedChores() {
	Common::List<uint>::iterator it = choreIds.begin();

	while (it != choreIds.end()) {
		uint choreId = *it;
		Common::List<uint>::iterator next = it;

		++next;

		if (ChoreInstance::isInstanceValid(choreId)) {
			if (!ChoreInstance::isInstanceValid(choreId) || ChoreInstance::getInstance(choreId)->isFinished()) {
				Common::List<uint>::iterator removeIt = choreIds.begin();

				while (removeIt != choreIds.end()) {
					if (*removeIt == choreId) {
						if (removeIt == next) {
							warning("Actor::removeFinishedChores(): the chore id is listed twice");
							++next;
						}

						removeIt = choreIds.erase(removeIt);
					} else {
						++removeIt;
					}
				}

				if (ChoreInstance::isInstanceValid(choreId)) {
					ChoreInstance *instance = ChoreInstance::getInstance(choreId);

					if (instance)
						delete instance;
				}
			}
		} else {
			Common::List<uint>::iterator removeIt = choreIds.begin();

			while (removeIt != choreIds.end()) {
				if (*removeIt == choreId) {
					if (removeIt == next) {
						warning("Actor::removeFinishedChores(): the chore id is listed twice");
						++next;
					}

					removeIt = choreIds.erase(removeIt);
				} else {
					++removeIt;
				}
			}
		}

		it = next;
	}
}

uint *Actor::findChoreInstance(uint *outId, uint previousId) {
	Common::List<uint>::iterator it = choreIds.begin();

	if (previousId && it != choreIds.end()) {
		uint value;

		do {
			value = *it;
			++it;
		} while (previousId != value && it != choreIds.end());
	}

	if (it == choreIds.end())
		*outId = 0;
	else
		*outId = *it;

	return outId;
}

uint *Actor::startChore(uint *outId, const LString &choreName, const LString &choreSetName) {
	uint newId = ChoreInstance::newInstance(this, choreName, choreSetName);

	if (ChoreInstance::isInstanceValid(newId))
		choreIds.push_back(newId);

	*outId = newId;

	return outId;
}

void Actor::stopChores(bool skipLooping) {
	Common::List<uint>::iterator it;

	for (it = choreIds.begin(); it != choreIds.end(); ++it) {
		if (!skipLooping || !ChoreInstance::isInstanceValid(*it) || !ChoreInstance::getInstance(*it)->isLooping()) {
			if (ChoreInstance::isInstanceValid(*it))
				ChoreInstance::getInstance(*it)->stop();
		}
	}
}

void Actor::setChore(int slot, LString choreName, LString choreSetName) {
	if (slot < 0 || slot >= kNumChoreSlots) {
		warning("Actor::setChore(): invalid slot %d", slot);

		return;
	}

	if (!choreSlotNames[slot].empty()) {
		if (choreName == choreSlotNames[slot] && choreSetName == choreSlotSetNames[slot])
			return;

		if (ChoreInstance::isInstanceValid(choreSlotIds[slot])) {
			ChoreInstance::getInstance(choreSlotIds[slot])->stop();

			uint startedId;

			startChore(&startedId, choreName, choreSetName);
			choreSlotIds[slot] = startedId;

			if (ChoreInstance::isInstanceValid(startedId))
				ChoreInstance::getInstance(choreSlotIds[slot])->setPlaying();

			if (ChoreInstance::isInstanceValid(choreSlotIds[slot]))
				ChoreInstance::getInstance(choreSlotIds[slot])->setLooping(true);
		}
	}

	choreSlotNames[slot] = choreName;
	choreSlotSetNames[slot] = choreSetName;
}

void Actor::setWalkChore(LString choreName, LString choreSetName) {
	setChore(1, choreName, choreSetName);
}

void Actor::setTurnChores(LString leftChoreName, LString leftChoreSetName, LString rightChoreName, LString rightChoreSetName) {
	setChore(2, leftChoreName, leftChoreSetName);
	setChore(3, rightChoreName, rightChoreSetName);
}

void Actor::setRestChore(LString choreName, LString choreSetName) {
	setChore(0, choreName, choreSetName);
}

void Actor::setMumbleChore(LString choreName, LString choreSetName) {
	setChore(4, choreName, choreSetName);
}

void Actor::setTalkChore(int index, LString choreName, LString choreSetName) {
	setChore(index + 5, choreName, choreSetName);
}

void Actor::updateWalkChores() {
	uint startedId;

	if (!ChoreInstance::isInstanceValid(choreSlotIds[0])) {
		startChore(&startedId, choreSlotNames[0], choreSlotSetNames[0]);
		choreSlotIds[0] = startedId;

		if (ChoreInstance::isInstanceValid(startedId))
			ChoreInstance::getInstance(choreSlotIds[0])->setPlaying();

		if (ChoreInstance::isInstanceValid(choreSlotIds[0]))
			ChoreInstance::getInstance(choreSlotIds[0])->setLooping(true);
	}

	if (!extra)
		return;

	if (extra->walking) {
		extra->turnDir = 0;

		if (!ChoreInstance::isInstanceValid(choreSlotIds[1])) {
			extra->resting = false;
			startChore(&startedId, choreSlotNames[1], choreSlotSetNames[1]);
			choreSlotIds[1] = startedId;

			if (ChoreInstance::isInstanceValid(startedId))
				ChoreInstance::getInstance(choreSlotIds[1])->fadeIn(0.25f);

			if (ChoreInstance::isInstanceValid(choreSlotIds[1]))
				ChoreInstance::getInstance(choreSlotIds[1])->setLooping(true);
		}
	} else if (ChoreInstance::isInstanceValid(choreSlotIds[1]) && (mode & 1) == 0) {
		ChoreInstance::getInstance(choreSlotIds[1])->fadeOut(0.25f);
		choreSlotIds[1] = 0;
	}

	if (extra->turnDir <= 0) {
		if (ChoreInstance::isInstanceValid(choreSlotIds[2]))
			ChoreInstance::getInstance(choreSlotIds[2])->fadeOut(0.25f);

		choreSlotIds[2] = 0;
	} else if (!ChoreInstance::isInstanceValid(choreSlotIds[2])) {
		extra->resting = false;
		startChore(&startedId, choreSlotNames[2], choreSlotSetNames[2]);
		choreSlotIds[2] = startedId;

		if (ChoreInstance::isInstanceValid(startedId))
			ChoreInstance::getInstance(choreSlotIds[2])->fadeIn(0.25f);

		if (ChoreInstance::isInstanceValid(choreSlotIds[2]))
			ChoreInstance::getInstance(choreSlotIds[2])->setLooping(true);
	}

	if (extra->turnDir >= 0) {
		if (ChoreInstance::isInstanceValid(choreSlotIds[3]))
			ChoreInstance::getInstance(choreSlotIds[3])->fadeOut(0.25f);

		choreSlotIds[3] = 0;
	} else if (!ChoreInstance::isInstanceValid(choreSlotIds[3])) {
		extra->resting = false;
		startChore(&startedId, choreSlotNames[3], choreSlotSetNames[3]);
		choreSlotIds[3] = startedId;

		if (ChoreInstance::isInstanceValid(startedId))
			ChoreInstance::getInstance(choreSlotIds[3])->fadeIn(0.25f);

		if (ChoreInstance::isInstanceValid(choreSlotIds[3]))
			ChoreInstance::getInstance(choreSlotIds[3])->setLooping(true);
	}

	if (!extra->walking && !extra->turnDir)
		extra->resting = true;

	extra->walking = false;
	extra->turnDir = 0;
}

void Actor::attachNode(CNode *node) {
	if (node) {
		registerANodes(node);
		node->setParent(this);
	}
}

void Actor::detachNode(CNode *node) {
	if (node) {
		unregisterANodes(node);
		node->detachFromParent();
	}
}

void Actor::registerANodes(CNode *node) {
	for (CNode *current = node; current; current = current->nextSibling) {
		registerANodes(current->firstChild);

		ANode *animNode = dynamic_cast<ANode *>(current);

		if (animNode)
			animNodes.push_back(animNode);
	}
}

void Actor::unregisterANodes(CNode *node) {
	for (CNode *current = node; current; current = current->nextSibling) {
		unregisterANodes(current->firstChild);

		ANode *animNode = dynamic_cast<ANode *>(current);

		if (animNode) {
			Common::List<ANode *>::iterator it = animNodes.begin();

			while (it != animNodes.end()) {
				if (*it == animNode)
					it = animNodes.erase(it);
				else
					++it;
			}
		}
	}
}

uint Actor::findShadowPrimitive(RPrimitiveInstance *instance) {
	for (uint i = 0; i < shadowMeshes.size(); i++) {
		if (shadowMeshes[i] == instance)
			return i;
	}

	return (uint)-1;
}

void Actor::removeShadowPrimitive(int index) {
	if (index >= 0 && (uint)index < shadowMeshes.size())
		shadowMeshes.remove_at(index);
}

static ProfilerTimer updateAllActorsTimer("Actor::UpdateAllActors()", ProfilerTimer::kTrackNesting);

void Actor::updateAllActors(float deltaTime) {
	updateAllActorsTimer.start();

	_collisionCount = 0;

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom) {
		updateAllActorsTimer.stop();

		return;
	}

	updateSpeechOnly(deltaTime);

	Common::List<Actor *>::iterator it;

	for (it = currentRoom->actors.begin(); it != currentRoom->actors.end(); ++it) {
		Actor *current = *it;

		if (!current)
			continue;

		if (current->extra)
			current->extra->inCollision = false;

		if (!current->instances.empty()) {
			if (_collisionCount < kMaxCollisionActors) {
				_collisionActors[_collisionCount] = current;
				_collisionCount++;
			} else {
				warning("Actor::updateAllActors(): too many actors in the collision list");
			}
		}

		Camera *setupCamera = currentRoom->getSetupCamera(-1);

		if (setupCamera)
			current->onScreen = current->calcOnScreen(setupCamera);

		current->update(deltaTime);
	}

	Overworld *overworld = Overworld::getInstance();

	for (it = overworld->actors.begin(); it != overworld->actors.end(); ++it) {
		Actor *current = *it;

		if (!current)
			continue;

		if (!current->instances.empty()) {
			if (_collisionCount < kMaxCollisionActors) {
				_collisionActors[_collisionCount] = current;
				_collisionCount++;
			} else {
				warning("Actor::updateAllActors(): too many actors in the collision list");
			}
		}

		current->onScreen = true;
		current->update(deltaTime);
	}

	updatePositions();
	updateAllActorsTimer.stop();
}

void Actor::updateSpeechOnly(float deltaTime) {
	for (int i = 0; i < kNumSpeakers; i++) {
		if (_speakers[i])
			_speakers[i]->updateActorSpeech(deltaTime);
	}
}

static const int kVoiceIdSize = TextEvent::kTagBufferSize;
static const int kVoicePathSize = 36;
static const int kVoiceSoundMode = 2;
static const int kMouthSlot = 4;
static const int kFirstVisemeSlot = 6;
static const int kLastVisemeSlot = 14;
static const float kVisemeFadeTime = 0.0625f;
static const int kSpeechTextFlags = 13;
static const int kMinTextRate = 1;
static const int kMaxTextRate = 10;
static const int kTextMarginX = 40;
static const int kTextMarginY = 25;

static int truncateSpeechTime(double value) {
	if (!(value > -2147483649.0 && value < 2147483648.0)) {
		warning("Actor::updateActorSpeech(): the time is out of range");

		return -2147483647 - 1;
	}

	return (int)value;
}

void Actor::updateActorSpeech(float deltaTime) {
	if (!extra) {
		warning("Actor::updateActorSpeech(): missing extra data");

		return;
	}

	if (!extra->talking)
		return;

	if (extra->soundHandle && extra->lipsyncActive) {
		int mouthShape;

		if (EmiEngine::isDumpingFrames()) {
			mouthShape = LipData::getLipMouthShape((uint)extra->soundHandle, extra->visemeMs);
			extra->visemeMs = truncateSpeechTime((double)deltaTime * 1000.0 + (double)extra->visemeMs);
		} else {
			mouthShape = LipData::getLipMouthShape((uint)extra->soundHandle, -1);
		}

		if (mouthShape == 0) {
			shutUp(false);
		} else if (mouthShape < 0 || mouthShape >= kNumChoreSlots) {
			warning("Actor::updateActorSpeech(): invalid mouth shape %d", mouthShape);
		} else if (mouthShape != extra->visemeCurrent) {
			int currentShape = extra->visemeCurrent;

			if (currentShape != 0) {
				if (currentShape < 0 || currentShape >= kNumChoreSlots) {
					warning("Actor::updateActorSpeech(): invalid current mouth shape %d", currentShape);
				} else {
					if (ChoreInstance::isInstanceValid(choreSlotIds[currentShape]))
						ChoreInstance::getInstance(choreSlotIds[currentShape])->fadeOut(kVisemeFadeTime);

					choreSlotIds[currentShape] = 0;
				}
			}

			uint startedId;

			startChore(&startedId, choreSlotNames[mouthShape], choreSlotSetNames[mouthShape]);
			choreSlotIds[mouthShape] = startedId;

			if (ChoreInstance::isInstanceValid(startedId))
				ChoreInstance::getInstance(choreSlotIds[mouthShape])->fadeIn(kVisemeFadeTime);

			extra->visemeCurrent = mouthShape;
		}
	}

	int elapsedMs = truncateSpeechTime((double)deltaTime * -1000.0);

	extra->talkTimerMs = (int)((uint)extra->talkTimerMs + (uint)elapsedMs);

	if ((extra->soundHandle && !isVoicePlaying()) || (!extra->soundHandle && extra->talkTimerMs < 0))
		shutUp(false);
}

void Actor::sayLine(const char *text, int volume, int pan, int x, int y) {
	if (!text) {
		warning("Actor::sayLine(): the text is null");

		return;
	}

	if (!extra)
		allocateActorExtra();

	bool textEnabled = (_speechMode & 1) != 0;
	bool voiceEnabled = (_speechMode & 2) != 0;
	char voiceId[kVoiceIdSize];
	char voicePath[kVoicePathSize];
	const char *lineText = TextEvent::translate(text, voiceId);

	voicePath[0] = 0;

	if (voiceId[0]) {
		Common::sprintf_s(voicePath, "%s.wvc", voiceId);

		LFile *voiceFile = gpFileServices->open(voicePath, "rb");

		if (!voiceFile) {
			Common::sprintf_s(voicePath, "%s.wav", voiceId);
			voiceFile = gpFileServices->open(voicePath, "rb");
		}

		if (voiceFile)
			gpFileServices->close(voiceFile, true);
		else
			voicePath[0] = 0;
	}

	if (extra->talking) {
		if (voiceId[0]) {
			if (!memchr(extra->sayLineId, 0, Extra_v2::kSayLineIdSize))
				warning("Actor::sayLine(): the current line id is not terminated");
			else if (strcmp(extra->sayLineId, voiceId) == 0)
				return;
		}

		shutUp(true);
	}

	extra->talkTimerMs = 0;
	extra->talking = true;

	int idLength = 0;

	while (idLength < Extra_v2::kSayLineIdSize && voiceId[idLength]) {
		extra->sayLineId[idLength] = voiceId[idLength];
		idLength++;
	}

	while (idLength < Extra_v2::kSayLineIdSize) {
		extra->sayLineId[idLength] = 0;
		idLength++;
	}

	_numSpeakers++;

	int speakerSlot = 0;

	while (speakerSlot < kNumSpeakers && _speakers[speakerSlot])
		speakerSlot++;

	if (speakerSlot < kNumSpeakers)
		_speakers[speakerSlot] = this;

	bool createText = textEnabled;

	if (voiceEnabled) {
		if (voicePath[0]) {
			extra->soundHandle = (int)Sounds::loadSound(voicePath, kVoiceSoundMode);

			if (extra->soundHandle)
				Sounds::startSound((uint)extra->soundHandle, 0, false, volume);

			extra->lipsyncActive = Sounds::getSoundLipData((uint)extra->soundHandle) != nullptr;

			if (extra->lipsyncActive)
				extra->visemeMs = 0;
		}

		extra->talkTimerMs = -1;

		if (!extra->soundHandle)
			createText = true;
	}

	if (createText) {
		LString lineString(lineText);
		TextEvent *textEvent = createActorText(lineString, x, y);

		extra->textEventId = textEvent->id;

		if (!extra->soundHandle)
			extra->talkTimerMs = (int)(10u * ((uint)(11 - _actorTextRate) * (uint)strlen(lineText) + 64u));

		textEvent->activate(-1.0f, kSpeechTextFlags);
	}

	if (extra->lipsyncActive) {
		int visemeSlot = kFirstVisemeSlot;

		while (visemeSlot <= kLastVisemeSlot && choreSlotNames[visemeSlot].empty())
			visemeSlot++;

		if (visemeSlot > kLastVisemeSlot)
			extra->lipsyncActive = false;
	}

	if (!extra->lipsyncActive) {
		uint startedId;

		startChore(&startedId, choreSlotNames[kMouthSlot], choreSlotSetNames[kMouthSlot]);
		choreSlotIds[kMouthSlot] = startedId;

		if (ChoreInstance::isInstanceValid(startedId))
			ChoreInstance::getInstance(choreSlotIds[kMouthSlot])->setPlaying();
	}
}

TextEvent *Actor::createActorText(const LString &text, int x, int y) {
	TextEvent *textEvent = new TextEvent(text);

	textEvent->geometryReady = false;
	textEvent->clipRect[0] = TextEvent::screenRect[0];
	textEvent->clipRect[1] = TextEvent::screenRect[1];
	textEvent->clipRect[2] = TextEvent::screenRect[2];
	textEvent->clipRect[3] = TextEvent::screenRect[3];

	if (extra)
		textEvent->color = extra->talkColor;
	else
		warning("Actor::createActorText(): missing extra data");

	textEvent->justify = 6;
	textEvent->geometryReady = false;

	int rectX;
	int rectY;
	int rectWidth;

	if (!onScreen || EmiEngine::getMode() == EmiEngine::kModeMovie) {
		rectX = offscreenTextRect.x;
		rectY = offscreenTextRect.y;
		rectWidth = offscreenTextRect.w;
	} else {
		rectX = drawnRect[0];
		rectY = drawnRect[1];
		rectWidth = drawnRect[2];
	}

	int halfWidth = textEvent->getWidth() / 2;
	int textX = x;

	if (x == -1)
		textX = rectX + rectWidth / 2;

	if (textX - halfWidth < kTextMarginX)
		textX = halfWidth + kTextMarginX;

	if (textX + halfWidth > TextEvent::screenRect[2] - kTextMarginX)
		textX = TextEvent::screenRect[2] - halfWidth - kTextMarginX;

	textEvent->x = textX;
	textEvent->y = 0;
	textEvent->geometryReady = false;

	int height = textEvent->getHeight();
	int textY = y;

	if (y == -1)
		textY = rectY - height;

	if (textY < kTextMarginY)
		textY = kTextMarginY;

	if (textY + height > TextEvent::screenRect[3] - kTextMarginY)
		textY = TextEvent::screenRect[3] - height - kTextMarginY;

	textEvent->x = textX;
	textEvent->y = textY;
	textEvent->geometryReady = false;
	textEvent->reserved24 = id;
	actorFlags |= 4;

	return textEvent;
}

void Actor::shutUp(bool stopNow) {
	if (!extra)
		return;

	if (extra->soundHandle) {
		if (stopNow)
			Sounds::stopSound((uint)extra->soundHandle);
		else
			Sounds::releaseSoundWhenDone((uint)extra->soundHandle);

		extra->soundHandle = 0;
	}

	if (extra->textEventId) {
		TextEvent *textEvent = TextEvent::getEvent(extra->textEventId);

		if (textEvent)
			delete textEvent;
	}

	if (ChoreInstance::isInstanceValid(choreSlotIds[kMouthSlot]))
		ChoreInstance::getInstance(choreSlotIds[kMouthSlot])->stop();

	choreSlotIds[kMouthSlot] = 0;

	int currentShape = extra->visemeCurrent;

	if (currentShape != 0) {
		if (currentShape < 0 || currentShape >= kNumChoreSlots) {
			warning("Actor::shutUp(): invalid current mouth shape %d", currentShape);
		} else {
			if (ChoreInstance::isInstanceValid(choreSlotIds[currentShape]))
				ChoreInstance::getInstance(choreSlotIds[currentShape])->fadeOut(kVisemeFadeTime);

			choreSlotIds[currentShape] = 0;
		}

		extra->visemeCurrent = 0;
	}

	for (int slot = kFirstVisemeSlot; slot <= kLastVisemeSlot; slot++) {
		if (ChoreInstance::isInstanceValid(choreSlotIds[slot])) {
			// The disasm passes the string object itself as the %s argument, let's use the name...
			Common::String message = Common::String::format("%s still talk choring", actorName.c_str());

			warning("%s", message.c_str());
			GUI::displayErrorDialog(Common::U32String(message));
		}
	}

	for (int i = 0; i < kNumSpeakers; i++) {
		if (_speakers[i] == this)
			_speakers[i] = nullptr;
	}

	extra->lipsyncActive = false;
	extra->textEventId = 0;
	extra->sayLineId[0] = 0;
	extra->talking = false;

	if (_numSpeakers > 0)
		_numSpeakers--;
}

bool Actor::isVoicePlaying() {
	if (!extra) {
		warning("Actor::isVoicePlaying(): missing extra data");

		return false;
	}

	if (!extra->soundHandle)
		return false;

	return Sounds::isSoundPlayingNoFiniteLoop((uint)extra->soundHandle);
}

int Actor::getNumActorsTalking() {
	return _numSpeakers;
}

int Actor::setSpeechMode(int newSpeechMode) {
	if (newSpeechMode != _speechMode && (newSpeechMode & 3) != 0)
		_speechMode = newSpeechMode;

	return newSpeechMode;
}

int Actor::getSpeechMode() {
	return _speechMode;
}

int Actor::setActorTextRate(int newTextRate) {
	if (newTextRate < kMinTextRate)
		newTextRate = kMinTextRate;
	else if (newTextRate > kMaxTextRate)
		newTextRate = kMaxTextRate;

	_actorTextRate = newTextRate;

	return newTextRate;
}

int Actor::getActorTextRate() {
	return _actorTextRate;
}

void Actor::update(float deltaTime) {
	if (extra)
		deltaTime = deltaTime * extra->timeScale;

	if ((mode & 1) != 0)
		followPath(deltaTime);

	if (extra && extra->puck)
		updateWalk(deltaTime);

	removeFinishedChores();
	updateWalkChores();

	Common::List<uint>::iterator choreIt;

	for (choreIt = choreIds.begin(); choreIt != choreIds.end(); ++choreIt) {
		if (ChoreInstance::isInstanceValid(*choreIt)) {
			ChoreInstance *instance = ChoreInstance::getInstance(*choreIt);

			instance->update(deltaTime, false);
		}
	}

	if (onScreen) {
		Common::List<ANode *>::iterator nodeIt;

		for (nodeIt = animNodes.begin(); nodeIt != animNodes.end(); ++nodeIt)
			(*nodeIt)->updateANode();

		if (extra && extra->lookAtActive)
			setLookAtJoints(deltaTime);
	}
}

bool Actor::updatePositions() {
	for (int i = 0; i < _collisionCount; i++) {
		Actor *current = _collisionActors[i];

		if (current->extra) {
			if ((current->nodeFlags & kWorldValid) == 0)
				current->updateWorldTransform();

			// The disasm treats an unordered comparison as equal
			bool moved = (current->world.pos.x < current->prevPos.x || current->world.pos.x > current->prevPos.x)
				|| (current->world.pos.y < current->prevPos.y || current->world.pos.y > current->prevPos.y)
				|| (current->world.pos.z < current->prevPos.z || current->world.pos.z > current->prevPos.z);

			if (moved)
				current->resolveCollisions();
		}
	}

	return true;
}

void Actor::followPath(float deltaTime) {
	const float maxAngle = 3.4028235e38f;

	if (!extra) {
		warning("Actor::followPath(): missing extra data");

		return;
	}

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	double deltaX = (double)world.pos.x - extra->walkTarget.x;
	double deltaY = (double)world.pos.y - extra->walkTarget.y;
	double deltaZ = (double)world.pos.z - extra->walkTarget.z;
	double distance = sqrt((deltaZ * deltaZ + deltaY * deltaY) + deltaX * deltaX);

	float rate = extra->walkRate;

	if (rate < 0.0f)
		rate = -rate;

	float limit = (float)((double)rate * extra->timeScale * deltaTime * 1.25);

	// The disasm takes the branches below also when the comparisons are unordered
	if (!(distance >= limit)) {
		do {
			if (extra->pathIndex >= extra->numPathPoints)
				break;

			extra->pathIndex++;
			setActorPosition(&extra->walkTarget.x);

			if (extra->pathIndex < Extra_v2::kMaxPathPoints)
				extra->walkTarget = extra->path[extra->pathIndex];
			else
				warning("Actor::followPath(): path index out of range");

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			deltaY = (double)world.pos.y - extra->walkTarget.y;
			deltaZ = (double)world.pos.z - extra->walkTarget.z;
			deltaX = (double)world.pos.x - extra->walkTarget.x;
			distance = sqrt((deltaZ * deltaZ + deltaY * deltaY) + deltaX * deltaX);
		} while (!(distance >= limit));

		if (extra->pathIndex == extra->numPathPoints) {
			mode &= ~1U;

			if (ChoreInstance::isInstanceValid(choreSlotIds[1]))
				ChoreInstance::getInstance(choreSlotIds[1])->stop();

			choreSlotIds[1] = 0;

			return;
		}
	}

	mode |= 1U;

	if (extra->puck)
		extra->puck->pointAt(&extra->walkTarget.x, maxAngle);
	else
		warning("Actor::followPath(): missing puck");

	double cosLimit = cos((double)kDegreesToRadians * 80.0);
	double cosAlign = cos((double)kDegreesToRadians * extra->alignOff);

	if (!(cosLimit >= cosAlign))
		forward();
}

void Actor::updateWalk(float deltaTime) {
	Puck_v2 *puck = extra->puck;
	int turnDirection = extra->turnDir;

	if (turnDirection != 0 && (mode & 1) == 0)
		puck->rotate((float)-((double)deltaTime * extra->timeScale * extra->turnRate * (double)turnDirection));

	if (extra->walking) {
		float distance = (float)((double)extra->timeScale * extra->walkRate * deltaTime);
		int allowLeft = 1;
		int allowRight = 1;

		if (!extra->walkDominate) {
			allowLeft = extra->turnDir != -1;
			allowRight = extra->turnDir != 1;
		}

		extra->walking = puck->forward(distance, allowLeft, allowRight);
	}

	extra->aligned = alignToPuck(puck);

	if (extra->turnDir == 0 && !extra->aligned) {
		if (extra->alignOff > 180.0f)
			extra->turnDir = 1;
		else
			extra->turnDir = -1;
	}

	if ((puck->nodeFlags & kPositionValid) == 0)
		puck->computeLocalPositionFromWorld();

	setLocalPosition(puck->pos);
}

bool Actor::alignToPuck(Puck_v2 *puck) {
	if ((nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	float savedX = eulerDeg.x;
	float savedY = eulerDeg.y;
	float savedZ = eulerDeg.z;

	if ((puck->nodeFlags & kEulerValid) == 0)
		puck->computeLocalMatrixFromWorld();

	double difference = puck->eulerDeg.y;

	if (difference - difference != 0.0 || (double)savedY - savedY != 0.0) {
		warning("Actor::alignToPuck(): invalid yaw");
		difference = 0.0;
		savedY = 0.0f;
	}

	while (difference < savedY)
		difference += 360.0;

	difference = difference - savedY;

	while (difference >= 360.0)
		difference -= 360.0;

	double sum = difference + extra->offsetYaw;
	float alignAngle = (float)sum;

	// The disasm compares the sum before it is rounded to float
	if (!(sum >= 0.5) || alignAngle > 359.5f) {
		extra->snapTurn = false;
		extra->alignOff = 0.0f;

		return true;
	}

	double turnStep = 360.0;

	extra->alignOff = alignAngle;

	if (!extra->snapTurn) {
		float frameTime = EmiEngine::getFrameTime();

		turnStep = (double)frameTime * extra->timeScale * extra->turnRate;
	}

	bool reversed = false;

	if (alignAngle > 180.0f) {
		reversed = true;
		alignAngle = 360.0f - alignAngle;
	}

	if (!(turnStep + 0.01f >= alignAngle))
		alignAngle = (float)(((double)alignAngle - turnStep) * 0.1f + turnStep);

	double direction = reversed ? -1.0 : 1.0;

	eulerDeg.x = savedX;
	eulerDeg.z = savedZ;
	eulerDeg.y = (float)(direction * alignAngle + savedY);

	nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
	updateLocalState();

	// The disasm returns the previous value of snapTurn
	bool previousSnapTurn = extra->snapTurn;

	extra->snapTurn = false;

	return previousSnapTurn;
}

void Actor::turn(float degrees) {
	if (!extra) {
		warning("Actor::turn(): missing extra data");

		return;
	}

	if (!extra->puck)
		enablePuck(true);

	if (degrees > 0.0f)
		extra->turnDir = 1;
	else
		extra->turnDir = -1;
}

void Actor::forward() {
	if (!extra) {
		warning("Actor::forward(): missing extra data");

		return;
	}

	if (!extra->puck)
		enablePuck(true);

	extra->walking = true;
}

void Actor::enableWalkSystem(bool enable) {
	if (!extra)
		allocateActorExtra();

	if (enable) {
		extra->inBoxes = true;
		enablePuck(true);

		if (room.resource) {
			Room_v2 *lockedRoom = nullptr;

			if (room.lock())
				lockedRoom = room.typed;

			WalkPlaneCollection *collection = nullptr;

			if (lockedRoom)
				collection = lockedRoom->walkPlanes;
			else
				warning("Actor::enableWalkSystem(): the room could not be locked");

			extra->puck->validate(collection);

			if (lockedRoom)
				room.unlock();
		}
	} else {
		extra->inBoxes = false;

		if (extra->puck)
			extra->puck->plane = nullptr;
	}
}

void Actor::enablePuck(bool enable) {
	if (!extra)
		allocateActorExtra();

	if (enable) {
		if (extra->puck) {
			extra->puck->reset();
		} else {
			const float maxAngle = 3.4028235e38f;
			float target[3];

			Puck_v2 *newPuck = new Puck_v2(extra->reflectionAngle);

			newPuck->actorName = actorName;

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			newPuck->setLocalPosition(world.pos);

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			target[0] = world.rot.m[6];
			target[1] = world.rot.m[7];
			target[2] = world.rot.m[8];

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			target[0] = target[0] + world.pos.x;
			target[1] = target[1] + world.pos.y;
			target[2] = target[2] + world.pos.z;

			newPuck->pointAt(target, maxAngle);
			extra->puck = newPuck;
		}
	} else if (extra->puck) {
		delete extra->puck;
		extra->puck = nullptr;
	}
}

void Actor::setActorPosition(const float *position) {
	if (extra && extra->puck) {
		Puck_v2 *puck = extra->puck;
		CVector newPosition;

		puck->reset();

		newPosition.x = position[0];
		newPosition.y = position[1];
		newPosition.z = position[2];
		puck->setLocalPosition(newPosition);

		if (extra->inBoxes) {
			Room_v2 *lockedRoom = nullptr;

			if (room.lock())
				lockedRoom = room.typed;

			WalkPlaneCollection *collection = nullptr;

			if (lockedRoom) {
				collection = lockedRoom->walkPlanes;
				room.unlock();
			} else {
				warning("Actor::setActorPosition(): the room could not be locked");
			}

			puck->validate(collection);
		}

		if ((puck->nodeFlags & kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		setLocalPosition(puck->pos);
	} else {
		CVector newPosition;

		newPosition.x = position[0];
		newPosition.y = position[1];
		newPosition.z = position[2];
		setLocalPosition(newPosition);
	}
}

void Actor::setActorRotation(const float *rotation, bool snap) {
	mode &= ~1U;

	if (extra && extra->puck) {
		Puck_v2 *puck = extra->puck;

		puck->eulerDeg.y = rotation[1];
		puck->eulerDeg.x = 0.0f;
		puck->eulerDeg.z = 0.0f;
		puck->nodeFlags = (puck->nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
		puck->updateLocalState();

		extra->snapTurn = snap;

		if ((nodeFlags & kEulerValid) == 0)
			computeLocalMatrixFromWorld();

		extra->alignOff = rotation[1] - eulerDeg.y;

		if ((nodeFlags & kEulerValid) == 0)
			computeLocalMatrixFromWorld();

		float currentYaw = eulerDeg.y;

		eulerDeg.x = rotation[0];
		eulerDeg.y = currentYaw;
		eulerDeg.z = rotation[2];
		nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
		updateLocalState();
	} else {
		eulerDeg.x = rotation[0];
		eulerDeg.y = rotation[1];
		eulerDeg.z = rotation[2];
		nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
		updateLocalState();
	}
}

void Actor::setReflection(float angle) {
	if (!extra) {
		warning("Actor::setReflection(): missing extra data");

		return;
	}

	if (extra->puck)
		extra->puck->reflectionAngle = angle;
}

void Actor::setWalkDominate(bool dominate) {
	if (!extra) {
		warning("Actor::setWalkDominate(): missing extra data");

		return;
	}

	extra->walkDominate = dominate;
}

void Actor::leaveSector(WalkPlane_v2 *plane) {
	if (!plane)
		return;

	if (!extra) {
		warning("Actor::leaveSector(): missing extra data");

		return;
	}

	Puck_v2 *puck = extra->puck;

	if (puck) {
		plane->jumpOut(puck);

		if ((puck->nodeFlags & kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		setActorPosition(&puck->pos.x);
	}
}

void Actor::actorWalkTo(float *target, const float *goal, Actor *other) {
	// The disasm's output buffer holds only 30 points but the avoiding path can grow past that, let's give it room
	enum {
		kOutputPathCapacity = 100
	};

	float inputPath[Extra_v2::kMaxPathPoints][3];
	float outputPath[kOutputPathCapacity][3];
	float start[3];

	memset(inputPath, 0, sizeof(inputPath));
	memset(outputPath, 0, sizeof(outputPath));

	mode &= ~1U;

	if (!extra) {
		warning("Actor::actorWalkTo(): missing extra data");

		return;
	}

	if (extra->puck) {
		Room_v2 *lockedRoom = nullptr;

		if (room.lock())
			lockedRoom = room.typed;

		WalkPlaneCollection *collection = nullptr;

		if (lockedRoom) {
			collection = lockedRoom->walkPlanes;
			room.unlock();
		} else {
			warning("Actor::actorWalkTo(): the room could not be locked");
		}

		if (!collection) {
			warning("Actor::actorWalkTo(): the room has no walk planes");

			return;
		}

		Puck_v2 *puck = extra->puck;
		WalkPlane_v2 *plane = collection->planeClosest(target, 0x1000);

		if (plane)
			plane->closestPoint(target);

		if ((puck->nodeFlags & kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		start[0] = puck->pos.x;
		start[1] = puck->pos.y;
		start[2] = puck->pos.z;

		if (collection->buildPath(start, target, true)) {
			float (*pathPoints)[3];
			int pointCount = WalkPlaneCollection::getPath(&pathPoints);

			for (int i = 0; i < pointCount; i++) {
				inputPath[i][0] = pathPoints[i][0];
				inputPath[i][1] = pathPoints[i][1];
				inputPath[i][2] = pathPoints[i][2];
			}

			int newCount = buildAvoidingPath(other, &inputPath[0][0], pointCount, &outputPath[0][0]);

			if (newCount > Extra_v2::kMaxPathPoints) {
				warning("Actor::actorWalkTo(): too many path points");
				newCount = Extra_v2::kMaxPathPoints;
			}

			extra->numPathPoints = newCount;
			extra->pathIndex = 1;

			for (int i = 0; i < newCount; i++) {
				extra->path[i].x = outputPath[i][0];
				extra->path[i].y = outputPath[i][1];
				extra->path[i].z = outputPath[i][2];
			}

			extra->walkTarget = extra->path[extra->pathIndex];

			if (newCount > 1)
				mode |= 1U;
		}

		extra->goal.x = goal[0];
		extra->goal.y = goal[1];
		extra->goal.z = goal[2];
	}
}

static const float kCollisionFloatMax = 3.4028235e+38f;
static const float kCollisionMargin = 0.01f;
static const float kCollisionMinLength = 1e-20f;
static const int kCollisionAnyPlane = -1;

static double calcCollisionRadius(const RPrimitive *prim) {
	double extentX = (double)prim->box.max.x - prim->box.min.x;
	double extentZ = (double)prim->box.max.z - prim->box.min.z;

	return sqrt((extentZ * extentZ) * 0.25 + (extentX * extentX) * 0.25);
}

void Actor::getOuttaTheBox(float *point, const float *box, const float *transform) {
	double deltaX = (double)point[0] - transform[9];
	double deltaY = (double)point[1] - transform[10];
	double deltaZ = (double)point[2] - transform[11];

	float localX = (float)((deltaX * transform[0] + deltaZ * transform[2]) + deltaY * transform[1]);
	float localY = (float)((deltaZ * transform[5] + deltaY * transform[4]) + deltaX * transform[3]);
	float localZ = (float)((deltaX * transform[6] + deltaZ * transform[8]) + deltaY * transform[7]);

	if (localX <= box[3] && localX >= box[0] && localY <= box[4] && localY >= box[1] && localZ <= box[5] && localZ >= box[2]) {
		float outX = localX;
		float outY = localY;
		float outZ = localZ;
		double best = kCollisionFloatMax;

		if (box[5] > localZ) {
			double distance = (double)box[5] - localZ;

			if (distance < 0.0)
				distance = -distance;

			if (distance < best) {
				outX = localX;
				outY = localY;
				outZ = (float)((double)box[5] + kCollisionMargin);
				best = (float)distance;
			}
		}

		if (box[2] < localZ) {
			double distance = (double)box[2] - localZ;

			if (distance < 0.0)
				distance = -distance;

			if (distance < best) {
				outX = localX;
				outY = localY;
				outZ = (float)((double)box[2] - kCollisionMargin);
				best = (float)distance;
			}
		}

		if (box[3] > localX) {
			double distance = (double)box[3] - localX;

			if (distance < 0.0)
				distance = -distance;

			if (distance < best) {
				outX = (float)((double)box[3] + kCollisionMargin);
				outY = localY;
				outZ = localZ;
				best = (float)distance;
			}
		}

		if (box[0] < localX) {
			double distance = (double)box[0] - localX;

			if (distance < 0.0)
				distance = -distance;

			if (distance < best) {
				outX = (float)((double)box[0] - kCollisionMargin);
				outY = localY;
				outZ = localZ;
			}
		}

		double worldX = ((double)outZ * transform[6] + (double)outX * transform[0]) + (double)outY * transform[3];
		float worldY = (float)(((double)outY * transform[4] + (double)outZ * transform[7]) + (double)outX * transform[1]);
		float worldZ = (float)(((double)outY * transform[5] + (double)outZ * transform[8]) + (double)outX * transform[2]);

		point[0] = (float)(worldX + transform[9]);
		point[1] = (float)((double)worldY + transform[10]);
		point[2] = (float)((double)worldZ + transform[11]);
	}
}

bool Actor::testCollision(float startX, float startY, float startZ, float endX, float endY, float endZ, float originX, float originY, float originZ, const float *box, const float *transform) {
	float best = 1.0f;

	double startDeltaX = ((double)startX - originX) - transform[9];
	float startRelativeY = (float)((double)startY - originY);
	float startRelativeZ = (float)((double)startZ - originZ);
	double startDeltaY = (double)startRelativeY - transform[10];
	double startDeltaZ = (double)startRelativeZ - transform[11];

	float localStartX = (float)((startDeltaZ * transform[2] + startDeltaY * transform[1]) + startDeltaX * transform[0]);
	float localStartY = (float)((startDeltaZ * transform[5] + startDeltaY * transform[4]) + startDeltaX * transform[3]);
	float localStartZ = (float)((startDeltaX * transform[6] + startDeltaZ * transform[8]) + startDeltaY * transform[7]);

	double endDeltaX = ((double)endX - originX) - transform[9];
	float endRelativeY = (float)((double)endY - originY);
	float endRelativeZ = (float)((double)endZ - originZ);
	double endDeltaY = (double)endRelativeY - transform[10];
	double endDeltaZ = (double)endRelativeZ - transform[11];

	float localEndX = (float)((endDeltaZ * transform[2] + endDeltaY * transform[1]) + endDeltaX * transform[0]);
	float localEndY = (float)((endDeltaZ * transform[5] + endDeltaY * transform[4]) + endDeltaX * transform[3]);
	float localEndZ = (float)((endDeltaX * transform[6] + endDeltaZ * transform[8]) + endDeltaY * transform[7]);

	float directionX = (float)((double)localEndX - localStartX);
	float directionY = (float)((double)localEndY - localStartY);
	float directionZ = (float)((double)localEndZ - localStartZ);

	if (localStartZ > box[5] && localEndZ < box[5]) {
		double fraction = ((double)box[5] - localStartZ) / directionZ;

		if (fraction < 1.0) {
			float fractionFloat = (float)fraction;
			float hitX = (float)((double)directionX * fractionFloat + localStartX);
			float hitDeltaY = (float)((double)directionY * fractionFloat);
			float hitDeltaZ = (float)((double)directionZ * fractionFloat);
			float hitY = (float)((double)hitDeltaY + localStartY);
			float hitZ = (float)((double)hitDeltaZ + localStartZ);

			if (hitX > box[0] && hitX < box[3]) {
				_hitPoint.x = hitX;
				_hitPoint.y = hitY;
				_hitPoint.z = hitZ;
				best = fractionFloat;
			}
		}
	}

	if (localStartZ < box[2] && localEndZ > box[2]) {
		double fraction = ((double)box[2] - localStartZ) / directionZ;

		if (fraction < best) {
			float fractionFloat = (float)fraction;
			float hitX = (float)((double)directionX * fractionFloat + localStartX);
			float hitDeltaY = (float)((double)directionY * fractionFloat);
			float hitDeltaZ = (float)((double)directionZ * fractionFloat);
			float hitY = (float)((double)hitDeltaY + localStartY);
			float hitZ = (float)((double)hitDeltaZ + localStartZ);

			if (hitX > box[0] && hitX < box[3]) {
				_hitPoint.x = hitX;
				_hitPoint.y = hitY;
				_hitPoint.z = hitZ;
				best = fractionFloat;
			}
		}
	}

	if (localStartX > box[3] && localEndX < box[3]) {
		double fraction = ((double)box[3] - localStartX) / directionX;

		if (fraction < best) {
			float fractionFloat = (float)fraction;
			float hitDeltaY = (float)((double)directionY * fractionFloat);
			float hitDeltaZ = (float)((double)directionZ * fractionFloat);
			float hitX = (float)((double)directionX * fractionFloat + localStartX);
			float hitY = (float)((double)hitDeltaY + localStartY);
			double hitZExact = (double)hitDeltaZ + localStartZ;
			float hitZ = (float)hitZExact;

			if (hitZExact > box[2] && hitZ < box[5]) {
				_hitPoint.x = hitX;
				_hitPoint.y = hitY;
				_hitPoint.z = hitZ;
				best = fractionFloat;
			}
		}
	}

	if (localStartX < box[0] && localEndX > box[0]) {
		double fraction = ((double)box[0] - localStartX) / directionX;

		if (fraction < best) {
			float fractionFloat = (float)fraction;
			float hitDeltaY = (float)((double)directionY * fractionFloat);
			float hitDeltaZ = (float)((double)directionZ * fractionFloat);
			float hitX = (float)((double)directionX * fractionFloat + localStartX);
			float hitY = (float)((double)hitDeltaY + localStartY);
			double hitZExact = (double)hitDeltaZ + localStartZ;
			float hitZ = (float)hitZExact;

			if (hitZExact > box[2] && hitZ < box[5]) {
				_hitPoint.x = hitX;
				_hitPoint.y = hitY;
				_hitPoint.z = hitZ;
				best = fractionFloat;
			}
		}
	}

	return best != 1.0f;
}

int Actor::findClosestCandidate(float startX, float startY, float startZ, float goalX, float goalY, float goalZ, const ObjectCorners_v2 *corners) {
	int found = -1;
	float best = kCollisionFloatMax;

	for (int i = 0; i < ObjectCorners_v2::kNumCorners; i++) {
		if (!corners->valid[i])
			continue;

		double startDeltaZ = (double)startZ - corners->corner[i][2];
		double startDeltaY = (double)startY - corners->corner[i][1];
		double startDeltaX = (double)startX - corners->corner[i][0];
		double goalDeltaZ = (double)corners->corner[i][2] - goalZ;
		double goalDeltaY = (double)corners->corner[i][1] - goalY;
		float goalDeltaX = (float)((double)corners->corner[i][0] - goalX);

		float total = (float)(((startDeltaX * startDeltaX + startDeltaZ * startDeltaZ) + startDeltaY * startDeltaY)
			+ (((double)goalDeltaX * goalDeltaX + goalDeltaZ * goalDeltaZ) + goalDeltaY * goalDeltaY));

		if (total < best) {
			found = i;
			best = total;
		}
	}

	return found;
}

int Actor::buildAvoidingPath(Actor *other, float *path, int count, float *outPath) {
	int outCount = 0;
	float startX = 0.0f;
	float startY = 0.0f;
	float startZ = 0.0f;
	float endX = 0.0f;
	float endY = 0.0f;
	float endZ = 0.0f;
	float goal[3];

	goal[0] = 0.0f;
	goal[1] = 0.0f;
	goal[2] = 0.0f;

	if (!other || !other->extra || !extra) {
		if (other && other->extra)
			warning("Actor::buildAvoidingPath(): the actor has no extra data");

		for (int i = 0; i < count; i++) {
			outPath[3 * i] = path[3 * i];
			outPath[3 * i + 1] = path[3 * i + 1];
			outPath[3 * i + 2] = path[3 * i + 2];
			outCount++;
		}

		return outCount;
	}

	if (count <= 0) {
		warning("Actor::buildAvoidingPath(): the path has no points");

		return 0;
	}

	double thisRadius = 0.5;

	if (!instances.empty() && instances[0]) {
		if (instances[0]->prim)
			thisRadius = calcCollisionRadius(instances[0]->prim);
		else
			warning("Actor::buildAvoidingPath(): primitive instance without primitive");
	}

	thisRadius = thisRadius * extra->collisionScale;

	double otherRadius = 0.5;

	if (!other->instances.empty() && other->instances[0]) {
		if (other->instances[0]->prim)
			otherRadius = calcCollisionRadius(other->instances[0]->prim);
		else
			warning("Actor::buildAvoidingPath(): primitive instance without primitive");
	}

	double radius = thisRadius + otherRadius * other->extra->collisionScale;
	float radiusFloat = (float)radius;
	float negativeRadius = (float)-radius;
	float box[6];

	box[0] = negativeRadius;
	box[1] = negativeRadius;
	box[2] = negativeRadius;
	box[3] = radiusFloat;
	box[4] = radiusFloat;
	box[5] = radiusFloat;

	if ((other->nodeFlags & kPositionValid) == 0)
		other->computeLocalPositionFromWorld();

	if ((other->nodeFlags & kWorldValid) == 0)
		other->updateWorldTransform();

	float *lastPoint = &path[3 * count - 3];

	getOuttaTheBox(lastPoint, box, other->world.rot.m);

	if ((other->nodeFlags & kPositionValid) == 0)
		other->computeLocalPositionFromWorld();

	if ((other->nodeFlags & kWorldValid) == 0)
		other->updateWorldTransform();

	getOuttaTheBox(path, box, other->world.rot.m);

	goal[0] = lastPoint[0];
	goal[1] = lastPoint[1];
	goal[2] = lastPoint[2];

	bool locked = room.lock();
	Room_v2 *currentRoom = locked ? room.typed : nullptr;
	WalkPlaneCollection *planes = nullptr;

	if (currentRoom)
		planes = currentRoom->walkPlanes;
	else
		warning("Actor::buildAvoidingPath(): the room could not be locked");

	if (locked)
		room.unlock();

	if (!planes) {
		warning("Actor::buildAvoidingPath(): the room has no walk planes");

		return 0;
	}

	if (!planes->planeAt(goal, kCollisionAnyPlane))
		return 0;

	int index = 0;
	bool collided = false;

	for (index = 0; index < count - 1; index++) {
		startX = path[3 * index];
		startY = path[3 * index + 1];
		startZ = path[3 * index + 2];
		endX = path[3 * index + 3];
		endY = path[3 * index + 4];
		endZ = path[3 * index + 5];

		outPath[3 * outCount] = path[3 * index];
		outPath[3 * outCount + 1] = path[3 * index + 1];
		outPath[3 * outCount + 2] = path[3 * index + 2];
		outCount++;

		if ((other->nodeFlags & kWorldValid) == 0)
			other->updateWorldTransform();

		if (testCollision(startX, startY, startZ, endX, endY, endZ, 0.0f, 0.0f, 0.0f, box, other->world.rot.m)) {
			collided = true;

			break;
		}
	}

	if (!collided) {
		outPath[3 * outCount] = path[3 * index];
		outPath[3 * outCount + 1] = path[3 * index + 1];
		outPath[3 * outCount + 2] = path[3 * index + 2];

		return outCount + 1;
	}

	ObjectCorners_v2 corners;

	for (int i = 0; i < ObjectCorners_v2::kNumCorners; i++) {
		corners.corner[i][0] = 0.0f;
		corners.corner[i][1] = 0.0f;
		corners.corner[i][2] = 0.0f;
	}

	float hit[3];

	hit[0] = _hitPoint.x;
	hit[1] = _hitPoint.y;
	hit[2] = _hitPoint.z;

	if ((other->nodeFlags & kWorldValid) == 0)
		other->updateWorldTransform();

	const float *matrix = other->world.rot.m;
	double hitWorldX = ((double)hit[1] * matrix[3] + (double)hit[2] * matrix[6]) + (double)hit[0] * matrix[0];
	float hitOffsetY = (float)(((double)hit[1] * matrix[4] + (double)hit[2] * matrix[7]) + (double)hit[0] * matrix[1]);
	float hitOffsetZ = (float)(((double)hit[1] * matrix[5] + (double)hit[2] * matrix[8]) + (double)hit[0] * matrix[2]);

	hit[0] = (float)(hitWorldX + other->world.pos.x);
	hit[1] = (float)((double)hitOffsetY + other->world.pos.y);
	hit[2] = (float)((double)hitOffsetZ + other->world.pos.z);

	WalkPlane_v2 *closestPlane = planes->planeClosest(hit, kCollisionAnyPlane);

	if (closestPlane)
		closestPlane->project(hit);
	else
		warning("Actor::buildAvoidingPath(): no walk plane near the hit point");

	corners.corner[0][0] = negativeRadius;
	corners.corner[0][1] = 0.0f;
	corners.corner[0][2] = negativeRadius;
	corners.corner[1][0] = radiusFloat;
	corners.corner[1][1] = 0.0f;
	corners.corner[1][2] = negativeRadius;
	corners.corner[2][0] = radiusFloat;
	corners.corner[2][1] = 0.0f;
	corners.corner[2][2] = radiusFloat;
	corners.corner[3][0] = negativeRadius;
	corners.corner[3][1] = 0.0f;
	corners.corner[3][2] = radiusFloat;

	for (int k = 0; k < ObjectCorners_v2::kNumCorners; k++) {
		float *corner = corners.corner[k];

		if ((other->nodeFlags & kWorldValid) == 0)
			other->updateWorldTransform();

		matrix = other->world.rot.m;

		double cornerWorldX = ((double)matrix[6] * corner[2] + (double)matrix[3] * corner[1]) + (double)corner[0] * matrix[0];
		float cornerOffsetZ = (float)(((double)matrix[8] * corner[2] + (double)matrix[5] * corner[1]) + (double)matrix[2] * corner[0]);

		corner[0] = (float)(cornerWorldX + other->world.pos.x);
		corner[1] = hit[1];
		corner[2] = (float)((double)cornerOffsetZ + other->world.pos.z);

		corners.valid[k] = 0;
		endX = corner[0];
		endY = corner[1];
		endZ = corner[2];

		if ((other->nodeFlags & kWorldValid) == 0)
			other->updateWorldTransform();

		if (!testCollision(startX, startY, startZ, endX, endY, endZ, 0.0f, 0.0f, 0.0f, box, other->world.rot.m)
			&& planes->buildPath(&path[3 * index], corner, false))
			corners.valid[k] = 1;
	}

	int closest = findClosestCandidate(startX, startY, startZ, goal[0], goal[1], goal[2], &corners);

	if (closest == -1 || !planes->buildPath(&path[3 * index], corners.corner[closest], true))
		return outCount;

	float (*pathPoints)[3];
	int pathCount = WalkPlaneCollection::getPath(&pathPoints);

	if (pathCount > 1) {
		for (int j = 1; j < pathCount; j++) {
			outPath[3 * outCount] = pathPoints[j][0];
			outPath[3 * outCount + 1] = pathPoints[j][1];
			outPath[3 * outCount + 2] = pathPoints[j][2];
			outCount++;
		}
	}

	corners.valid[closest] = 0;

	if ((other->nodeFlags & kWorldValid) == 0)
		other->updateWorldTransform();

	float *lastOutPoint = &outPath[3 * outCount - 3];
	float *nextOutPoint = &outPath[3 * outCount];

	if (!testCollision(lastOutPoint[0], lastOutPoint[1], lastOutPoint[2], goal[0], goal[1], goal[2], 0.0f, 0.0f, 0.0f, box, other->world.rot.m)
		&& planes->buildPath(lastOutPoint, goal, true)) {
		pathCount = WalkPlaneCollection::getPath(&pathPoints);

		if (pathCount > 1) {
			for (int j = 1; j < pathCount; j++) {
				outPath[3 * outCount] = pathPoints[j][0];
				outPath[3 * outCount + 1] = pathPoints[j][1];
				outPath[3 * outCount + 2] = pathPoints[j][2];
				outCount++;
			}

			return outCount;
		}

		return outCount;
	}

	int previousCorner = (closest + 3) % 4;
	int nextCorner = (closest + 5) % 4;
	const float *previousPoint = corners.corner[previousCorner];
	bool previousReachable = planes->buildPath(lastOutPoint, previousPoint, false);
	const float *nextPoint = corners.corner[nextCorner];
	bool nextReachable = planes->buildPath(lastOutPoint, nextPoint, false);
	int current = nextCorner;
	int direction = 1;

	if (nextReachable) {
		if (previousReachable) {
			double previousToLastZ = (double)lastOutPoint[2] - previousPoint[2];
			double previousToLastY = (double)lastOutPoint[1] - previousPoint[1];
			double previousToLastX = (double)lastOutPoint[0] - previousPoint[0];
			double previousToGoalZ = (double)previousPoint[2] - goal[2];
			float previousToGoalY = (float)((double)previousPoint[1] - goal[1]);
			float previousToGoalX = (float)((double)previousPoint[0] - goal[0]);
			float nextToLastZ = (float)((double)lastOutPoint[2] - nextPoint[2]);
			float nextToLastY = (float)((double)lastOutPoint[1] - nextPoint[1]);
			float nextToLastX = (float)((double)lastOutPoint[0] - nextPoint[0]);
			float nextToGoalZ = (float)((double)nextPoint[2] - goal[2]);
			float nextToGoalY = (float)((double)nextPoint[1] - goal[1]);
			float nextToGoalX = (float)((double)nextPoint[0] - goal[0]);

			double previousDistance = ((previousToLastX * previousToLastX + previousToLastY * previousToLastY) + previousToLastZ * previousToLastZ)
				+ (((double)previousToGoalX * previousToGoalX + (double)previousToGoalY * previousToGoalY) + previousToGoalZ * previousToGoalZ);
			double nextDistance = (((double)nextToGoalX * nextToGoalX + (double)nextToGoalY * nextToGoalY) + (double)nextToGoalZ * nextToGoalZ)
				+ (((double)nextToLastX * nextToLastX + (double)nextToLastY * nextToLastY) + (double)nextToLastZ * nextToLastZ);

			if (nextDistance > previousDistance) {
				current = previousCorner;
				direction = -1;
			}
		}
	} else if (!previousReachable) {
		return outCount;
	} else {
		current = previousCorner;
		direction = -1;
	}

	for (;;) {
		if ((other->nodeFlags & kWorldValid) == 0)
			other->updateWorldTransform();

		bool blocked = testCollision(nextOutPoint[-3], nextOutPoint[-2], nextOutPoint[-1], goal[0], goal[1], goal[2], 0.0f, 0.0f, 0.0f, box, other->world.rot.m);

		if (!blocked || current == closest)
			break;

		outCount++;
		nextOutPoint[0] = corners.corner[current][0];
		nextOutPoint[1] = corners.corner[current][1];
		nextOutPoint[2] = corners.corner[current][2];
		nextOutPoint += 3;
		current = (current + direction + 4) % 4;
	}

	outPath[3 * outCount] = goal[0];
	outPath[3 * outCount + 1] = goal[1];
	outPath[3 * outCount + 2] = goal[2];

	return outCount + 1;
}

bool Actor::resolveCollisions() {
	if (instances.empty())
		return true;

	RPrimitiveInstance *instance = instances[0];

	if (!instance) {
		warning("Actor::resolveCollisions(): missing primitive instance");

		return true;
	}

	RPrimitive *prim = instance->prim;

	if (!prim)
		return true;

	if (!extra) {
		warning("Actor::resolveCollisions(): the actor has no extra data");

		return true;
	}

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *matrix = world.rot.m;
	float offsetX = (float)(((double)matrix[3] * prim->center.y + (double)matrix[6] * prim->center.z) + (double)matrix[0] * prim->center.x);
	float offsetY = (float)(((double)matrix[4] * prim->center.y + (double)matrix[7] * prim->center.z) + (double)matrix[1] * prim->center.x);
	float offsetZ = (float)(((double)matrix[5] * prim->center.y + (double)matrix[8] * prim->center.z) + (double)matrix[2] * prim->center.x);

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	float centerX = (float)((double)offsetX + world.pos.x);
	float centerY = (float)((double)offsetY + world.pos.y);
	float centerZ = (float)((double)offsetZ + world.pos.z);
	float radius = (float)(calcCollisionRadius(prim) * extra->collisionScale);

	// The disasm handles the collision modes 1 and 2 of this actor in the same way
	if (extra->collisionMode == 0)
		return true;

	for (int i = 0; i < _collisionCount; i++) {
		Actor *other = _collisionActors[i];

		if (!other || other == this)
			continue;

		Extra_v2 *otherExtra = other->extra;

		if (!otherExtra)
			continue;

		if (otherExtra->collisionMode == 0)
			continue;

		if (other->instances.empty())
			continue;

		RPrimitiveInstance *otherInstance = other->instances[0];

		if (!otherInstance) {
			warning("Actor::resolveCollisions(): missing primitive instance");

			continue;
		}

		RPrimitive *otherPrim = otherInstance->prim;

		if (!otherPrim)
			continue;

		if ((other->nodeFlags & kWorldValid) == 0)
			other->updateWorldTransform();

		const float *otherMatrix = other->world.rot.m;
		float otherOffsetX = (float)(((double)otherMatrix[6] * otherPrim->center.z + (double)otherMatrix[3] * otherPrim->center.y) + (double)otherPrim->center.x * otherMatrix[0]);
		float otherOffsetY = (float)(((double)otherMatrix[7] * otherPrim->center.z + (double)otherMatrix[1] * otherPrim->center.x) + (double)otherMatrix[4] * otherPrim->center.y);
		float otherOffsetZ = (float)(((double)otherMatrix[8] * otherPrim->center.z + (double)otherMatrix[2] * otherPrim->center.x) + (double)otherMatrix[5] * otherPrim->center.y);

		if ((other->nodeFlags & kWorldValid) == 0)
			other->updateWorldTransform();

		double otherCenterX = (double)otherOffsetX + other->world.pos.x;
		double otherCenterY = (double)otherOffsetY + other->world.pos.y;
		double otherCenterZ = (double)otherOffsetZ + other->world.pos.z;
		float otherRadius = (float)(calcCollisionRadius(otherPrim) * otherExtra->collisionScale);

		float deltaZ = (float)((double)centerZ - otherCenterZ);
		float deltaY = (float)((double)centerY - otherCenterY);
		float deltaX = (float)((double)centerX - otherCenterX);
		float distance = (float)sqrt(((double)deltaX * deltaX + (double)deltaY * deltaY) + (double)deltaZ * deltaZ);

		if (!((double)otherRadius + radius > distance))
			continue;

		if (otherExtra->collisionMode != 2)
			continue;

		if (radius == 0.0f) {
			// The original could divide by zero here, let's avoid that...
			warning("Actor::resolveCollisions(): the collision radius is zero");

			continue;
		}

		extra->inCollision = true;
		otherExtra->inCollision = true;

		if ((nodeFlags & kPositionValid) == 0)
			computeLocalPositionFromWorld();

		_collisionOldPosition.x = pos.x;
		_collisionOldPosition.y = pos.y;
		_collisionOldPosition.z = pos.z;

		double directionLength = sqrt(((double)deltaX * deltaX + (double)deltaZ * deltaZ) + (double)deltaY * deltaY);
		double directionX = deltaX;
		float directionY = deltaY;
		float directionZ = deltaZ;

		if (directionLength >= kCollisionMinLength) {
			float inverseLength = (float)(1.0 / (float)directionLength);

			directionX = (double)deltaX * inverseLength;
			directionY = (float)((double)deltaY * inverseLength);
			directionZ = (float)((double)deltaZ * inverseLength);
		}

		double push = (1.0 - ((double)distance - otherRadius) / radius) * radius;
		float pushFloat = (float)push;
		float pushY = (float)((double)directionY * pushFloat);
		float pushZ = (float)((double)directionZ * pushFloat);
		double pushedX = directionX * push + centerX;
		float pushedY = (float)((double)pushY + centerY);
		float pushedZ = (float)((double)pushZ + centerZ);
		float newPosition[3];

		newPosition[0] = (float)(pushedX - offsetX);
		newPosition[1] = (float)((double)pushedY - offsetY);
		newPosition[2] = (float)((double)pushedZ - offsetZ);

		setActorPosition(newPosition);

		if ((nodeFlags & kPositionValid) == 0)
			computeLocalPositionFromWorld();

		_collisionNewPosition.x = pos.x;
		_collisionNewPosition.y = pos.y;
		_collisionNewPosition.z = pos.z;
		_hitPoint.x = _collisionNewPosition.x;
		_hitPoint.y = _collisionNewPosition.y;
		_hitPoint.z = _collisionNewPosition.z;

		Lua_V2::callCollisionHandler(this, other, _collisionOldPosition.x, _collisionOldPosition.y, _collisionOldPosition.z,
			_hitPoint.x, _hitPoint.y, _hitPoint.z, _collisionNewPosition.x, _collisionNewPosition.y, _collisionNewPosition.z);
	}

	if ((nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	prevPos.x = pos.x;
	prevPos.y = pos.y;
	prevPos.z = pos.z;

	return true;
}

static ProfilerTimer actorCalcOnScreenTimer("Actor::CalcOnScreen()", ProfilerTimer::kTrackNesting);
static ProfilerTimer actorDrawTimer("Actor::Draw()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);
static ProfilerTimer actorDrawShadowTimer("Actor::DrawShadow()", ProfilerTimer::kTrackNesting);

static const int kSortOrderSetups = 32;
static const int kSortWalkPlaneMask = 0x1000;
static const int kShadowTriangleFlags = 0x301;
static const int kShadowVertexCapacity = 2000;
static const float kMinClipLength = 1e-20f;

static int truncateScreenValue(double value) {
	if (!(value > -2147483649.0 && value < 2147483648.0)) {
		warning("Actor::calcOnScreen(): the screen rectangle is out of range");

		return -2147483647 - 1;
	}

	return (int)value;
}

int Actor::getSortOrder(int setupIndex) {
	Actor *sortActor = this;

	if (parentActorId) {
		do {
			if (sortActor->sortOrder != -2)
				break;

			Actor *parentActor = actorDeref(sortActor->parentActorId);

			if (!parentActor)
				break;

			sortActor = parentActor;
		} while (sortActor->parentActorId);
	}

	Extra_v2 *sortExtra = sortActor->extra;

	if (!sortExtra || !sortExtra->inBoxes || !sortActor->room.resource)
		return sortActor->sortOrder;

	WalkPlane_v2 *plane = nullptr;

	if (sortExtra->puck) {
		plane = sortExtra->puck->plane;
	} else {
		bool locked = sortActor->room.lock();
		Room_v2 *currentRoom = locked ? sortActor->room.typed : nullptr;
		WalkPlaneCollection *collection = nullptr;

		if (currentRoom)
			collection = currentRoom->walkPlanes;
		else
			warning("Actor::getSortOrder(): the room could not be locked");

		if (locked)
			sortActor->room.unlock();

		if ((sortActor->nodeFlags & kWorldValid) == 0)
			sortActor->updateWorldTransform();

		if (collection)
			plane = collection->planeAt(&sortActor->world.pos.x, kSortWalkPlaneMask);
		else
			warning("Actor::getSortOrder(): the room has no walk planes");
	}

	if (!plane)
		return sortActor->sortOrder;

	if (setupIndex < 0) {
		warning("Actor::getSortOrder(): negative setup index");

		return sortActor->sortOrder;
	}

	return plane->sortOrder[setupIndex % kSortOrderSetups];
}

bool Actor::calcOnScreen(Camera *camera) {
	actorCalcOnScreenTimer.start();

	CVector point;

	camera->applyProjection();
	camera->updateViewMatrix();

	CTransform viewTransform = camera->view;
	IRect viewport = camera->getViewportRect();
	int viewLeft = viewport.x;
	int viewTop = viewport.y;
	int viewRight = viewport.x + viewport.w;
	int viewBottom = viewport.y + viewport.h;

	float minX = (float)((double)viewRight + 1.0);
	float minY = (float)((double)viewBottom + 1.0);
	float maxX = (float)((double)viewLeft - 1.0);
	float maxY = (float)((double)viewTop - 1.0);

	const float *m = viewTransform.rot.m;

	for (uint i = 0; i < instances.size(); i++) {
		RPrimitiveInstance *instance = instances[i];

		if (!instance)
			continue;

		if (!instance->prim) {
			warning("Actor::calcOnScreen(): primitive instance without primitive");

			continue;
		}

		const BBox &box = instance->prim->box;

		for (int corner = 0; corner < 8; corner++) {
			point.x = (corner & 4) ? box.max.x : box.min.x;
			point.y = (corner & 2) ? box.max.y : box.min.y;
			point.z = (corner & 1) ? box.max.z : box.min.z;

			localToWorld(&point);

			float rowY = (float)(((double)m[1] * point.x + (double)m[4] * point.y) + (double)m[7] * point.z);
			float rowZ = (float)(((double)m[2] * point.x + (double)m[5] * point.y) + (double)m[8] * point.z);
			double viewX = (((double)m[3] * point.y + (double)m[6] * point.z) + (double)m[0] * point.x) + viewTransform.pos.x;
			float viewY = (float)((double)viewTransform.pos.y + rowY);
			double viewZ = (double)viewTransform.pos.z + rowZ;

			if (viewZ >= 0.0) {
				float inverseZ = (float)(1.0 / viewZ);
				float screenX = (float)(((viewX * camera->focalX) * inverseZ) + camera->halfWidth);
				float screenY = (float)(camera->halfHeight - (((double)viewY * camera->focalY) * inverseZ));

				// The disasm updates the minimum also when the comparison is unordered
				if (!(screenX >= minX))
					minX = screenX;

				if (screenX > maxX)
					maxX = screenX;

				if (!(screenY >= minY))
					minY = screenY;

				if (screenY > maxY)
					maxY = screenY;
			}
		}
	}

	drawnRect[0] = truncateScreenValue(minX);
	drawnRect[1] = truncateScreenValue(minY);
	drawnRect[2] = truncateScreenValue((double)maxX - minX);
	drawnRect[3] = truncateScreenValue((double)maxY - minY);

	bool visible = true;

	if ((int)((uint)drawnRect[2] + (uint)drawnRect[0]) < viewLeft
		|| drawnRect[0] > viewRight
		|| (int)((uint)drawnRect[3] + (uint)drawnRect[1]) < viewTop
		|| drawnRect[1] > viewBottom)
		visible = false;

	actorCalcOnScreenTimer.stop();

	return visible;
}

int Actor::drawActor(Camera *camera) {
	actorDrawTimer.start();

	if (onScreen)
		draw(camera);

	actorDrawTimer.stop();

	return 0;
}

void Actor::activateShadow(bool active, Shadow_v2 *shadow) {
	if (active) {
		if (shadow) {
			if (findShadowIndex(shadow) < 0)
				shadows.push_back(shadow);
		} else {
			shadows.clear();

			bool locked = room.lock();
			Room_v2 *currentRoom = locked ? room.typed : nullptr;

			if (currentRoom) {
				Common::List<Shadow_v2 *>::iterator it;

				for (it = currentRoom->shadows.begin(); it != currentRoom->shadows.end(); ++it) {
					if (*it)
						activateShadow(active, *it);
				}
			} else {
				warning("Actor::activateShadow(): the room could not be locked");
			}

			room.unlock();
		}
	} else if (shadow) {
		int index = findShadowIndex(shadow);

		if (index >= 0)
			shadows.remove_at(index);
	} else {
		shadows.clear();
	}
}

int Actor::findShadowIndex(Shadow_v2 *shadow) {
	for (uint i = 0; i < shadows.size(); i++) {
		if (shadows[i] == shadow)
			return i;
	}

	return -1;
}

int Actor::drawShadow(Camera *camera, Shadow_v2 *shadow) {
	actorDrawShadowTimer.start();

	CDrawContext *drawContext = CDrawContext::context;

	if (!onScreen) {
		actorDrawShadowTimer.stop();

		return 0;
	}

	if (!drawContext || !_shadowVertices) {
		warning("Actor::drawShadow(): no draw context or shadow vertices");
		actorDrawShadowTimer.stop();

		return 0;
	}

	drawContext->setCameraMatrix(reinterpret_cast<const float *>(&camera->view));

	float shadowPoint[3];
	float *position = shadow->getPosition(shadowPoint);
	float shadowX = position[0];
	float shadowY = position[1];
	float shadowZ = position[2];

	Color shadowColor(shadow->color);

	Common::Array<RPrimitiveInstance *> *meshInstances = &instances;

	if (shadowMeshes.size() > 0)
		meshInstances = &shadowMeshes;

	if (shadow->sectors.empty() || !shadow->sectors[0]) {
		warning("Actor::drawShadow(): shadow without sectors");
		drawContext->resetCameraMatrix();
		actorDrawShadowTimer.stop();

		return 0;
	}

	WalkPlane_v2 *firstSector = shadow->sectors[0];
	float planeNormalX = firstSector->normal[0];
	float planeNormalY = firstSector->normal[1];
	float planeD = firstSector->d;
	float planeNormalZ = firstSector->normal[2];

	for (uint i = 0; i < meshInstances->size(); i++) {
		RPrimitiveInstance *instance = (*meshInstances)[i];

		if (!instance || !instance->prim) {
			warning("Actor::drawShadow(): primitive instance without primitive");

			continue;
		}

		RMesh *mesh = dynamic_cast<RMesh *>(instance->prim);

		if (!mesh)
			continue;

		instance->update(this, camera);

		RMeshInstance *meshInstance = static_cast<RMeshInstance *>((*meshInstances)[i]);

		if (!meshInstance->vertexArray) {
			warning("Actor::drawShadow(): mesh instance without vertex array");

			continue;
		}

		if (meshInstance->vertexArray->numVerts > kShadowVertexCapacity) {
			warning("Actor::drawShadow(): too many vertices for the shadow vertex array");

			continue;
		}

		_shadowVertices->numVerts = meshInstance->vertexArray->numVerts;

		CDCVertex *destination = static_cast<CDCVertex *>(_shadowVertices->lock());
		CDCVertex *source = static_cast<CDCVertex *>(meshInstance->vertexArray->lock());

		if (!destination || !source) {
			warning("Actor::drawShadow(): could not lock the vertex arrays");

			continue;
		}

		for (int v = 0; v < _shadowVertices->numVerts; v++) {
			CVector localPoint;
			CVector worldPoint;

			localPoint.x = source[v].x;
			localPoint.y = source[v].y;
			localPoint.z = source[v].z;

			localToWorld(&localPoint, &worldPoint);

			destination[v].x = worldPoint.x;
			destination[v].y = worldPoint.y;
			destination[v].z = worldPoint.z;

			double deltaX = (double)destination[v].x - shadowX;
			float deltaY = (float)((double)destination[v].y - shadowY);
			float deltaZ = (float)((double)destination[v].z - shadowZ);

			double numerator = (((double)planeNormalY * destination[v].y + (double)planeNormalZ * destination[v].z) + (double)planeNormalX * destination[v].x) + planeD;
			double denominator = ((double)planeNormalZ * deltaZ + (double)planeNormalY * deltaY) + deltaX * planeNormalX;
			float projectionScale = (float)(-(numerator / denominator));

			float offsetY = (float)((double)deltaY * projectionScale);
			float offsetZ = (float)((double)deltaZ * projectionScale);

			destination[v].x = (float)(deltaX * projectionScale + destination[v].x);
			destination[v].y = (float)(offsetY + destination[v].y);
			destination[v].z = (float)(offsetZ + destination[v].z);

			uint red = (uint)shadowColor.r <= 0xFFu ? (uint)shadowColor.r : 0xFFu;
			uint green = (uint)shadowColor.g <= 0xFFu ? (uint)shadowColor.g : 0xFFu;
			uint blue = (uint)shadowColor.b <= 0xFFu ? (uint)shadowColor.b : 0xFFu;
			uint alpha = (uint)shadowColor.a <= 0xFFu ? (uint)shadowColor.a : 0xFFu;

			destination[v].color = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}

		meshInstance->vertexArray->unlock();
		_shadowVertices->unlock();

		for (uint s = 0; s < shadow->sectors.size(); s++) {
			WalkPlane_v2 *sector = shadow->sectors[s];

			if (!sector) {
				warning("Actor::drawShadow(): unresolved shadow sector");

				continue;
			}

			if (sector->numVerts > drawContext->getMaxClipPlanes())
				continue;

			int numVerts = sector->numVerts;

			for (int e = 0; e < numVerts; e++) {
				wpVertex *start = sector->verts[e];
				wpVertex *end = sector->verts[(e + 1) % numVerts];

				double toShadowX = (double)shadowX - start->x;
				double toShadowY = (double)shadowY - start->y;
				double toShadowZ = (double)shadowZ - start->z;

				float edgeX = (float)((double)end->x - start->x);
				float edgeY = (float)((double)end->y - start->y);
				double edgeZ = (double)end->z - start->z;

				float clipX = (float)(edgeY * toShadowZ - edgeZ * toShadowY);
				float clipY = (float)(edgeZ * toShadowX - toShadowZ * edgeX);
				float clipZ = (float)(toShadowY * edgeX - edgeY * toShadowX);

				double length = sqrt(((double)clipX * clipX + (double)clipZ * clipZ) + (double)clipY * clipY);

				if (length >= kMinClipLength) {
					double inverse = 1.0 / length;

					clipX = (float)(clipX * inverse);
					clipY = (float)(inverse * clipY);
					clipZ = (float)(inverse * clipZ);
				}

				float clipD = (float)(-(((double)shadowY * clipY + (double)shadowZ * clipZ) + (double)shadowX * clipX));

				drawContext->addClipPlane(clipX, clipY, clipZ, clipD);
			}

			drawContext->submitVertices(_shadowVertices);

			for (uint t = 0; t < mesh->trisets.size(); t++) {
				TriSet *triset = mesh->trisets[t];

				if (triset)
					drawContext->drawTriangles(kShadowTriangleFlags, nullptr, (int)triset->indices.size(), triset->indices.data());
			}

			drawContext->releaseVertices();
			drawContext->clearClipPlanes();
		}
	}

	drawContext->resetCameraMatrix();

	actorDrawShadowTimer.stop();

	return 0;
}

static const float kLookAtMinLength = 1e-20f;
static const float kLookAtDotLimit = 0.9999f;

static void setJointQuaternion(CNode *joint, const CQuaternion &rotation) {
	joint->quat = rotation;
	joint->nodeFlags = (joint->nodeFlags & ~(uint)CNode::kEulerValid) | CNode::kQuaternionValid | CNode::kLocalSet;

	if (joint->nodeFlags & CNode::kWorldValid)
		joint->updateChildrenLocalTransforms();

	if ((joint->nodeFlags & CNode::kPositionValid) == 0)
		joint->computeLocalPositionFromWorld();

	if ((joint->nodeFlags & (CNode::kEulerValid | CNode::kQuaternionValid)) == 0)
		joint->updateLocalQuaternion();

	joint->nodeFlags = (joint->nodeFlags & ~(uint)CNode::kWorldValid) | CNode::kLocalSet;
}

bool Actor::setActorHead(LString jointName, float *params) {
	if (!extra)
		allocateActorExtra();

	extra->lookAtActive = false;
	extra->lookAtHasTarget = false;
	extra->lookJointName = "";
	extra->lookOffset.x = 0.0f;
	extra->lookOffset.y = 0.0f;
	extra->lookOffset.z = 0.0f;

	if (!jointName.empty()) {
		CNode *joint = findNode(jointName);

		if (!joint)
			return false;

		if ((joint->nodeFlags & CNode::kQuaternionValid) == 0)
			joint->updateLocalQuaternion();

		extra->lookGoalQuat = joint->quat;
		extra->lookCurQuat = extra->lookGoalQuat;
		extra->lookAtActive = true;
		extra->lookJointName = jointName;

		if ((nodeFlags & CNode::kWorldValid) == 0)
			updateWorldTransform();

		float forwardX = world.rot.m[6];
		float forwardY = world.rot.m[7];
		float forwardZ = world.rot.m[8];

		if ((joint->nodeFlags & CNode::kWorldValid) == 0)
			joint->updateWorldTransform();

		extra->lookDirection.x = (float)((double)forwardX + joint->world.pos.x);
		extra->lookDirection.y = (float)((double)forwardY + joint->world.pos.y);
		extra->lookDirection.z = (float)((double)forwardZ + joint->world.pos.z);
		joint->worldToLocal(&extra->lookDirection);

		CVector &direction = extra->lookDirection;
		double length = sqrt(((double)direction.x * direction.x + (double)direction.y * direction.y) + (double)direction.z * direction.z);

		if (length >= (double)kLookAtMinLength) {
			double inverse = 1.0 / length;

			direction.x = (float)(inverse * direction.x);
			direction.y = (float)(inverse * direction.y);
			direction.z = (float)(inverse * direction.z);
		}

		if (!params) {
			warning("Actor::setActorHead(): no offset given");

			return true;
		}

		extra->lookOffset.x = params[0];
		extra->lookOffset.y = params[1];
		extra->lookOffset.z = params[2];
	}

	return true;
}

bool Actor::setHeadLimits(float horizontalLimit, float upLimit, float downLimit) {
	double halfAngle = (double)horizontalLimit * 0.5;
	float halfCos = (float)cos(halfAngle);
	float halfSin = (float)sin(halfAngle);

	if (!extra)
		allocateActorExtra();

	extra->lookLimits[0].x = halfCos;
	extra->lookLimits[0].y = 0.0f;
	extra->lookLimits[0].z = halfSin;
	extra->lookLimits[1].x = -halfCos;
	extra->lookLimits[1].y = 0.0f;
	extra->lookLimits[1].z = halfSin;
	extra->lookLimits[2].x = 0.0f;
	extra->lookLimits[2].y = -(float)cos((double)upLimit);
	extra->lookLimits[2].z = (float)sin((double)upLimit);
	extra->lookLimits[3].x = 0.0f;
	extra->lookLimits[3].y = (float)cos((double)downLimit);
	extra->lookLimits[3].z = (float)sin((double)downLimit);

	return true;
}

bool Actor::canActorSeePoint(const float *point, float *out) {
	if (!extra)
		return false;

	if (!point || !out) {
		warning("Actor::canActorSeePoint(): missing point or output");

		return false;
	}

	if ((nodeFlags & CNode::kWorldValid) == 0)
		updateWorldTransform();

	float forwardX = world.rot.m[6];
	float forwardZ = world.rot.m[8];
	double forwardLength = sqrt((double)world.rot.m[8] * world.rot.m[8] + (double)world.rot.m[6] * world.rot.m[6]);

	if (forwardLength >= (double)kLookAtMinLength) {
		double inverse = 1.0 / forwardLength;

		forwardX = (float)((double)world.rot.m[6] * inverse);
		forwardZ = (float)((double)world.rot.m[8] * inverse);
	}

	if ((nodeFlags & CNode::kWorldValid) == 0)
		updateWorldTransform();

	float deltaX = (float)((double)point[0] - world.pos.x);
	double deltaY = (double)point[1] - world.pos.y;
	float deltaZ = (float)((double)point[2] - world.pos.z);
	double distance = sqrt(((double)deltaZ * deltaZ + deltaY * deltaY) + (double)deltaX * deltaX);
	float directionX = deltaX;
	float directionZ = deltaZ;

	if (distance >= (double)kLookAtMinLength) {
		double inverse = 1.0 / distance;

		directionX = (float)((double)deltaX * inverse);
		directionZ = (float)((double)deltaZ * inverse);
		*out = (float)distance;
	} else {
		*out = 0.0f;
	}

	double horizontal = sqrt((double)directionZ * directionZ + (double)directionX * directionX);
	float horizontalStored = (float)horizontal;
	double normalizedX = directionX;
	float normalizedZ = directionZ;

	if (horizontal >= (double)kLookAtMinLength) {
		double inverse = 1.0 / (double)horizontalStored;
		float inverseStored = (float)inverse;

		normalizedX = inverse * (double)directionX;
		normalizedZ = (float)((double)directionZ * inverseStored);
	}

	double dot = ((double)forwardX * normalizedX + (double)normalizedZ * (double)forwardZ) + 0.0 * 0.0;

	return dot >= (double)extra->lookCosFov;
}

bool Actor::canActorSeeActor(Actor *other, float *out) {
	if (!other) {
		warning("Actor::canActorSeeActor(): no actor");

		return false;
	}

	if (!other->room.resource)
		return false;

	if (!room.resource)
		return false;

	if (room.resource->name != other->room.resource->name)
		return false;

	if ((other->nodeFlags & CNode::kWorldValid) == 0)
		other->updateWorldTransform();

	return canActorSeePoint(&other->world.pos.x, out);
}

void Actor::setLookAtJoints(float deltaTime) {
	CQuaternion slerped;
	CQuaternion target;

	if (!extra)
		return;

	if (extra->lookTargetActorId != 0) {
		Actor *targetActor = actorDeref(extra->lookTargetActorId);

		if (targetActor) {
			if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
				targetActor->updateWorldTransform();

			extra->lookTarget.x = targetActor->world.pos.x;
			extra->lookTarget.y = targetActor->world.pos.y;
			extra->lookTarget.z = targetActor->world.pos.z;
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("SetLookAtJoints trying to look at deleted actor!!");
			else
				warning("SetLookAtJoints trying to look at deleted actor!!");

			extra->lookTargetActorId = 0;
			extra->lookAtHasTarget = false;
		}
	}

	CNode *joint = findNode(extra->lookJointName);

	if (!joint)
		return;

	if ((joint->nodeFlags & CNode::kQuaternionValid) == 0)
		joint->updateLocalQuaternion();

	CQuaternion jointQuat = joint->quat;
	CVector direction = extra->lookDirection;

	if (extra->lookAtHasTarget) {
		CVector aim;
		aim.x = (float)((double)extra->lookOffset.x + extra->lookTarget.x);
		aim.y = (float)((double)extra->lookOffset.y + extra->lookTarget.y);
		aim.z = (float)((double)extra->lookOffset.z + extra->lookTarget.z);

		CVector toTarget = aim;
		joint->worldToLocal(&toTarget);

		CPlane plane;
		plane.n.x = 0.0f;
		plane.n.y = 0.0f;
		plane.n.z = 0.0f;
		plane.d = 0.0f;

		CVector worldLimit;

		for (int i = 0; i < Extra_v2::kNumLookLimits; i++) {
			localToWorldDirection(&extra->lookLimits[i], &worldLimit);

			if ((joint->nodeFlags & CNode::kWorldValid) == 0)
				joint->updateWorldTransform();

			plane.setFromNormalAndPoint(worldLimit, joint->world.pos);

			if ((joint->nodeFlags & CNode::kWorldValid) == 0)
				joint->updateWorldTransform();

			plane.transformToLocal(&joint->world);

			if (plane.classifyPoint(toTarget) == CPlane::kPointBehind)
				plane.projectPoint(&toTarget);
		}

		double targetLength = sqrt(((double)toTarget.x * toTarget.x + (double)toTarget.y * toTarget.y) + (double)toTarget.z * toTarget.z);

		if (targetLength >= (double)kLookAtMinLength) {
			double inverse = 1.0 / targetLength;

			toTarget.x = (float)(inverse * toTarget.x);
			toTarget.y = (float)(inverse * toTarget.y);
			toTarget.z = (float)(inverse * toTarget.z);
		}

		float cosine = (float)(((double)toTarget.z * direction.z + (double)toTarget.y * direction.y) + (double)direction.x * toTarget.x);
		CVector cross;
		cross.x = (float)((double)toTarget.z * direction.y - (double)toTarget.y * direction.z);
		cross.y = (float)((double)direction.z * toTarget.x - (double)toTarget.z * direction.x);
		cross.z = (float)((double)toTarget.y * direction.x - (double)direction.y * toTarget.x);

		CVector axis = cross;
		double crossLength = sqrt(((double)cross.z * cross.z + (double)cross.y * cross.y) + (double)cross.x * cross.x);

		if (crossLength >= (double)kLookAtMinLength) {
			double inverse = 1.0 / crossLength;

			axis.x = (float)((double)cross.x * inverse);
			axis.y = (float)((double)axis.y * inverse);
			axis.z = (float)((double)axis.z * inverse);
		}

		float angle = (float)acos((double)cosine);
		CQuaternion rotation(axis, angle);
		const CQuaternion &goal = extra->lookGoalQuat;
		float resultW = (float)((((double)rotation.w * goal.w - (double)goal.x * rotation.x) - (double)goal.y * rotation.y) - (double)goal.z * rotation.z);
		float resultZ = (float)((((double)goal.x * rotation.y + (double)rotation.w * goal.z) + (double)rotation.z * goal.w) - (double)goal.y * rotation.x);
		float resultY = (float)((((double)goal.z * rotation.x + (double)rotation.w * goal.y) + (double)rotation.y * goal.w) - (double)goal.x * rotation.z);
		float resultX = (float)((((double)rotation.w * goal.x + (double)goal.y * rotation.z) + (double)rotation.x * goal.w) - (double)goal.z * rotation.y);

		target = CQuaternion(resultX, resultY, resultZ, resultW);
	} else {
		target = jointQuat;
	}

	float step = (float)((double)deltaTime * extra->lookRate);
	float dot = extra->lookCurQuat.dot(target);

	// The disasm clamps the dot product to the open interval
	if (dot > kLookAtDotLimit)
		dot = kLookAtDotLimit;
	else if (!(dot >= -kLookAtDotLimit))
		dot = -kLookAtDotLimit;

	double span = acos((double)dot);
	span = span + span;

	if (span >= (double)step) {
		float fraction = (float)((double)step / span);

		extra->lookCurQuat.slerp(target, fraction, &slerped);
		slerped.normalize();
		setJointQuaternion(joint, slerped);
		extra->lookCurQuat = slerped;
	} else {
		setJointQuaternion(joint, target);
		extra->lookCurQuat = target;
	}
}

static const int kSavedStringBufferSize = 512;
static const uint kMaxSkippedStateSize = 65536;
static const char *const kOverworldRoomName = "[OW]";

static void writeSavedString(tSaveRestoreAccess *access, const LString &string) {
	int length = (int)string.size() + 1;

	access(&length, sizeof(length));
	access((void *)string.c_str(), length);
}

static bool readSavedString(tSaveRestoreAccess *access, LString &string) {
	char buffer[kSavedStringBufferSize + 1];
	uint length = 0;

	access(&length, sizeof(length));

	if (length > (uint)kSavedStringBufferSize) {
		warning("Actor::restore(): invalid string length %u", length);

		return false;
	}

	access(buffer, length);
	buffer[length] = 0;
	string = buffer;

	return true;
}

static void writeSavedFlag(tSaveRestoreAccess *access, bool value) {
	byte raw = value ? 1 : 0;

	access(&raw, sizeof(raw));
}

static bool readSavedFlag(tSaveRestoreAccess *access) {
	byte raw = 0;

	access(&raw, sizeof(raw));

	return raw != 0;
}

static void restoreNodeEuler(CNode *node, const CVector &angles) {
	node->eulerDeg = angles;
	node->nodeFlags = (node->nodeFlags & ~CNode::kQuaternionValid) | CNode::kEulerValid | CNode::kLocalSet;
	node->updateLocalState();
}

bool Actor::save(tSaveRestoreAccess *access) {
	writeSavedString(access, actorName);
	access(&id, sizeof(id));
	access(&actorFlags, sizeof(actorFlags));
	access(&mode, sizeof(mode));

	if ((nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	access(&pos, sizeof(pos));

	if ((nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	access(&eulerDeg, sizeof(eulerDeg));
	access(&prevPos, sizeof(prevPos));
	access(&reserved312, sizeof(reserved312));
	writeSavedFlag(access, onScreen);
	access(&sortOrder, sizeof(sortOrder));

	if (parentActorId && !actorDeref(parentActorId))
		parentActorId = 0;

	access(&parentActorId, sizeof(parentActorId));

	if (parentActorId) {
		if (parent) {
			access(parent->name, CNode::kNameSize);
		} else {
			char emptyName[CNode::kNameSize];

			warning("Actor::save(): the parent node is missing");
			memset(emptyName, 0, sizeof(emptyName));
			access(emptyName, CNode::kNameSize);
		}
	}

	for (int i = 0; i < 4; i++)
		access(&drawnRect[i], sizeof(int));

	LString roomName;

	if (room.resource) {
		bool locked = room.lock();

		if (locked && room.typed)
			roomName = room.typed->name;
		else
			warning("Actor::save(): the room could not be locked");

		if (locked)
			room.unlock();
	} else if (inOverworld) {
		roomName = kOverworldRoomName;
	}

	writeSavedString(access, roomName);

	removeFinishedChores();

	int choreCount = (int)choreIds.size();

	access(&choreCount, sizeof(choreCount));

	Common::List<uint>::iterator choreIt;

	for (choreIt = choreIds.begin(); choreIt != choreIds.end(); ++choreIt) {
		uint choreId = *choreIt;

		access(&choreId, sizeof(choreId));

		if (ChoreInstance::isInstanceValid(choreId)) {
			ChoreInstance *instance = ChoreInstance::getInstance(choreId);

			if (instance)
				instance->save(access);
		}
	}

	for (int i = 0; i < kNumChoreSlots; i++) {
		writeSavedString(access, choreSlotNames[i]);
		writeSavedString(access, choreSlotSetNames[i]);
		access(&choreSlotIds[i], sizeof(uint));
	}

	int primitiveCount = (int)instances.size();

	access(&primitiveCount, sizeof(primitiveCount));

	for (int i = 0; i < primitiveCount; i++) {
		RPrimitiveInstance *instance = instances[i];

		if (instance->prim) {
			writeSavedString(access, instance->prim->name);
		} else {
			warning("Actor::save(): the primitive instance has no primitive");
			writeSavedString(access, LString());
		}

		int stateSize = instance->getSaveSize();

		access(&stateSize, sizeof(stateSize));
		instance->saveState(access);
	}

	int shadowCount = (int)shadows.size();

	access(&shadowCount, sizeof(shadowCount));

	for (int i = 0; i < shadowCount; i++)
		writeSavedString(access, shadows[i]->name);

	writeSavedFlag(access, extra != nullptr);

	if (extra) {
		writeSavedFlag(access, extra->lookAtActive);
		writeSavedFlag(access, extra->lookAtHasTarget);
		access(&extra->lookTarget, sizeof(CVector));
		access(&extra->lookOffset, sizeof(CVector));
		access(&extra->lookTargetActorId, sizeof(int));
		access(&extra->lookGoalQuat, sizeof(CQuaternion));
		access(&extra->lookDirection, sizeof(CVector));
		access(&extra->lookCurQuat, sizeof(CQuaternion));
		access(extra->lookLimits, Extra_v2::kNumLookLimits * sizeof(CVector));
		writeSavedString(access, extra->lookJointName);
		access(&extra->lookCosFov, sizeof(float));
		access(&extra->lookRate, sizeof(float));
		writeSavedFlag(access, extra->walking);

		// The disasm saves only the first byte of turnDir
		byte turnDirLow = (byte)(extra->turnDir & 0xFF);

		access(&turnDirLow, sizeof(turnDirLow));
		writeSavedFlag(access, extra->resting);
		access(&extra->walkRate, sizeof(float));
		access(&extra->turnRate, sizeof(float));
		writeSavedFlag(access, extra->aligned);
		writeSavedFlag(access, extra->snapTurn);
		access(&extra->offsetYaw, sizeof(float));
		access(&extra->alignOff, sizeof(float));
		access(&extra->walkTarget, sizeof(CVector));
		access(&extra->goal, sizeof(CVector));
		access(&extra->numPathPoints, sizeof(int));
		access(extra->path, Extra_v2::kMaxPathPoints * sizeof(CVector));
		access(&extra->pathIndex, sizeof(int));
		writeSavedFlag(access, extra->puck != nullptr);
		writeSavedFlag(access, extra->inBoxes);
		writeSavedFlag(access, extra->walkDominate);
		access(&extra->reflectionAngle, sizeof(float));
		writeSavedFlag(access, extra->talking);
		access(&extra->talkTimerMs, sizeof(int));
		access(extra->sayLineId, Extra_v2::kSayLineIdSize);
		access(&extra->textEventId, sizeof(int));
		writeSavedFlag(access, extra->lipsyncActive);
		access(&extra->soundHandle, sizeof(int));
		access(&extra->visemeCurrent, sizeof(int));
		access(&extra->collisionMode, sizeof(int));
		access(&extra->collisionScale, sizeof(float));
		writeSavedFlag(access, extra->inCollision);
		access(&extra->talkColor, sizeof(Color));
		access(&extra->timeScale, sizeof(float));

		if (extra->puck) {
			Puck_v2 *puck = extra->puck;

			if ((puck->nodeFlags & kPositionValid) == 0)
				puck->computeLocalPositionFromWorld();

			access(&puck->pos, sizeof(CVector));

			if ((puck->nodeFlags & kEulerValid) == 0)
				puck->computeLocalMatrixFromWorld();

			access(&puck->eulerDeg, sizeof(CVector));

			// The disasm saves only the first byte of the reflection angle
			byte reflectionRaw[sizeof(float)];

			memcpy(reflectionRaw, &puck->reflectionAngle, sizeof(reflectionRaw));
			access(&reflectionRaw[0], sizeof(byte));
			writeSavedFlag(access, puck->constrained);
			access(puck->constraint, sizeof(puck->constraint));
			access(puck->lineA, sizeof(puck->lineA));
			access(puck->lineB, sizeof(puck->lineB));
		}
	}

	access(&drawFlags, sizeof(drawFlags));

	return true;
}

bool Actor::restore(tSaveRestoreAccess *access) {
	CVector savedVector;

	if (!readSavedString(access, actorName))
		return false;

	access(&id, sizeof(id));
	access(&actorFlags, sizeof(actorFlags));
	access(&mode, sizeof(mode));
	access(&savedVector, sizeof(savedVector));
	setLocalPosition(savedVector);
	access(&savedVector, sizeof(savedVector));
	restoreNodeEuler(this, savedVector);
	access(&prevPos, sizeof(prevPos));
	access(&reserved312, sizeof(reserved312));
	onScreen = readSavedFlag(access);
	access(&sortOrder, sizeof(sortOrder));
	access(&parentActorId, sizeof(parentActorId));

	if (parentActorId)
		access(parentNodeName, CNode::kNameSize);

	for (int i = 0; i < 4; i++)
		access(&drawnRect[i], sizeof(int));

	LString savedRoomName;

	if (!readSavedString(access, savedRoomName))
		return false;

	if (savedRoomName == kOverworldRoomName)
		Overworld::getInstance()->putActor(this);
	else if (!savedRoomName.empty())
		putInSet(savedRoomName.c_str());
	else
		putInSet(nullptr);

	uint choreCount = 0;

	access(&choreCount, sizeof(choreCount));

	for (uint i = 0; i < choreCount; i++) {
		uint choreId = 0;

		access(&choreId, sizeof(choreId));

		if (ChoreInstance::isInstanceValid(choreId)) {
			ChoreInstance *existing = ChoreInstance::getInstance(choreId);

			if (existing)
				delete existing;
		}

		choreId = ChoreInstance::newInstanceWithId(this, choreId);

		if (ChoreInstance::isInstanceValid(choreId)) {
			ChoreInstance *instance = ChoreInstance::getInstance(choreId);

			if (instance)
				instance->restore(access);
		}

		choreIds.push_back(choreId);
	}

	for (int i = 0; i < kNumChoreSlots; i++) {
		if (!readSavedString(access, choreSlotNames[i]))
			return false;

		if (!readSavedString(access, choreSlotSetNames[i]))
			return false;

		access(&choreSlotIds[i], sizeof(uint));
	}

	uint primitiveCount = 0;

	access(&primitiveCount, sizeof(primitiveCount));

	for (uint i = 0; i < primitiveCount; i++) {
		LString primitiveName;
		bool found = false;

		if (!readSavedString(access, primitiveName))
			return false;

		for (uint j = 0; j < instances.size() && !found; j++) {
			RPrimitiveInstance *instance = instances[j];

			if (instance->prim && instance->prim->name == primitiveName && !instance->stateRestored) {
				int stateSize = 0;

				access(&stateSize, sizeof(stateSize));
				instance->restoreState(access);
				found = true;
			}
		}

		if (!found) {
			uint stateSize = 0;

			Common::String message = Common::String::format("Could not find file %s.", primitiveName.c_str());

			warning("%s", message.c_str());
			GUI::displayErrorDialog(Common::U32String(message));
			access(&stateSize, sizeof(stateSize));

			if (stateSize > kMaxSkippedStateSize) {
				warning("Actor::restore(): invalid primitive state size %u", stateSize);

				return false;
			}

			byte discard[256];

			while (stateSize > 0) {
				uint chunk = stateSize < sizeof(discard) ? stateSize : (uint)sizeof(discard);

				access(discard, chunk);
				stateSize -= chunk;
			}
		}
	}

	uint shadowCount = 0;

	access(&shadowCount, sizeof(shadowCount));

	for (uint i = 0; i < shadowCount; i++) {
		LString shadowName;

		if (!readSavedString(access, shadowName))
			return false;

		bool locked = room.lock();
		Room_v2 *lockedRoom = locked ? room.typed : nullptr;

		if (lockedRoom)
			activateShadow(true, lockedRoom->findShadow(shadowName));
		else
			warning("Actor::restore(): the room could not be locked");

		if (locked)
			room.unlock();
	}

	if (readSavedFlag(access)) {
		extra = nullptr;
		allocateActorExtra();

		extra->lookAtActive = readSavedFlag(access);
		extra->lookAtHasTarget = readSavedFlag(access);
		access(&extra->lookTarget, sizeof(CVector));
		access(&extra->lookOffset, sizeof(CVector));
		access(&extra->lookTargetActorId, sizeof(int));
		access(&extra->lookGoalQuat, sizeof(CQuaternion));
		access(&extra->lookDirection, sizeof(CVector));
		access(&extra->lookCurQuat, sizeof(CQuaternion));
		access(extra->lookLimits, Extra_v2::kNumLookLimits * sizeof(CVector));

		if (!readSavedString(access, extra->lookJointName))
			return false;

		access(&extra->lookCosFov, sizeof(float));
		access(&extra->lookRate, sizeof(float));
		extra->walking = readSavedFlag(access);

		// The disasm restores only the first byte of turnDir
		byte turnDirLow = 0;

		access(&turnDirLow, sizeof(turnDirLow));
		extra->turnDir = (extra->turnDir & ~0xFF) | turnDirLow;
		extra->resting = readSavedFlag(access);
		access(&extra->walkRate, sizeof(float));
		access(&extra->turnRate, sizeof(float));
		extra->aligned = readSavedFlag(access);
		extra->snapTurn = readSavedFlag(access);
		access(&extra->offsetYaw, sizeof(float));
		access(&extra->alignOff, sizeof(float));
		access(&extra->walkTarget, sizeof(CVector));
		access(&extra->goal, sizeof(CVector));
		access(&extra->numPathPoints, sizeof(int));
		access(extra->path, Extra_v2::kMaxPathPoints * sizeof(CVector));
		access(&extra->pathIndex, sizeof(int));

		bool hasPuck = readSavedFlag(access);

		extra->inBoxes = readSavedFlag(access);
		extra->walkDominate = readSavedFlag(access);
		access(&extra->reflectionAngle, sizeof(float));
		extra->talking = readSavedFlag(access);
		access(&extra->talkTimerMs, sizeof(int));
		access(extra->sayLineId, Extra_v2::kSayLineIdSize);
		access(&extra->textEventId, sizeof(int));
		extra->lipsyncActive = readSavedFlag(access);
		access(&extra->soundHandle, sizeof(int));
		access(&extra->visemeCurrent, sizeof(int));
		access(&extra->collisionMode, sizeof(int));
		access(&extra->collisionScale, sizeof(float));
		extra->inCollision = readSavedFlag(access);
		access(&extra->talkColor, sizeof(Color));
		access(&extra->timeScale, sizeof(float));

		if (hasPuck) {
			Puck_v2 *puck = new Puck_v2(extra->reflectionAngle);
			CVector puckVector;

			extra->puck = puck;
			puck->actorName = actorName;

			access(&puckVector, sizeof(puckVector));
			puck->setLocalPosition(puckVector);
			access(&puckVector, sizeof(puckVector));
			restoreNodeEuler(puck, puckVector);

			byte reflectionRaw[sizeof(float)];

			memcpy(reflectionRaw, &puck->reflectionAngle, sizeof(reflectionRaw));
			access(&reflectionRaw[0], sizeof(byte));
			memcpy(&puck->reflectionAngle, reflectionRaw, sizeof(reflectionRaw));
			puck->constrained = readSavedFlag(access);
			access(puck->constraint, sizeof(puck->constraint));
			access(puck->lineA, sizeof(puck->lineA));
			access(puck->lineB, sizeof(puck->lineB));

			if (extra->inBoxes && room.resource) {
				bool locked = room.lock();
				Room_v2 *lockedRoom = locked ? room.typed : nullptr;

				if (lockedRoom)
					puck->validate(lockedRoom->walkPlanes);
				else
					warning("Actor::restore(): the room could not be locked");

				if (locked)
					room.unlock();
			}
		}

		if (extra->talking) {
			for (int i = 0; i < kNumSpeakers; i++) {
				if (!_speakers[i]) {
					_speakers[i] = this;
					_numSpeakers++;

					break;
				}
			}
		}
	}

	access(&drawFlags, sizeof(drawFlags));

	return true;
}

void Actor::saveActors(tSaveRestoreAccess *access) {
	int selectedId = -1;

	if (_selectedActor)
		selectedId = _selectedActor->id;

	access(&selectedId, sizeof(selectedId));

	int actorCount = (int)_actors.size();

	access(&actorCount, sizeof(actorCount));

	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it)
		(*it)->save(access);

	access(&ChoreInstance::_nextId, sizeof(ChoreInstance::_nextId));
	access(&_nextId, sizeof(_nextId));
}

void Actor::restoreActors(tSaveRestoreAccess *access) {
	for (int i = 0; i < kNumSpeakers; i++)
		_speakers[i] = nullptr;

	_numSpeakers = 0;

	int selectedId = 0;
	int actorCount = 0;

	access(&selectedId, sizeof(selectedId));
	access(&actorCount, sizeof(actorCount));

	for (int i = 0; i < actorCount; i++) {
		Actor *actor = new Actor("Restored");

		actor->restore(access);
	}

	access(&ChoreInstance::_nextId, sizeof(ChoreInstance::_nextId));
	access(&_nextId, sizeof(_nextId));

	_selectedActor = nullptr;

	if (selectedId >= 0)
		_selectedActor = actorDeref(selectedId);

	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		Actor *actor = *it;

		if (!actor->parentActorId)
			continue;

		Actor *parentActor = actorDeref(actor->parentActorId);

		if (parentActor) {
			char nodeName[CNode::kNameSize + 1];

			memcpy(nodeName, actor->parentNodeName, CNode::kNameSize);
			nodeName[CNode::kNameSize] = 0;

			CNode *parentNode = parentActor->findNode(nodeName);

			if (!parentNode)
				parentNode = parentActor;

			actor->setParent(parentNode);
		} else {
			actor->parentActorId = 0;
			actor->parentNodeName[0] = 0;
		}
	}
}

} // End of namespace Grim
