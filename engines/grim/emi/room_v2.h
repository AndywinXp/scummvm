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

#ifndef GRIM_EMI_ROOM_V2_H
#define GRIM_EMI_ROOM_V2_H

#include "common/list.h"

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/clightnode.h"
#include "engines/grim/emi/layer.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/setup.h"
#include "engines/grim/emi/shadow_v2.h"
#include "engines/grim/emi/walkplanecollection.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class Actor;
class CDrawContext;

class Room_v2 : public LResource {
public:
	Room_v2();
	virtual ~Room_v2();

	virtual uint resSizeOfItem();

	void setSetup(int setupIndex);
	Camera *getSetupCamera(int setupIndex);
	int draw(CDrawContext *drawContext);
	void addActor(Actor *actor);
	int removeActor(Actor *actor);
	void setLightsActive(bool active);
	void saveLightStates();
	void restoreLightStates();
	CLightNode *findLight(const char *lightName);
	void addLight(CLightNode *light);
	void removeLight(CLightNode *light);
	Shadow_v2 *findShadow(const LString &shadowName);
	Shadow_v2 *newShadow();
	void removeShadow(Shadow_v2 *shadow);
	void setSoundParameters(float newMaxDist, char newMinVol, char newMaxVol, float newPanFactor, float newMaxRadius);
	bool save(tSaveRestoreAccess *access);
	bool restore(tSaveRestoreAccess *access);
	void freeSetups();
	bool loadText(const char *path);
	bool parseSection(LFile *file, const char *sectionName);
	bool parseSetups(LFile *file);
	bool parseLights(LFile *file);
	bool parseShadows(LFile *file);
	bool parseSectors(LFile *file);
	bool loadBinary(const char *path);
	bool saveBinary(const char *path);

	static bool startup();
	static void shutdown();
	static void saveCurrentRoom(tSaveRestoreAccess *access);
	static void restoreCurrentRoom(tSaveRestoreAccess *access);
	static Layer *findLayerById(int layerId);
	static void loadRoom(const char *roomName);
	static Room_v2 *getCurrentRoom();
	static const char *getCurrentRoomName();

	Setup *setups;
	int numSetups;
	int currentSetup;
	Common::List<Shadow_v2 *> shadows;
	Common::List<CLightNode *> lights;
	Common::List<Actor *> actors;
	WalkPlaneCollection *walkPlanes;
	char minVol;
	char maxVol;
	float maxDist;
	float panFactor;
	float maxRadius;

protected:
	virtual bool resLoadItem(const char *itemName);

private:
	static bool _started;
	static Room_v2 *_currentRoom;
	static LResourceHandle<Room_v2> _currentHandle;
	static Room_v2 *_lastDrawnRoom;
	static int _lastDrawnSetup;

	enum {
		kLineBufferSize = 256
	};

	static char _lineBuffer[kLineBufferSize];
};

} // End of namespace Grim

#endif
