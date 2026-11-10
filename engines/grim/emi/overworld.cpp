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

#include "common/scummsys.h"
#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/clight.h"
#include "engines/grim/emi/clightnode.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/cvector.h"
#include "engines/grim/emi/overworld.h"

namespace Grim {

static const float kProjectionNearClip = 0.05f;
static const float kProjectionFarClip = 20.0f;

Overworld *Overworld::_instance = nullptr;

Overworld *Overworld::getInstance() {
	if (!_instance)
		_instance = new Overworld();

	return _instance;
}

Overworld::Overworld() {
	enabled = true;

	CLightNode *ambientNode = new CLightNode();

	addLight(ambientNode);
	memset(ambientNode->name, 0, sizeof(ambientNode->name));
	Common::strlcpy(ambientNode->name, "ambient", CNode::kNameSize);

	CLight *ambientLight = new CLight(Color(4u), 1.0f, 10.0f, 10.0f);

	ambientLight->intensity = 0.4f;
	ambientLight->setColor(255, 255, 255);
	ambientNode->setLight(ambientLight);

	CLightNode *pointNode = new CLightNode();

	addLight(pointNode);
	memset(pointNode->name, 0, sizeof(pointNode->name));
	Common::strlcpy(pointNode->name, "point1", CNode::kNameSize);

	CVector pointPosition;

	pointPosition.x = -1.0f;
	pointPosition.y = 0.7f;
	pointPosition.z = 0.0f;
	pointNode->setLocalPosition(pointPosition);

	CLight *pointLight = new CLight(Color(1u), 1.0f, 2.0f, 4.0f);

	pointLight->intensity = 1.0f;
	pointLight->setColor(255, 255, 255);
	pointNode->setLight(pointLight);
}

Overworld::~Overworld() {
	while (!lights.empty()) {
		CLightNode *node = lights.front();

		if (!node) {
			warning("Overworld::~Overworld(): null light in the list");
			lights.pop_front();

			continue;
		}

		removeLight(node);
		delete node->light;
		delete node;
	}

	actors.clear();
}

int Overworld::draw(CDrawContext *drawContext) {
	if (!enabled)
		return 0;

	Camera camera;

	camera.setLocalPosition(CVector());
	camera.nearClip = kProjectionNearClip;
	camera.farClip = kProjectionFarClip;
	drawContext->clear(false, true);

	Common::List<Actor *>::iterator it;

	for (it = actors.begin(); it != actors.end(); ++it) {
		Actor *actor = *it;

		if (actor) {
			camera.applyProjection();
			actor->onScreen = true;
			actor->drawActor(&camera);
		}
	}

	return 0;
}

void Overworld::putActor(Actor *actor) {
	if (!actor) {
		warning("Overworld::putActor(): missing actor");

		return;
	}

	int occurrences = 0;
	Common::List<Actor *>::iterator it;

	for (it = actors.begin(); it != actors.end(); ++it) {
		if (*it == actor)
			occurrences++;
	}

	if (occurrences == 0) {
		actors.push_back(actor);
		actor->inOverworld = true;
	}
}

int Overworld::removeActor(Actor *actor) {
	if (!actor) {
		warning("Overworld::removeActor(): missing actor");

		return 0;
	}

	int occurrences = 0;
	Common::List<Actor *>::iterator it;

	for (it = actors.begin(); it != actors.end(); ++it) {
		if (*it == actor)
			occurrences++;
	}

	it = actors.begin();

	while (it != actors.end()) {
		if (*it == actor)
			it = actors.erase(it);
		else
			++it;
	}

	actor->inOverworld = false;

	return occurrences;
}

int Overworld::clearActors() {
	int count = (int)actors.size();
	Common::List<Actor *>::iterator it;

	for (it = actors.begin(); it != actors.end(); ++it) {
		if (*it)
			(*it)->inOverworld = false;
		else
			warning("Overworld::clearActors(): null actor in the list");
	}

	actors.clear();

	return count;
}

void Overworld::addLight(CLightNode *light) {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it == light)
			return;
	}

	lights.push_back(light);
}

void Overworld::removeLight(CLightNode *light) {
	if (!light) {
		warning("Overworld::removeLight(): missing light");

		return;
	}

	light->setActive(false);

	Common::List<CLightNode *>::iterator it = lights.begin();

	while (it != lights.end()) {
		if (*it == light)
			it = lights.erase(it);
		else
			++it;
	}
}

void Overworld::setLightsActive(bool active) {
	Common::List<CLightNode *>::iterator it;

	for (it = lights.begin(); it != lights.end(); ++it) {
		if (*it)
			(*it)->setActive(active);
	}
}

} // End of namespace Grim
