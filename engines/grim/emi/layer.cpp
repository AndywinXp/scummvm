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

#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/layer.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/profilertimer.h"

namespace Grim {

static const int kQuadsFlags = 0x10001;

static ProfilerTimer layerDrawTimer("Layer::Draw()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);

bool Layer::fix16Bit = true;
int Layer::_nextId = 1;

void Layer::set16BitFix(bool enable) {
	fix16Bit = enable;
}

void Layer::saveRestoreNextId(tSaveRestoreAccess *access) {
	access(&_nextId, sizeof(_nextId));
}

Layer::Layer(const char *layerName, int layerSortOrder, int layerFrame, int layerId) {
	if (layerId == -1) {
		id = _nextId;
		_nextId++;
	} else {
		id = layerId;
	}

	frame = 0;
	sortOrder = 0;

	if (strlen(layerName) >= kNameSize)
		warning("Layer::Layer(): the name is too long");

	Common::strlcpy(name, layerName, kNameSize);
	sortOrder = layerSortOrder;
	frame = layerFrame;

	LString resourceName = LString::format("BG%s", layerName);

	background.resource = gpResourceManager->findResource(resourceName.c_str());

	if (!background.resource) {
		background.resource = gpResourceManager->createResource(resourceName.c_str());

		if (!background.resource)
			background.allocateResource();

		if (background.resource) {
			background.resource->resSetName(resourceName.c_str());
			gpResourceManager->addResource(background.resource);
		}
	}

	background.setResource(background.resource);

	resourceName = LString::format("TS%s", layerName);

	tiledScene.resource = gpResourceManager->findResource(resourceName.c_str());

	if (!tiledScene.resource) {
		tiledScene.resource = gpResourceManager->createResource(resourceName.c_str());

		if (!tiledScene.resource)
			tiledScene.allocateResource();

		if (tiledScene.resource) {
			tiledScene.resource->resSetName(resourceName.c_str());
			gpResourceManager->addResource(tiledScene.resource);
		}
	}

	tiledScene.setResource(tiledScene.resource);
}

Layer::~Layer() {
}

void *Layer::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void Layer::operator delete(void *block) {
	gpMemoryServices->free(block);
}

int Layer::draw(CDrawContext *context) {
	layerDrawTimer.start();

	Background *backgroundData = nullptr;

	if (background.lock())
		backgroundData = background.typed;

	if (tiledScene.lock()) {
		TiledScene *scene = tiledScene.typed;

		if (scene) {
			context->setOrtho(-1.0f, 1.0f, -1.0f, 1.0f);

			int frameCount = scene->getFrameCount();

			if (frameCount > 0) {
				// The disasm wraps the frame in place
				while (frame < 0)
					frame += frameCount;

				int frameIndex = frame % frameCount;
				int firstBatch = scene->frames[2 * frameIndex];
				int batchCount = scene->frames[2 * frameIndex + 1];

				if (batchCount > 0 && !backgroundData) {
					warning("Layer::draw(): the background is not available");
				} else if (batchCount > 0) {
					for (int batch = firstBatch; batch < firstBatch + batchCount; batch++) {
						if (batch < 0 || batch >= scene->header[TiledScene::kHeaderNumBatches]) {
							warning("Layer::draw(): invalid batch %d", batch);
							break;
						}

						const int *entry = &scene->batches[3 * batch];
						CTexture *texture = backgroundData->getTexture(entry[0]);

						context->drawQuads(kQuadsFlags, texture, entry[1], entry[2], &scene->vertices);
					}
				}
			}

			tiledScene.unlock();
			background.unlock();
		}
	}

	layerDrawTimer.stop();

	return 0;
}

} // End of namespace Grim
