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

#include "common/textconsole.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/irect.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/setup.h"
#include "engines/grim/emi/tiledscene.h"

namespace Grim {

static const int kViewportWidth = 640;
static const int kViewportHeight = 480;
static const float kViewportFov = 90.0f;
static const int kLayerSortStep = 10;

Setup::Setup(EmiEngine *engine) : camera(engine->getDrawContext()), _engine(engine) {
	name[0] = 0;
	layersBuilt = false;

	IRect viewport;

	viewport.x = 0;
	viewport.y = 0;
	viewport.w = kViewportWidth;
	viewport.h = kViewportHeight;
	camera.setViewport(&viewport, kViewportFov);
}

Setup::~Setup() {
	while (!layers.empty())
		removeLayer(layers.front());

	backgroundName.clear();
	layers.clear();
}

void *Setup::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void Setup::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void *Setup::operator new(size_t, void *where) {
	return where;
}

void Setup::operator delete(void *, void *) {
}

void *Setup::operator new[](size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void Setup::operator delete[](void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

Setup *Setup::newArray(EmiEngine *engine, int count) {
	byte *block = static_cast<byte *>(Setup::operator new[](kArrayHeaderSize + (size_t)count * sizeof(Setup)));
	Setup *array = reinterpret_cast<Setup *>(block + kArrayHeaderSize);

	*reinterpret_cast<int *>(block) = count;

	for (int i = 0; i < count; i++)
		new (&array[i]) Setup(engine);

	return array;
}

void Setup::deleteArray(Setup *array) {
	byte *block = reinterpret_cast<byte *>(array) - kArrayHeaderSize;
	int count = *reinterpret_cast<int *>(block);

	for (int i = count; i > 0; i--)
		array[i - 1].~Setup();

	Setup::operator delete[](block);
}

void Setup::buildLayers() {
	if (layersBuilt)
		return;

	Layer *firstLayer = new Layer(_engine, backgroundName.c_str(), 0, 0, -1);
	TiledScene *scene = nullptr;

	if (firstLayer->tiledScene.lock())
		scene = firstLayer->tiledScene.typed;

	if (scene) {
		int frame = 0;
		int layerSortOrder = 0;

		while (frame < scene->getFrameCount()) {
			addLayer(backgroundName.c_str(), layerSortOrder, frame, -1);
			frame++;
			layerSortOrder += kLayerSortStep;
		}
	} else {
		warning("Setup::buildLayers(): the tiled scene is not available");
	}

	firstLayer->tiledScene.unlock();
	delete firstLayer;
	layersBuilt = true;
}

int Setup::getSize() {
	return (int)(sizeof(Setup) + layers.size() * sizeof(Layer *));
}

int Setup::addLayer(const char *layerName, int layerSortOrder, int layerFrame, int layerId) {
	Layer *layer = new Layer(_engine, layerName, layerSortOrder, layerFrame, layerId);

	layers.push_back(layer);

	return layer->id;
}

void Setup::removeLayer(Layer *layer) {
	Common::List<Layer *>::iterator it = layers.begin();

	while (it != layers.end()) {
		if (*it == layer)
			it = layers.erase(it);
		else
			++it;
	}

	if (layer) {
		delete layer;
		layer = nullptr;
	}
}

void Setup::removeLayerById(int layerId) {
	Common::List<Layer *>::iterator it;

	for (it = layers.begin(); it != layers.end(); ++it) {
		if ((*it)->id == layerId) {
			removeLayer(*it);

			return;
		}
	}
}

bool Setup::save(tSaveRestoreAccess *access) {
	access(&layersBuilt, sizeof(layersBuilt));

	int count = (int)layers.size();

	access(&count, sizeof(count));

	Common::List<Layer *>::iterator it;

	for (it = layers.begin(); it != layers.end(); ++it) {
		Layer *layer = *it;
		int value = layer->id;

		access(&value, sizeof(value));
		value = layer->sortOrder;
		access(&value, sizeof(value));
		value = layer->frame;
		access(&value, sizeof(value));

		int length = (int)strlen(layer->name) + 1;

		access(&length, sizeof(length));
		access(layer->name, length);
	}

	return true;
}

bool Setup::restore(tSaveRestoreAccess *access) {
	Common::List<Layer *>::iterator it;

	for (it = layers.begin(); it != layers.end(); ++it) {
		delete *it;
		*it = nullptr;
	}

	layers.clear();

	byte built = 0;
	int count = 0;

	access(&built, sizeof(built));
	layersBuilt = built != 0;
	access(&count, sizeof(count));

	for (int i = 0; i < count; i++) {
		char layerName[kNameSize];
		int layerId = 0;
		int layerSortOrder = 0;
		int layerFrame = 0;
		int length = 0;

		access(&layerId, sizeof(layerId));
		access(&layerSortOrder, sizeof(layerSortOrder));
		access(&layerFrame, sizeof(layerFrame));
		access(&length, sizeof(length));

		layerName[0] = 0;

		if (length > kNameSize) {
			warning("Setup::restore(): the layer name is too long");
			access(layerName, kNameSize);

			for (int skipped = kNameSize; skipped < length; skipped++) {
				char discard;

				access(&discard, sizeof(discard));
			}
		} else if (length > 0) {
			access(layerName, length);
		}

		layerName[kNameSize - 1] = 0;
		addLayer(layerName, layerSortOrder, layerFrame, layerId);
	}

	return true;
}

} // End of namespace Grim
