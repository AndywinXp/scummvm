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

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "common/array.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/clight.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/inputrecorder.h"
#include "engines/grim/emi/irect.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"

namespace Grim {

static const uint kSavedRoomNameSize = 128;

bool Room_v2::_started = false;
Room_v2 *Room_v2::_currentRoom = nullptr;
LResourceHandle<Room_v2> Room_v2::_currentHandle;
Room_v2 *Room_v2::_lastDrawnRoom = nullptr;
int Room_v2::_lastDrawnSetup = -1;

enum {
	kDrawLayer = 0,
	kDrawActor = 1,
	kDrawShadow = 2,
	kNumSortBins = 16
};

struct DrawItem {
	int key;
	float depth;
	int type;
	void *object;
	int index;
};

static ProfilerTimer roomDrawTimer("Room::Draw()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);

static void mergeDrawItems(Common::Array<DrawItem> &first, Common::Array<DrawItem> &second) {
	Common::Array<DrawItem> merged;
	uint firstIndex = 0;
	uint secondIndex = 0;

	while (firstIndex < first.size() && secondIndex < second.size()) {
		const DrawItem &left = first[firstIndex];
		const DrawItem &right = second[secondIndex];
		bool takeRight;

		if (right.key == left.key)
			takeRight = (double)right.depth > (double)left.depth;
		else
			takeRight = right.key > left.key;

		if (takeRight) {
			merged.push_back(right);
			secondIndex++;
		} else {
			merged.push_back(left);
			firstIndex++;
		}
	}

	while (firstIndex < first.size()) {
		merged.push_back(first[firstIndex]);
		firstIndex++;
	}

	while (secondIndex < second.size()) {
		merged.push_back(second[secondIndex]);
		secondIndex++;
	}

	first = merged;
	second.clear();
}

static void sortDrawItems(Common::Array<DrawItem> &items) {
	Common::Array<DrawItem> bins[kNumSortBins];
	Common::Array<DrawItem> carry;
	uint maxBin = 0;
	uint next = 0;

	while (next < items.size()) {
		carry.clear();
		carry.push_back(items[next]);
		next++;

		uint bin = 0;

		while (bin < maxBin && !bins[bin].empty()) {
			mergeDrawItems(bins[bin], carry);
			SWAP(bins[bin], carry);
			bin++;
		}

		if (bin == kNumSortBins - 1) {
			mergeDrawItems(bins[bin], carry);
		} else {
			SWAP(bins[bin], carry);

			if (bin == maxBin)
				maxBin++;
		}
	}

	items.clear();

	for (uint bin = maxBin; bin > 0; bin--)
		mergeDrawItems(items, bins[bin - 1]);
}

Room_v2::Room_v2() {
	setups = nullptr;
	numSetups = 0;
	currentSetup = 0;
	walkPlanes = nullptr;
	minVol = 0;
	maxVol = 0;
	maxDist = 0.0f;
	panFactor = 0.0f;
	maxRadius = 0.0f;
}

Room_v2::~Room_v2() {
	freeSetups();

	if (walkPlanes) {
		delete walkPlanes;
		walkPlanes = nullptr;
	}

	while (!lights.empty()) {
		CLightNode *lightNode = lights.front();

		if (lightNode) {
			removeLight(lightNode);

			if (lightNode->light) {
				delete lightNode->light;
				lightNode->light = nullptr;
			}

			delete lightNode;
		} else {
			warning("Room_v2::~Room_v2(): null light");
			lights.pop_front();
		}
	}

	while (!shadows.empty())
		removeShadow(shadows.front());

	actors.clear();
}

uint Room_v2::resSizeOfItem() {
	uint size = sizeof(Room_v2);

	for (int i = 0; i < numSetups; i++)
		size += setups[i].getSize();

	size += sizeof(Shadow_v2 *) * shadows.size();

	Common::List<Shadow_v2 *>::iterator shadowIt;

	for (shadowIt = shadows.begin(); shadowIt != shadows.end(); ++shadowIt)
		size += (*shadowIt)->getSize();

	size += sizeof(CLightNode *) * lights.size();

	Common::List<CLightNode *>::iterator lightIt;

	for (lightIt = lights.begin(); lightIt != lights.end(); ++lightIt) {
		if (*lightIt) {
			size += sizeof(CLightNode);

			if ((*lightIt)->light)
				size += sizeof(CLight);
		}
	}

	size += sizeof(Actor *) * actors.size();

	if (walkPlanes)
		size += walkPlanes->getSize();

	return size;
}

bool Room_v2::resLoadItem(const char *itemName) {
	LString textPath(itemName);
	LString binaryPath = textPath + "b";
	bool binaryExists = gpFileServices->exists(binaryPath.c_str());
	bool textExists = gpFileServices->exists(textPath.c_str());
	bool useText = true;

	if (binaryExists) {
		if (!textExists)
			useText = false;
		else if (gpFileServices->getModTime(binaryPath.c_str()) >= gpFileServices->getModTime(textPath.c_str()))
			useText = false;
	}

	if (useText) {
		if (!loadText(itemName)) {
			if (gpConsoleServices)
				gpConsoleServices->logError("Parse problem in file '%s'.\n", itemName);
			else
				warning("Parse problem in file '%s'.", itemName);

			return false;
		}

		if (walkPlanes) {
			Common::List<Shadow_v2 *>::iterator it;

			for (it = shadows.begin(); it != shadows.end(); ++it)
				(*it)->resolveSectors(walkPlanes);
		} else if (!shadows.empty()) {
			warning("Room_v2::resLoadItem(): shadows without sectors");
		}

		saveBinary(binaryPath.c_str());
	} else if (!loadBinary(binaryPath.c_str())) {
		if (gpConsoleServices)
			gpConsoleServices->logError("Parse problem in file '%s'.\n", binaryPath.c_str());
		else
			warning("Parse problem in file '%s'.", binaryPath.c_str());

		return false;
	}

	return true;
}

void Room_v2::setSetup(int setupIndex) {
	int index = setupIndex;

	if (numSetups <= 0) {
		warning("Room_v2::setSetup(): the room has no setups");

		return;
	}

	while (index < 0)
		index += numSetups;

	if (index >= numSetups)
		index %= numSetups;

	int oldSetup = currentSetup;

	if (index != oldSetup) {
		currentSetup = index;
		Lua_V2::callCameraChange(oldSetup, index);
	}
}

Camera *Room_v2::getSetupCamera(int setupIndex) {
	int index = setupIndex;

	if (index < 0 || index >= numSetups)
		index = currentSetup;

	if (!setups || index < 0 || index >= numSetups) {
		warning("Room_v2::getSetupCamera(): invalid setup %d", index);

		return nullptr;
	}

	return &setups[index].camera;
}

int Room_v2::draw(CDrawContext *drawContext) {
	roomDrawTimer.start();

	if (!setups || currentSetup < 0 || currentSetup >= numSetups) {
		warning("Room_v2::draw(): invalid setup %d", currentSetup);
		roomDrawTimer.stop();

		return 0;
	}

	int layersDrawn = 0;
	Camera *camera = &setups[currentSetup].camera;
	camera->updateViewMatrix();
	CTransform view = camera->view;

	if (_lastDrawnRoom == this && _lastDrawnSetup == currentSetup) {
		Setup *setup = &setups[_lastDrawnSetup];
		drawContext->clear(false, true);

		Common::Array<DrawItem> items;
		Common::List<Layer *>::iterator layerIt;

		for (layerIt = setup->layers.begin(); layerIt != setup->layers.end(); ++layerIt) {
			DrawItem item;
			item.key = (*layerIt)->sortOrder;
			item.depth = 100000.0f;
			item.type = kDrawLayer;
			item.object = *layerIt;
			item.index = 0;
			items.push_back(item);
		}

		Common::List<Actor *>::iterator actorIt;

		for (actorIt = actors.begin(); actorIt != actors.end(); ++actorIt) {
			Actor *actor = *actorIt;

			if ((actor->nodeFlags & CNode::kWorldValid) == 0)
				actor->updateWorldTransform();

			float x = actor->world.pos.x;
			float y = actor->world.pos.y;
			float z = actor->world.pos.z;
			double product = (double)view.rot.m[5] * y + (double)view.rot.m[2] * x;
			product = product + (double)view.rot.m[8] * z;
			float depth = (float)((double)view.pos.z + product);

			if (!actor->instances.empty()) {
				DrawItem item;
				item.key = actor->getSortOrder(_lastDrawnSetup);
				item.depth = depth;
				item.type = kDrawActor;
				item.object = actor;
				item.index = 0;
				items.push_back(item);
			}

			for (uint i = 0; i < actor->shadows.size(); i++) {
				DrawItem item;
				item.key = actor->getSortOrder(_lastDrawnSetup);
				item.type = kDrawShadow;
				item.depth = depth;
				item.object = actor;
				item.index = (int)i;
				items.push_back(item);
			}
		}

		if (items.size() >= 2)
			sortDrawItems(items);

		for (uint i = 0; i < items.size(); i++) {
			DrawItem &item = items[i];

			if (item.key < 0)
				continue;

			if (item.type == kDrawLayer) {
				layersDrawn += ((Layer *)item.object)->draw(drawContext);
			} else if (item.type == kDrawActor) {
				setup->camera.applyProjection();
				((Actor *)item.object)->drawActor(&setup->camera);
			} else if (item.type == kDrawShadow) {
				Actor *actor = (Actor *)item.object;
				setup->camera.applyProjection();
				actor->drawShadow(&setup->camera, actor->shadows[item.index]);
			}
		}

		roomDrawTimer.stop();

		return layersDrawn;
	}

	_lastDrawnRoom = this;
	_lastDrawnSetup = currentSetup;
	setups[_lastDrawnSetup].buildLayers();
	Sounds::setSoundListener(camera);
	Lua_V2::callPostCameraChange(_lastDrawnSetup);
	roomDrawTimer.stop();

	return -1;
}

void Room_v2::addActor(Actor *actor) {
	actors.push_back(actor);
}

int Room_v2::removeActor(Actor *actor) {
	int count = 0;
	Common::List<Actor *>::iterator it = actors.begin();

	while (it != actors.end()) {
		if (*it == actor) {
			count++;
			it = actors.erase(it);
		} else {
			++it;
		}
	}

	return count;
}

void Room_v2::setLightsActive(bool active) {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it)
			(*it)->setActive(active);
	}
}

void Room_v2::saveLightStates() {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it)
			(*it)->savedActive = (*it)->active;
	}
}

void Room_v2::restoreLightStates() {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it)
			(*it)->setActive((*it)->savedActive);
	}
}

CLightNode *Room_v2::findLight(const char *lightName) {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it && scumm_stricmp(lightName, (*it)->name) == 0)
			return *it;
	}

	return nullptr;
}

void Room_v2::addLight(CLightNode *light) {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it == light)
			return;
	}

	lights.push_back(light);
}

void Room_v2::removeLight(CLightNode *light) {
	light->setActive(false);

	Common::List<CLightNode *>::iterator it = lights.begin();

	while (it != lights.end()) {
		if (*it == light)
			it = lights.erase(it);
		else
			++it;
	}
}

Shadow_v2 *Room_v2::findShadow(const LString &shadowName) {
	Common::List<Shadow_v2 *>::iterator it;

	for (it = shadows.begin(); it != shadows.end(); ++it) {
		if (*it && (*it)->name == shadowName)
			return *it;
	}

	return nullptr;
}

Shadow_v2 *Room_v2::newShadow() {
	Shadow_v2 *shadow = new Shadow_v2();

	shadows.push_back(shadow);

	return shadow;
}

void Room_v2::removeShadow(Shadow_v2 *shadow) {
	Common::List<Shadow_v2 *>::iterator it = shadows.begin();

	while (it != shadows.end()) {
		if (*it == shadow)
			it = shadows.erase(it);
		else
			++it;
	}

	delete shadow;
}

void Room_v2::setSoundParameters(float newMaxDist, char newMinVol, char newMaxVol, float newPanFactor, float newMaxRadius) {
	maxDist = newMaxDist;
	minVol = newMinVol;
	maxVol = newMaxVol;
	panFactor = newPanFactor;
	maxRadius = newMaxRadius;
}

bool Room_v2::save(tSaveRestoreAccess *access) {
	access(&currentSetup, sizeof(currentSetup));
	access(&minVol, sizeof(minVol));
	access(&maxVol, sizeof(maxVol));
	access(&maxDist, sizeof(maxDist));
	access(&panFactor, sizeof(panFactor));
	access(&maxRadius, sizeof(maxRadius));

	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		CLightNode *lightNode = *it;

		if (lightNode) {
			if ((lightNode->nodeFlags & CNode::kWorldValid) == 0)
				lightNode->updateWorldTransform();

			float position[3];

			position[0] = lightNode->world.pos.x;
			position[1] = lightNode->world.pos.y;
			position[2] = lightNode->world.pos.z;
			access(position, sizeof(position));

			byte activeByte = lightNode->active ? 1 : 0;

			access(&activeByte, sizeof(activeByte));
		}
	}

	for (int i = 0; i < numSetups; i++)
		setups[i].save(access);

	if (walkPlanes)
		walkPlanes->save(access);
	else
		warning("Room_v2::save(): the room has no sectors");

	return true;
}

bool Room_v2::restore(tSaveRestoreAccess *access) {
	access(&currentSetup, sizeof(currentSetup));
	access(&minVol, sizeof(minVol));
	access(&maxVol, sizeof(maxVol));
	access(&maxDist, sizeof(maxDist));
	access(&panFactor, sizeof(panFactor));
	access(&maxRadius, sizeof(maxRadius));

	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		CLightNode *lightNode = *it;

		if (lightNode) {
			float position[3] = { 0.0f, 0.0f, 0.0f };

			access(position, sizeof(position));

			CVector newPosition;

			newPosition.x = position[0];
			newPosition.y = position[1];
			newPosition.z = position[2];
			lightNode->setLocalPosition(newPosition);

			byte activeByte = 0;

			access(&activeByte, sizeof(activeByte));
			lightNode->setActive(activeByte != 0);
		}
	}

	for (int i = 0; i < numSetups; i++)
		setups[i].restore(access);

	if (walkPlanes)
		walkPlanes->restore(access);
	else
		warning("Room_v2::restore(): the room has no sectors");

	return true;
}

void Room_v2::freeSetups() {
	if (numSetups > 0 && setups) {
		delete[] setups;
		setups = nullptr;
		numSetups = 0;
	} else {
		setups = nullptr;
		numSetups = 0;
	}
}

bool Room_v2::startup() {
	if (_started)
		return false;

	_currentHandle.setResource(nullptr);
	_started = true;

	return true;
}

void Room_v2::shutdown() {
	_started = false;
}

void Room_v2::saveCurrentRoom(tSaveRestoreAccess *access) {
	if (_currentRoom) {
		uint length = _currentRoom->name.size() + 1;

		access(&length, sizeof(length));
		access((void *)_currentRoom->name.c_str(), length);
		_currentRoom->save(access);
		Layer::saveRestoreNextId(access);
	} else {
		int noRoom = 0;

		access(&noRoom, sizeof(noRoom));
		Layer::saveRestoreNextId(access);
	}
}

void Room_v2::restoreCurrentRoom(tSaveRestoreAccess *access) {
	char buffer[kSavedRoomNameSize + 1];
	uint length = 0;

	access(&length, sizeof(length));

	if (length) {
		if (length > kSavedRoomNameSize) {
			warning("Room_v2::restoreCurrentRoom(): invalid room name length %u", length);

			return;
		}

		access(buffer, length);
		buffer[length] = 0;
		loadRoom(buffer);

		if (_currentRoom)
			_currentRoom->restore(access);
		else
			warning("Room_v2::restoreCurrentRoom(): the room could not be loaded");
	}

	Layer::saveRestoreNextId(access);
}

Layer *Room_v2::findLayerById(int layerId) {
	if (!_currentRoom || !_currentRoom->setups)
		return nullptr;

	for (int i = 0; i < _currentRoom->numSetups; i++) {
		Common::List<Layer *>::iterator it;

		for (it = _currentRoom->setups[i].layers.begin(); it != _currentRoom->setups[i].layers.end(); ++it) {
			if ((*it)->id == layerId)
				return *it;
		}
	}

	return nullptr;
}

void Room_v2::loadRoom(const char *roomName) {
	if (_currentRoom) {
		if (roomName && _currentRoom->name == roomName)
			return;

		_currentRoom->setLightsActive(false);
		_currentHandle.unlock();

		Common::List<Actor *>::iterator it;

		for (it = _currentRoom->actors.begin(); it != _currentRoom->actors.end(); ++it) {
			if (gpConsoleServices)
				gpConsoleServices->logError("Actor '%s' left in room.\n", (*it)->actorName.c_str());
			else
				warning("Actor '%s' left in room.", (*it)->actorName.c_str());
		}

		_currentHandle.setResource(nullptr);
		_currentRoom = nullptr;
		Sounds::setSoundListener(nullptr);
	}

	if (!roomName)
		return;

	_currentHandle.resource = gpResourceManager->findResource(roomName);

	if (!_currentHandle.resource) {
		_currentHandle.resource = gpResourceManager->createResource(roomName);

		if (!_currentHandle.resource)
			_currentHandle.allocateResource();

		if (_currentHandle.resource) {
			_currentHandle.resource->resSetName(roomName);
			gpResourceManager->addResource(_currentHandle.resource);
		}
	}

	_currentHandle.setResource(_currentHandle.resource);

	if (_currentHandle.lock())
		_currentRoom = _currentHandle.typed;
	else
		_currentRoom = nullptr;

	if (!_currentRoom) {
		warning("Room_v2::loadRoom(): could not load room '%s'", roomName);

		return;
	}

	_currentRoom->setSoundParameters(8.0f, 20, 127, 1.5f, 0.8f);
	_currentRoom->setLightsActive(true);

	if (InputRecorder::instance)
		InputRecorder::instance->recordRoom(roomName);
	else
		warning("Room_v2::loadRoom(): there is no input recorder");
}

Room_v2 *Room_v2::getCurrentRoom() {
	return _currentRoom;
}

const char *Room_v2::getCurrentRoomName() {
	if (!_currentRoom)
		return nullptr;

	return _currentRoom->name.c_str();
}

static const float kDegToRad = 3.14159274f * 0.00555555569f;
static const int kTokenSize = 256;
static const int kBinaryStringSize = 256;
static const int kMaxSetups = 4096;

char Room_v2::_lineBuffer[Room_v2::kLineBufferSize];

static void logLoaderError(const LString &message) {
	if (gpConsoleServices)
		gpConsoleServices->logError("%s", message.c_str());
	else
		warning("%s", message.c_str());
}

static void setNodeRotation(CNode *node, const CQuaternion &rotation) {
	node->quat = rotation;
	node->nodeFlags = (node->nodeFlags & ~(CNode::kEulerValid | CNode::kQuaternionValid | CNode::kLocalSet)) | CNode::kQuaternionValid | CNode::kLocalSet;
	node->updateLocalState();
}

static void setNodeEuler(CNode *node, const CVector &euler) {
	node->eulerDeg = euler;
	node->nodeFlags = (node->nodeFlags & ~CNode::kQuaternionValid) | CNode::kEulerValid | CNode::kLocalSet;
	node->updateLocalState();
}

static void setLightNodeName(CLightNode *lightNode, const char *source) {
	if (strlen(source) >= CNode::kNameSize)
		warning("Room_v2::setLightNodeName(): the light name is too long");

	memset(lightNode->name, 0, CNode::kNameSize);
	Common::strlcpy(lightNode->name, source, CNode::kNameSize);
}

static bool readBinaryBytes(LFile *file, void *buffer, uint size) {
	return file->read(buffer, size) == size;
}

static bool readBinaryInt(LFile *file, int *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_INT32(bytes);

	return true;
}

static bool readBinaryFloat(LFile *file, float *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_FLOAT32(bytes);

	return true;
}

static bool readBinaryVector(LFile *file, CVector *vector) {
	return readBinaryFloat(file, &vector->x) && readBinaryFloat(file, &vector->y) && readBinaryFloat(file, &vector->z);
}

static bool readBinaryQuaternion(LFile *file, CQuaternion *quaternion) {
	return readBinaryFloat(file, &quaternion->x) && readBinaryFloat(file, &quaternion->y) && readBinaryFloat(file, &quaternion->z) && readBinaryFloat(file, &quaternion->w);
}

static bool readBinaryColor(LFile *file, Color *color) {
	return readBinaryInt(file, &color->a) && readBinaryInt(file, &color->r) && readBinaryInt(file, &color->g) && readBinaryInt(file, &color->b);
}

static bool readBinaryString(LFile *file, char *buffer) {
	int length;

	if (!readBinaryInt(file, &length))
		return false;

	if (length < 0 || length > kBinaryStringSize) {
		warning("Room_v2::loadBinary(): invalid string length %d", length);

		return false;
	}

	if (!readBinaryBytes(file, buffer, length))
		return false;

	buffer[length] = 0;

	return true;
}

static bool failBinaryLoad(LFile *file, const char *reason) {
	warning("Room_v2::loadBinary(): %s", reason);
	gpFileServices->close(file, true);

	return false;
}

static void writeBinaryInt(LFile *file, int value) {
	byte bytes[4];

	WRITE_LE_INT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static void writeBinaryFloat(LFile *file, float value) {
	byte bytes[4];

	WRITE_LE_FLOAT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static void writeBinaryVector(LFile *file, const CVector &vector) {
	writeBinaryFloat(file, vector.x);
	writeBinaryFloat(file, vector.y);
	writeBinaryFloat(file, vector.z);
}

static void writeBinaryQuaternion(LFile *file, const CQuaternion &quaternion) {
	writeBinaryFloat(file, quaternion.x);
	writeBinaryFloat(file, quaternion.y);
	writeBinaryFloat(file, quaternion.z);
	writeBinaryFloat(file, quaternion.w);
}

static void writeBinaryColor(LFile *file, const Color &color) {
	writeBinaryInt(file, color.a);
	writeBinaryInt(file, color.r);
	writeBinaryInt(file, color.g);
	writeBinaryInt(file, color.b);
}

static void writeBinaryString(LFile *file, const LString &string) {
	writeBinaryInt(file, (int)string.size() + 1);
	file->write(string.c_str(), string.size() + 1);
}

bool Room_v2::loadText(const char *path) {
	char section[128];
	LFile *file = gpFileServices->open(path, "rt");

	if (!file || !file->isOpen()) {
		logLoaderError(LString::format("Error: Invalid load filename '%s'.\n", path));

		return false;
	}

	section[0] = 0;

	while (file->readLine(_lineBuffer, kLineBufferSize)) {
		if (sscanf(_lineBuffer, "section: %127s", section) == 1) {
			if (!parseSection(file, section)) {
				logLoaderError(LString::format("Load of file %s failed due to syntax errors.\n", path));
				gpFileServices->close(file, true);

				return false;
			}
		}
	}

	gpFileServices->close(file, true);

	return true;
}

bool Room_v2::parseSection(LFile *file, const char *sectionName) {
	bool parsed;

	if (scumm_stricmp("setups", sectionName) == 0) {
		parsed = parseSetups(file);
	} else if (scumm_stricmp("lights", sectionName) == 0) {
		parsed = parseLights(file);
	} else if (scumm_stricmp("shadows", sectionName) == 0) {
		parsed = parseShadows(file);
	} else if (scumm_stricmp("sectors", sectionName) == 0) {
		parsed = parseSectors(file);
	} else {
		logLoaderError(LString::format("Warning: Unknown Section '%s'.\n", sectionName));

		return true;
	}

	if (!parsed) {
		logLoaderError(LString::format("Error while parsing section %s.\n", sectionName));

		return false;
	}

	return true;
}

bool Room_v2::parseSetups(LFile *file) {
	char token[kTokenSize];
	char backgroundToken[Setup::kNameSize];
	CVector interest;
	int currentIndex = -1;

	token[0] = 0;
	backgroundToken[0] = 0;
	setups = nullptr;

	if (!file->readLine(_lineBuffer, kLineBufferSize))
		return true;

	while (true) {
		int count;

		if (sscanf(_lineBuffer, " section: %255s\n", token) != 0) {
			if (!parseSection(file, token))
				return false;
		} else if (sscanf(_lineBuffer, " numsetups %d\n", &count) == 1) {
			if (count < 0 || count > kMaxSetups) {
				warning("Room_v2::parseSetups(): invalid number of setups %d", count);

				return false;
			}

			numSetups = count;
			setups = new Setup[count];
			currentIndex = -1;
		} else {
			if (!setups)
				return false;

			if (sscanf(_lineBuffer, " setup %255s", token) == 1) {
				currentIndex++;

				if (currentIndex >= numSetups) {
					logLoaderError("Too many setups in file!\n");

					return false;
				}

				if (strlen(token) >= Setup::kNameSize)
					warning("Room_v2::parseSetups(): the setup name is too long");

				Common::strlcpy(setups[currentIndex].name, token, Setup::kNameSize);
			} else if (currentIndex < 0) {
				logLoaderError(LString::format("Error parsing setups: Expected 'setup <name>', but got '%s'.\n", _lineBuffer));

				return false;
			} else {
				Setup *setup = &setups[currentIndex];
				Camera *camera = &setup->camera;
				CVector vector;
				float roll;
				float x;
				float y;
				float z;
				float w;
				float fov;

				// The disasm reads the background name and then ignores it
				if (sscanf(_lineBuffer, " background %127s", backgroundToken) == 1) {
				} else if (sscanf(_lineBuffer, " tiledscene %127s", backgroundToken) == 1) {
					setup->backgroundName = backgroundToken;
				} else if (sscanf(_lineBuffer, " position %f %f %f", &x, &y, &z) == 3) {
					vector.x = x;
					vector.y = y;
					vector.z = z;
					camera->setLocalPosition(vector);
				} else if (sscanf(_lineBuffer, " interest %f %f %f", &x, &y, &z) == 3) {
					interest.x = x;
					interest.y = y;
					interest.z = z;
					camera->lookAt(interest);
				} else if (sscanf(_lineBuffer, " roll %f", &roll) == 1) {
					float angle = -(kDegToRad * roll);
					CVector axis;

					axis.x = 0.0f;
					axis.y = 0.0f;
					axis.z = 1.0f;

					CQuaternion rollRotation(axis, angle);

					if ((camera->nodeFlags & CNode::kQuaternionValid) == 0)
						camera->updateLocalQuaternion();

					const CQuaternion &current = camera->quat;
					float newW = (float)((double)rollRotation.w * current.w - (double)rollRotation.x * current.x - (double)rollRotation.y * current.y - (double)rollRotation.z * current.z);
					float newZ = (float)((double)rollRotation.w * current.z + (double)rollRotation.z * current.w + (double)rollRotation.y * current.x - (double)rollRotation.x * current.y);
					float newY = (float)((double)rollRotation.x * current.z + (double)rollRotation.y * current.w + (double)rollRotation.w * current.y - (double)rollRotation.z * current.x);
					float newX = (float)((double)rollRotation.z * current.y + (double)rollRotation.x * current.w + (double)rollRotation.w * current.x - (double)rollRotation.y * current.z);

					setNodeRotation(camera, CQuaternion(newX, newY, newZ, newW));
				} else if (sscanf(_lineBuffer, " direction %f %f %f %f", &x, &y, &z, &w) == 4) {
					setNodeRotation(camera, CQuaternion(x, y, z, w));
				} else if (sscanf(_lineBuffer, " fov %f", &fov) == 1) {
					IRect viewport;

					viewport.x = 0;
					viewport.y = 0;
					viewport.w = 640;
					viewport.h = 480;
					camera->setViewport(&viewport, fov);
				} else if (sscanf(_lineBuffer, " nclip %f", &camera->nearClip) == 1) {
				} else if (sscanf(_lineBuffer, " fclip %f", &camera->farClip) == 1) {
				} else {
					logLoaderError(LString::format("Unrecognized line in set file: '%s'\n", _lineBuffer));

					return false;
				}
			}
		}

		if (!file->readLine(_lineBuffer, kLineBufferSize))
			return true;
	}
}

bool Room_v2::parseLights(LFile *file) {
	char token[kTokenSize];
	CLightNode *lightNode = nullptr;
	CLight *light = nullptr;
	float x;
	float y;
	float z;
	float w;
	int red;
	int green;
	int blue;

	token[0] = 0;

	if (!file->readLine(_lineBuffer, kLineBufferSize))
		return true;

	while (true) {
		if (sscanf(_lineBuffer, " section: %255s\n", token) != 0) {
			if (!parseSection(file, token))
				return false;
		} else if (sscanf(_lineBuffer, " light %255s", token) == 1) {
			lightNode = new CLightNode();
			addLight(lightNode);
			setLightNodeName(lightNode, token);
			light = new CLight(Color(1u), 1.0f, 10.0f, 10.0f);
			lightNode->setLight(light);
			lightNode->setActive(true);
		} else if (!lightNode || !light) {
			warning("Room_v2::parseLights(): the line '%s' comes before any light", _lineBuffer);
		} else if (sscanf(_lineBuffer, " type %255s", token) == 1) {
			if (strcmp(token, "omni") == 0)
				light->type = CLight::kTypeOmni;
			else if (strcmp(token, "spot") == 0)
				light->type = CLight::kTypeSpot;
			else if (strcmp(token, "direct") == 0)
				light->type = CLight::kTypeDirect;
			else
				light->type = CLight::kTypeAmbient;
		} else if (sscanf(_lineBuffer, " position %f %f %f", &x, &y, &z) == 3) {
			CVector position;

			position.x = x;
			position.y = y;
			position.z = z;
			lightNode->setLocalPosition(position);
		} else if (sscanf(_lineBuffer, " direction %f %f %f", &x, &y, &z) == 3) {
			CVector euler;

			euler.x = x;
			euler.y = y;
			euler.z = z;
			setNodeEuler(lightNode, euler);
		} else if (sscanf(_lineBuffer, " orientation %f %f %f %f", &x, &y, &z, &w) == 4) {
			setNodeRotation(lightNode, CQuaternion(x, y, z, w));
		} else if (sscanf(_lineBuffer, " intensity %f", &light->intensity) == 1) {
		} else if (sscanf(_lineBuffer, " umbraangle %f", &x) == 1) {
			light->umbraAngle = kDegToRad * x;
		} else if (sscanf(_lineBuffer, " penumbraangle %f", &x) == 1) {
			light->penumbraAngle = kDegToRad * x;
		} else if (sscanf(_lineBuffer, " color %d %d %d", &red, &green, &blue) == 3) {
			light->setColor(red, green, blue);
		} else if (sscanf(_lineBuffer, " focusdistance %f", &light->focusDistance) != 1) {
			sscanf(_lineBuffer, " spreaddistance %f", &light->spreadDistance);
		}

		if (!file->readLine(_lineBuffer, kLineBufferSize))
			return true;
	}
}

bool Room_v2::parseShadows(LFile *file) {
	char token[kTokenSize];
	Shadow_v2 *shadow = nullptr;
	float x;
	float y;
	float z;
	int red;
	int green;
	int blue;

	token[0] = 0;

	if (!file->readLine(_lineBuffer, kLineBufferSize))
		return true;

	while (true) {
		if (sscanf(_lineBuffer, " section: %255s\n", token) != 0) {
			if (!parseSection(file, token))
				return false;
		} else if (sscanf(_lineBuffer, " shadow %255s", token) == 1) {
			shadow = newShadow();
			shadow->setName(LString(token));
		} else if (!shadow) {
			warning("Room_v2::parseShadows(): the line '%s' comes before any shadow", _lineBuffer);
		} else if (sscanf(_lineBuffer, " light %255s", token) == 1) {
			shadow->setLightName(LString(token));
		} else if (sscanf(_lineBuffer, " point %f %f %f", &x, &y, &z) == 3) {
			shadow->point[0] = x;
			shadow->point[1] = y;
			shadow->point[2] = z;
		} else if (sscanf(_lineBuffer, " color %d %d %d", &red, &green, &blue) == 3) {
			Color shadowColor;

			shadowColor.setRGB(red, green, blue);
			shadow->setColor(shadowColor);
		} else if (sscanf(_lineBuffer, " sector %255s", token) == 1) {
			shadow->addSector(LString(token));
		}

		if (!file->readLine(_lineBuffer, kLineBufferSize))
			return true;
	}
}

bool Room_v2::parseSectors(LFile *file) {
	walkPlanes = new WalkPlaneCollection();

	char *nextSection = walkPlanes->parseCollection(file);

	// The disasm ignores the result of parsing the following section
	if (nextSection)
		parseSection(file, nextSection);

	walkPlanes->buildNeighbors();

	return true;
}

bool Room_v2::loadBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "rb");

	if (!file)
		return false;

	char buffer[kBinaryStringSize + 1];
	int count;

	if (!readBinaryInt(file, &count))
		return failBinaryLoad(file, "unexpected end of file");

	if (count < 0 || count > kMaxSetups)
		return failBinaryLoad(file, "invalid number of setups");

	numSetups = count;
	setups = new Setup[count];

	for (int i = 0; i < numSetups; i++) {
		Setup *setup = &setups[i];
		Camera *camera = &setup->camera;
		CVector position;
		CQuaternion rotation;
		float fov;
		IRect viewport;

		if (!readBinaryBytes(file, setup->name, Setup::kNameSize) || !readBinaryString(file, buffer))
			return failBinaryLoad(file, "invalid setup");

		if (!memchr(setup->name, 0, Setup::kNameSize)) {
			warning("Room_v2::loadBinary(): the setup name is not terminated");
			setup->name[Setup::kNameSize - 1] = 0;
		}

		setup->backgroundName = buffer;

		if (!readBinaryVector(file, &position))
			return failBinaryLoad(file, "invalid setup position");

		camera->setLocalPosition(position);

		if (!readBinaryQuaternion(file, &rotation))
			return failBinaryLoad(file, "invalid setup rotation");

		setNodeRotation(camera, rotation);

		if (!readBinaryFloat(file, &fov))
			return failBinaryLoad(file, "invalid setup field of view");

		viewport.x = 0;
		viewport.y = 0;
		viewport.w = 640;
		viewport.h = 480;
		camera->setViewport(&viewport, fov);

		if (!readBinaryFloat(file, &camera->nearClip) || !readBinaryFloat(file, &camera->farClip))
			return failBinaryLoad(file, "invalid setup clipping planes");
	}

	if (!readBinaryInt(file, &count))
		return failBinaryLoad(file, "unexpected end of file");

	for (int i = 0; i < count; i++) {
		CLightNode *lightNode = new CLightNode();
		CVector position;
		CQuaternion rotation;

		addLight(lightNode);

		if (!readBinaryBytes(file, lightNode->name, CNode::kNameSize))
			return failBinaryLoad(file, "invalid light name");

		if (!memchr(lightNode->name, 0, CNode::kNameSize)) {
			warning("Room_v2::loadBinary(): the light name is not terminated");
			lightNode->name[CNode::kNameSize - 1] = 0;
		}

		CLight *light = new CLight(Color(1u), 1.0f, 10.0f, 10.0f);

		lightNode->setLight(light);
		lightNode->setActive(true);

		if (!readBinaryVector(file, &position))
			return failBinaryLoad(file, "invalid light position");

		lightNode->setLocalPosition(position);

		if (!readBinaryQuaternion(file, &rotation))
			return failBinaryLoad(file, "invalid light rotation");

		setNodeRotation(lightNode, rotation);

		if (!readBinaryInt(file, &light->type) || !readBinaryFloat(file, &light->intensity) || !readBinaryColor(file, &light->color) ||
			!readBinaryFloat(file, &light->focusDistance) || !readBinaryFloat(file, &light->spreadDistance) ||
			!readBinaryFloat(file, &light->umbraAngle) || !readBinaryFloat(file, &light->penumbraAngle))
			return failBinaryLoad(file, "invalid light");
	}

	if (!readBinaryInt(file, &count))
		return failBinaryLoad(file, "unexpected end of file");

	if (count > 0) {
		float vertices[WalkPlane_v2::kMaxVertices * 3];

		walkPlanes = new WalkPlaneCollection();
		memset(vertices, 0, sizeof(vertices));

		for (int i = 0; i < count; i++) {
			int vertexCount;

			if (!readBinaryInt(file, &vertexCount))
				return failBinaryLoad(file, "unexpected end of file");

			if (vertexCount < 2 || vertexCount > WalkPlane_v2::kMaxVertices)
				return failBinaryLoad(file, "invalid number of sector vertices");

			for (int j = 0; j < vertexCount * 3; j++) {
				if (!readBinaryFloat(file, &vertices[j]))
					return failBinaryLoad(file, "invalid sector vertices");
			}

			WalkPlane_v2 *plane = new WalkPlane_v2(vertices, vertexCount, walkPlanes);
			int planeType;
			byte activeByte;
			float tolerance;

			walkPlanes->planes.push_back(plane);

			if (!readBinaryString(file, buffer))
				return failBinaryLoad(file, "invalid sector name");

			plane->name = buffer;

			if (!readBinaryInt(file, &plane->number) || !readBinaryBytes(file, &activeByte, 1) || !readBinaryInt(file, &planeType))
				return failBinaryLoad(file, "invalid sector");

			plane->active = activeByte != 0;
			plane->setType(planeType);

			if (!readBinaryInt(file, &plane->numSortOrders))
				return failBinaryLoad(file, "invalid sector sort orders");

			if (plane->numSortOrders < 0 || plane->numSortOrders > WalkPlane_v2::kMaxSortOrders) {
				plane->numSortOrders = 0;

				return failBinaryLoad(file, "invalid number of sector sort orders");
			}

			for (int j = 0; j < plane->numSortOrders; j++) {
				if (!readBinaryInt(file, &plane->sortOrder[j]))
					return failBinaryLoad(file, "invalid sector sort orders");
			}

			if (!readBinaryFloat(file, &tolerance))
				return failBinaryLoad(file, "invalid sector tolerance");

			if (!(tolerance < 0.0f || tolerance > 0.0f))
				plane->tolerance = 0.005f;
			else
				plane->tolerance = tolerance;
		}

		walkPlanes->buildNeighbors();
	}

	if (!readBinaryInt(file, &count))
		return failBinaryLoad(file, "unexpected end of file");

	for (int i = 0; i < count; i++) {
		Shadow_v2 *shadow = newShadow();
		int sectorCount;

		if (!readBinaryString(file, buffer))
			return failBinaryLoad(file, "invalid shadow name");

		shadow->name = buffer;

		if (!readBinaryString(file, buffer))
			return failBinaryLoad(file, "invalid shadow light name");

		shadow->lightName = buffer;

		if (!readBinaryFloat(file, &shadow->point[0]) || !readBinaryFloat(file, &shadow->point[1]) || !readBinaryFloat(file, &shadow->point[2]) ||
			!readBinaryInt(file, &sectorCount))
			return failBinaryLoad(file, "invalid shadow");

		for (int j = 0; j < sectorCount; j++) {
			WalkPlane_v2 *sector = nullptr;

			if (!readBinaryString(file, buffer))
				return failBinaryLoad(file, "invalid shadow sector name");

			if (walkPlanes)
				sector = walkPlanes->planeNamed(LString(buffer));
			else
				warning("Room_v2::loadBinary(): the shadow sector '%s' has no sectors to be found in", buffer);

			shadow->sectors.push_back(sector);
		}

		if (!readBinaryColor(file, &shadow->color))
			return failBinaryLoad(file, "invalid shadow color");
	}

	gpFileServices->close(file, true);

	return true;
}

bool Room_v2::saveBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "wb");

	if (!file)
		return false;

	writeBinaryInt(file, numSetups);

	for (int i = 0; i < numSetups; i++) {
		Camera *camera = &setups[i].camera;

		file->write(setups[i].name, Setup::kNameSize);
		writeBinaryString(file, setups[i].backgroundName);

		if ((camera->nodeFlags & CNode::kWorldValid) == 0)
			camera->updateWorldTransform();

		writeBinaryVector(file, camera->world.pos);
		writeBinaryQuaternion(file, camera->getWorldOrientation());
		writeBinaryFloat(file, camera->fovX);
		writeBinaryFloat(file, camera->nearClip);
		writeBinaryFloat(file, camera->farClip);
	}

	writeBinaryInt(file, (int)lights.size());

	Common::List<CLightNode *>::iterator lightIt;

	for (lightIt = lights.begin(); lightIt != lights.end(); ++lightIt) {
		CLightNode *lightNode = *lightIt;
		CLight defaultLight(Color(1u), 1.0f, 10.0f, 10.0f);
		CLight *light = lightNode->light;

		if (!light) {
			warning("Room_v2::saveBinary(): a light node has no light");
			light = &defaultLight;
		}

		file->write(lightNode->name, CNode::kNameSize);

		if ((lightNode->nodeFlags & CNode::kWorldValid) == 0)
			lightNode->updateWorldTransform();

		writeBinaryVector(file, lightNode->world.pos);
		writeBinaryQuaternion(file, lightNode->getWorldOrientation());
		writeBinaryInt(file, light->type);
		writeBinaryFloat(file, light->intensity);
		writeBinaryColor(file, light->color);
		writeBinaryFloat(file, light->focusDistance);
		writeBinaryFloat(file, light->spreadDistance);
		writeBinaryFloat(file, light->umbraAngle);
		writeBinaryFloat(file, light->penumbraAngle);
	}

	int planeCount = 0;

	if (walkPlanes)
		planeCount = (int)walkPlanes->planes.size();

	writeBinaryInt(file, planeCount);

	if (planeCount > 0) {
		Common::List<WalkPlane_v2 *>::iterator planeIt;

		for (planeIt = walkPlanes->planes.begin(); planeIt != walkPlanes->planes.end(); ++planeIt) {
			WalkPlane_v2 *plane = *planeIt;

			if (plane->numVerts < 0 || plane->numVerts > WalkPlane_v2::kMaxVertices) {
				warning("Room_v2::saveBinary(): invalid number of sector vertices");
				plane->numVerts = 0;
			}

			if (plane->numSortOrders < 0 || plane->numSortOrders > WalkPlane_v2::kMaxSortOrders) {
				warning("Room_v2::saveBinary(): invalid number of sector sort orders");
				plane->numSortOrders = 0;
			}

			writeBinaryInt(file, plane->numVerts);

			for (int i = 0; i < plane->numVerts; i++) {
				if (plane->verts[i]) {
					writeBinaryFloat(file, plane->verts[i]->x);
					writeBinaryFloat(file, plane->verts[i]->y);
					writeBinaryFloat(file, plane->verts[i]->z);
				} else {
					warning("Room_v2::saveBinary(): missing sector vertex");
					writeBinaryFloat(file, 0.0f);
					writeBinaryFloat(file, 0.0f);
					writeBinaryFloat(file, 0.0f);
				}
			}

			writeBinaryString(file, plane->name);
			writeBinaryInt(file, plane->number);

			byte activeByte = plane->active ? 1 : 0;

			file->write(&activeByte, 1);
			writeBinaryInt(file, plane->type);
			writeBinaryInt(file, plane->numSortOrders);

			for (int i = 0; i < plane->numSortOrders; i++)
				writeBinaryInt(file, plane->sortOrder[i]);

			writeBinaryFloat(file, plane->tolerance);
		}
	}

	writeBinaryInt(file, (int)shadows.size());

	Common::List<Shadow_v2 *>::iterator shadowIt;

	for (shadowIt = shadows.begin(); shadowIt != shadows.end(); ++shadowIt) {
		Shadow_v2 *shadow = *shadowIt;

		writeBinaryString(file, shadow->name);
		writeBinaryString(file, shadow->lightName);
		writeBinaryFloat(file, shadow->point[0]);
		writeBinaryFloat(file, shadow->point[1]);
		writeBinaryFloat(file, shadow->point[2]);
		writeBinaryInt(file, (int)shadow->sectors.size());

		for (uint i = 0; i < shadow->sectors.size(); i++) {
			if (shadow->sectors[i]) {
				writeBinaryString(file, shadow->sectors[i]->name);
			} else {
				warning("Room_v2::saveBinary(): unresolved shadow sector");
				writeBinaryString(file, LString());
			}
		}

		writeBinaryColor(file, shadow->color);
	}

	gpFileServices->close(file, true);

	return true;
}

} // End of namespace Grim
