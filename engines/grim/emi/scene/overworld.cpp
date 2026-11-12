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

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/scene/camera.h"
#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/scene/clight.h"
#include "engines/grim/emi/scene/clightnode.h"
#include "engines/grim/emi/graphics/color.h"
#include "engines/grim/emi/math/cvector.h"
#include "engines/grim/emi/scene/overworld.h"

namespace Grim {

static const float kProjectionNearClip = 0.05f;
static const float kProjectionFarClip = 20.0f;

Overworld::Overworld(EmiEngine *engine) : _engine(engine) {
	_enabled = true;

	CLightNode *ambientNode = new CLightNode(_engine);

	addLight(ambientNode);
	memset(ambientNode->_name, 0, sizeof(ambientNode->_name));
	Common::strlcpy(ambientNode->_name, "ambient", CNode::kNameSize);

	CLight *ambientLight = new CLight(Color(4u), 1.0f, 10.0f, 10.0f);

	ambientLight->_intensity = 0.4f;
	ambientLight->setColor(255, 255, 255);
	ambientNode->setLight(ambientLight);

	CLightNode *pointNode = new CLightNode(_engine);

	addLight(pointNode);
	memset(pointNode->_name, 0, sizeof(pointNode->_name));
	Common::strlcpy(pointNode->_name, "point1", CNode::kNameSize);

	CVector pointPosition;

	pointPosition._x = -1.0f;
	pointPosition._y = 0.7f;
	pointPosition._z = 0.0f;
	pointNode->setLocalPosition(pointPosition);

	CLight *pointLight = new CLight(Color(1u), 1.0f, 2.0f, 4.0f);

	pointLight->_intensity = 1.0f;
	pointLight->setColor(255, 255, 255);
	pointNode->setLight(pointLight);
}

Overworld::~Overworld() {
	while (!_lights.empty()) {
		CLightNode *node = _lights.front();

		if (!node) {
			warning("Overworld::~Overworld(): null light in the list");
			_lights.pop_front();

			continue;
		}

		removeLight(node);
		delete node->_light;
		delete node;
	}

	_actors.clear();
}

int Overworld::draw(CDrawContext *drawContext) {
	if (!_enabled)
		return 0;

	Camera camera(drawContext);

	camera.setLocalPosition(CVector());
	camera._nearClip = kProjectionNearClip;
	camera._farClip = kProjectionFarClip;
	drawContext->clear(false, true);

	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		Actor *actor = *it;

		if (actor) {
			camera.applyProjection();
			actor->_onScreen = true;
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

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		if (*it == actor)
			occurrences++;
	}

	if (occurrences == 0) {
		_actors.push_back(actor);
		actor->_inOverworld = true;
	}
}

int Overworld::removeActor(Actor *actor) {
	if (!actor) {
		warning("Overworld::removeActor(): missing actor");

		return 0;
	}

	int occurrences = 0;
	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		if (*it == actor)
			occurrences++;
	}

	it = _actors.begin();

	while (it != _actors.end()) {
		if (*it == actor)
			it = _actors.erase(it);
		else
			++it;
	}

	actor->_inOverworld = false;

	return occurrences;
}

int Overworld::clearActors() {
	int count = (int)_actors.size();
	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		if (*it)
			(*it)->_inOverworld = false;
		else
			warning("Overworld::clearActors(): null actor in the list");
	}

	_actors.clear();

	return count;
}

void Overworld::addLight(CLightNode *light) {
	Common::List<CLightNode *>::iterator it;

	for (it = _lights.begin(); it != _lights.end(); ++it) {
		if (*it == light)
			return;
	}

	_lights.push_back(light);
}

void Overworld::removeLight(CLightNode *light) {
	if (!light) {
		warning("Overworld::removeLight(): missing light");

		return;
	}

	light->setActive(false);

	Common::List<CLightNode *>::iterator it = _lights.begin();

	while (it != _lights.end()) {
		if (*it == light)
			it = _lights.erase(it);
		else
			++it;
	}
}

void Overworld::setLightsActive(bool active) {
	Common::List<CLightNode *>::iterator it;

	for (it = _lights.begin(); it != _lights.end(); ++it) {
		if (*it)
			(*it)->setActive(active);
	}
}

} // End of namespace Grim
