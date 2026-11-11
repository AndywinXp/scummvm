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
#include "common/ustr.h"
#include "common/util.h"

#include "engines/grim/emi/animcomponent.h"
#include "engines/grim/emi/animwgtcomponent.h"
#include "engines/grim/emi/cubecomponent.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/globalalphacomponent.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/localalphacomponent.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/luacommandcomponent.h"
#include "engines/grim/emi/luavarcomponent.h"
#include "engines/grim/emi/meshcomponent.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/shadowmeshcomponent.h"
#include "engines/grim/emi/skeletoncomponent.h"
#include "engines/grim/emi/snd3dcomponent.h"
#include "engines/grim/emi/sndcomponent.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/spritecomponent.h"
#include "engines/grim/emi/textureindexcomponent.h"

namespace Grim {

static ResourceManager gResourceManager;
static LResourceManager *gPreviousResourceManager = nullptr;
static MemoryHandler *gMemoryHandler = nullptr;
static float gUpdateTime = 0.0f;
static bool gLockedMessageShown = false;

uint MemoryHandler::purgeAmount(uint bytes) {
	if (gpResourceManager != &gResourceManager)
		return 0;

	LMemoryServices::mbCapLimitsActive = false;
	uint result = gResourceManager.purgeAmount(bytes);
	LMemoryServices::mbCapLimitsActive = true;

	return result;
}

ResourceManager::ResourceManager() {
	logFile = nullptr;
}

bool ResourceManager::resourceLoaded(LResource *resource, bool success) {
	if (logFile && !success)
		logFile->writeFormat("Resource %s failed to load.\n", resource->name.c_str());

	return true;
}

bool ResourceManager::addResource(LResource *resource) {
	if (!resource)
		return false;

	if (resource->name.empty())
		return false;

	Common::List<LResource *>::iterator it;

	for (it = allResources.begin(); it != allResources.end(); ++it) {
		if (*it == resource)
			return false;
	}

	allResources.push_front(resource);

	if (!resource->name.empty())
		resourceNameChange(resource, resource->name);

	return true;
}

bool ResourceManager::removeResource(LResource *resource) {
	Common::List<LResource *>::iterator it;

	for (it = allResources.begin(); it != allResources.end(); ++it) {
		if (*it == resource)
			break;
	}

	if (it == allResources.end())
		return false;

	resourcesByName.erase(resource->name);
	allResources.erase(it);

	return true;
}

LResource *ResourceManager::findResource(const LString &name) {
	ResourceMap::iterator it = resourcesByName.find(name);

	if (it == resourcesByName.end())
		return nullptr;

	return it->_value;
}

LResource *ResourceManager::findResource(const char *name) {
	return findResource(LString(name));
}

void ResourceManager::resourceNameChange(LResource *resource, const LString &newName) {
	resourcesByName.erase(resource->name);

	if (resourcesByName.contains(newName)) {
		if (gpConsoleServices)
			gpConsoleServices->logError("ResourceManager::ResNameChange - Resource name %s is not unique!\n", newName.c_str());
		else
			warning("ResourceManager::resourceNameChange(): resource name %s is not unique", newName.c_str());
	} else {
		resourcesByName[newName] = resource;
	}
}

LResource *ResourceManager::createResource(const LString &name) {
	const char *resourceName = name.c_str();

	if (*resourceName == '!') {
		const char *tag = resourceName + 1;

		if (!scumm_strnicmp(tag, "anim", 4))
			return new AnimComponent();

		if (!scumm_strnicmp(tag, "awgt", 4))
			return new AnimWgtComponent();

		if (!scumm_strnicmp(tag, "cube", 4))
			return new CubeComponent();

		if (!scumm_strnicmp(tag, "galp", 4))
			return new GlobalAlphaComponent();

		if (!scumm_strnicmp(tag, "lalp", 4))
			return new LocalAlphaComponent();

		if (!scumm_strnicmp(tag, "luac", 4))
			return new LuaCommandComponent();

		if (!scumm_strnicmp(tag, "luav", 4))
			return new LuaVarComponent();

		if (!scumm_strnicmp(tag, "mesh", 4))
			return new MeshComponent();

		if (!scumm_strnicmp(tag, "shad", 4))
			return new ShadowMeshComponent();

		if (!scumm_strnicmp(tag, "skel", 4))
			return new SkeletonComponent();

		if (!scumm_strnicmp(tag, "snd2", 4))
			return new SndComponent();

		if (!scumm_strnicmp(tag, "snd3", 4))
			return new Snd3DComponent();

		if (!scumm_strnicmp(tag, "sprt", 4))
			return new SpriteComponent();

		if (!scumm_strnicmp(tag, "texi", 4))
			return new TextureIndexComponent();
	} else if (*resourceName == '@') {
		return Sounds::createWaveSoundResource();
	}

	return nullptr;
}

LResource *ResourceManager::createResource(const char *name) {
	return createResource(LString(name));
}

int ResourceManager::purgeResource(LResource *resource, bool logPurge) {
	uint replaceDelta = 0;

	if (resource->lockCount)
		return -1;

	if (resource->notPurgable)
		return -1;

	if (resource->handleCount() != 0) {
		uint bytesBefore = gpMemoryServices->bytesAllocated;

		removeResource(resource);

		LResourceHandleBase *handle = resource->firstHandle();
		handle->setResource(nullptr);

		LResource *replacement = gpResourceManager->createResource(resource->name);

		if (!replacement) {
			handle->allocateResource();
			replacement = handle->resource;
		}

		replacement->resSetName(resource->name.c_str());
		resource->resReplaceResource(*replacement);
		addResource(replacement);

		replaceDelta = bytesBefore - gpMemoryServices->bytesAllocated;
	}

	LString resourceName = resource->name;
	uint bytesAfterReplace = gpMemoryServices->bytesAllocated + replaceDelta;

	Common::List<LResource *>::iterator it = lruResources.begin();

	while (it != lruResources.end()) {
		if (*it == resource)
			it = lruResources.erase(it);
		else
			++it;
	}

	delete resource;

	int reclaimed = (int)(bytesAfterReplace - gpMemoryServices->bytesAllocated);

	if (logPurge && logFile)
		logFile->writeFormat("PURGED RESOURCE %s: %d bytes reclaimed.\n", resourceName.c_str(), reclaimed);

	return reclaimed;
}

void ResourceManager::lockResource(LResource *resource) {
	Common::List<LResource *>::iterator it = lruResources.begin();

	while (it != lruResources.end()) {
		if (*it == resource)
			it = lruResources.erase(it);
		else
			++it;
	}
}

void ResourceManager::unlockResource(LResource *resource) {
	if (resource->lockCount == 0)
		lruResources.push_back(resource);
}

uint ResourceManager::purgeAmount(uint bytes) {
	uint freed = 0;

	if (logFile)
		logFile->writeFormat("Purge Amount: %d requested.\n", bytes);

	while (!lruResources.empty() && freed < bytes) {
		LResource *resource = lruResources.front();

		freed += (uint)purgeResource(resource, true);

		Common::List<LResource *>::iterator it = lruResources.begin();

		while (it != lruResources.end()) {
			if (*it == resource)
				it = lruResources.erase(it);
			else
				++it;
		}
	}

	if (freed < bytes) {
		if (logFile)
			logFile->writeFormat("!!!!! Purge Amount unsuccessful: only %d bytes freed!\n", freed);

		return freed;
	}

	if (logFile)
		logFile->writeFormat("Purge Amount successful: %d bytes freed.\n", freed);

	return freed;
}

int ResourceManager::purgeAll() {
	int total = 0;
	int reclaimed;

	for (reclaimed = purgeUnused(); reclaimed != 0; reclaimed = purgeUnused())
		total += reclaimed;

	if (allResources.size() != 0) {
		if (gpConsoleServices)
			gpConsoleServices->logError("ResourceManager::PurgeAll(): Remaining resources are locked or have dangling handles!\n");
		else
			warning("ResourceManager::purgeAll(): remaining resources are locked or have dangling handles");

		for (reclaimed = purgeUnlocked(); reclaimed != 0; reclaimed = purgeUnlocked())
			total += reclaimed;

		if (allResources.size() != 0) {
			if (gpConsoleServices)
				gpConsoleServices->logError("ResourceManager::PurgeAll(): Remaining resources are locked!\n");
			else
				warning("ResourceManager::purgeAll(): remaining resources are locked");
		}
	}

	return total;
}

int ResourceManager::purgeUnused() {
	int count = 0;
	int bytes = 0;

	Common::List<LResource *>::iterator it = allResources.begin();

	while (it != allResources.end()) {
		LResource *resource = *it;
		++it;

		if (resource->handles.empty()) {
			int reclaimed = purgeResource(resource, true);

			if (reclaimed < 0)
				reclaimed = 0;

			count++;
			bytes += reclaimed;
		}
	}

	if (logFile)
		logFile->writeFormat("Purged %d unused items for %d bytes.\n", count, bytes);

	return bytes;
}

int ResourceManager::purgeUnlocked() {
	int count = 0;
	int bytes = 0;

	Common::List<LResource *>::iterator it = allResources.begin();

	while (it != allResources.end()) {
		LResource *resource = *it;
		++it;

		if (!resource->lockCount) {
			int reclaimed = purgeResource(resource, true);

			if (reclaimed < 0)
				reclaimed = 0;

			count++;
			bytes += reclaimed;
		}
	}

	if (logFile)
		logFile->writeFormat("Purged %d unlocked items for %d bytes.\n", count, bytes);

	return bytes;
}

void ResourceManager::setLogFile(LFile *file) {
	logFile = file;
}

LFile *ResourceManager::detachLogFile() {
	LFile *file = logFile;
	logFile = nullptr;

	return file;
}

void ResourceManager::dumpLru(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (logFile) {
		int bytes = 0;
		int count = 0;

		logFile->writeFormat("Resource Dump\n");
		logFile->writeFormat("Flags:\n");
		logFile->writeFormat("L - Loaded\n");
		logFile->writeFormat("U - Unique\n");
		logFile->writeFormat("P - Purge on free\n");
		logFile->writeFormat("\n");
		logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		logFile->writeFormat("=============================================================\n");

		for (it = lruResources.begin(); it != lruResources.end(); ++it) {
			if (!type || (*it)->type.id == type->id) {
				count++;
				bytes += dumpResource(*it, logFile);
			}
		}

		logFile->writeFormat("\n");
		logFile->writeFormat("Total Bytes allocated: %d\n", bytes);
		logFile->writeFormat("Total items allocated: %d\n", count);
	} else {
		for (it = lruResources.begin(); it != lruResources.end(); ++it) {
			if (!type || (*it)->type.id == type->id)
				dumpResource(*it, nullptr);
		}
	}
}

void ResourceManager::dumpAll(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (logFile) {
		int bytes = 0;
		int count = 0;

		logFile->writeFormat("Resource Dump\n");
		logFile->writeFormat("Flags:\n");
		logFile->writeFormat("L - Loaded\n");
		logFile->writeFormat("U - Unique\n");
		logFile->writeFormat("P - Purge on free\n");
		logFile->writeFormat("\n");
		logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		logFile->writeFormat("=============================================================\n");

		for (it = allResources.begin(); it != allResources.end(); ++it) {
			if (!type || (*it)->type.id == type->id) {
				count++;
				bytes += dumpResource(*it, logFile);
			}
		}

		logFile->writeFormat("\n");
		logFile->writeFormat("Total Bytes allocated: %d\n", bytes);
		logFile->writeFormat("Total items allocated: %d\n", count);
	} else {
		for (it = allResources.begin(); it != allResources.end(); ++it) {
			if (!type || (*it)->type.id == type->id)
				dumpResource(*it, nullptr);
		}
	}
}

void ResourceManager::dumpUsed(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (logFile) {
		int bytes = 0;
		int count = 0;

		logFile->writeFormat("Resource Dump Used\n");
		logFile->writeFormat("Flags:\n");
		logFile->writeFormat("L - Loaded\n");
		logFile->writeFormat("U - Unique\n");
		logFile->writeFormat("P - Purge on free\n");
		logFile->writeFormat("\n");
		logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		logFile->writeFormat("=============================================================\n");

		for (it = allResources.begin(); it != allResources.end(); ++it) {
			if (!(*it)->handles.empty() && (!type || (*it)->type.id == type->id)) {
				count++;
				bytes += dumpResource(*it, logFile);
			}
		}

		logFile->writeFormat("\n");
		logFile->writeFormat("Total Bytes allocated: %d.\n", bytes);
		logFile->writeFormat("Total items allocated: %d.\n", count);
	} else {
		for (it = allResources.begin(); it != allResources.end(); ++it) {
			if (!(*it)->handles.empty() && (!type || (*it)->type.id == type->id))
				dumpResource(*it, nullptr);
		}
	}
}

void ResourceManager::dumpUnused(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (logFile) {
		int bytes = 0;
		int count = 0;

		logFile->writeFormat("Resource Dump Unused\n");
		logFile->writeFormat("Flags:\n");
		logFile->writeFormat("L - Loaded\n");
		logFile->writeFormat("U - Unique\n");
		logFile->writeFormat("P - Purge on free\n");
		logFile->writeFormat("N - Not Purgable\n");
		logFile->writeFormat("\n");
		logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		logFile->writeFormat("=============================================================\n");

		for (it = allResources.begin(); it != allResources.end(); ++it) {
			if ((*it)->handles.empty() && (!type || (*it)->type.id == type->id)) {
				count++;
				bytes += dumpResource(*it, logFile);
			}
		}

		logFile->writeFormat("\n");
		logFile->writeFormat("Total Bytes allocated: %d.\n", bytes);
		logFile->writeFormat("Total items allocated: %d.\n", count);
	} else {
		for (it = allResources.begin(); it != allResources.end(); ++it) {
			if ((*it)->handles.empty() && (!type || (*it)->type.id == type->id))
				dumpResource(*it, nullptr);
		}
	}
}

bool ResourceManager::startup() {
	gPreviousResourceManager = gpResourceManager;
	gpResourceManager = &gResourceManager;

	gMemoryHandler = new MemoryHandler();
	gpMemoryServices->registerMemoryHandler(gMemoryHandler);

	return true;
}

void ResourceManager::shutdown() {
	gpMemoryServices->unregisterMemoryHandler(gMemoryHandler);
	gResourceManager.purgeAll();

	delete gMemoryHandler;
	gMemoryHandler = nullptr;

	gpResourceManager = gPreviousResourceManager;
}

void ResourceManager::update(float frameTime) {
	uint bytes = gpMemoryServices->bytesAllocated;

	if (bytes > 0xC00000 && gResourceManager.purgeAmount(bytes - 0xC00000) < bytes - 0xC00000 && !gLockedMessageShown) {
		EmiEngine::showMessage("More than 12MB of resources have been left locked.\n"
				"Please record your location, report this to the proper authorities, and exit the game.\n"
				"\n"
				"\n"
				"(Then get back to work.)",
				"Congratulations, you have found a bug!");
		gLockedMessageShown = true;
	}

	double elapsed = gUpdateTime + frameTime;
	gUpdateTime = (float)elapsed;

	if (elapsed > 10.0) {
		Common::List<LResource *>::iterator it = gResourceManager.allResources.begin();

		while (it != gResourceManager.allResources.end()) {
			LResource *resource = *it;
			++it;

			if (resource && resource->purgeOnFree && !resource->loaded && resource->handles.empty())
				gResourceManager.purgeResource(resource, false);
		}

		gUpdateTime = 0.0f;
	}
}

int ResourceManager::dumpResource(LResource *resource, LFile *file) {
	if (!resource)
		return 0;

	if (file) {
		int nameLength = (int)resource->name.size() - 30;

		file->writeFormat("%-30s %c%c%c%c  %10d%6d%8d\n",
				resource->name.c_str() + (nameLength <= 0 ? 0 : nameLength),
				resource->loaded ? 'L' : ' ',
				resource->unique ? 'U' : ' ',
				resource->purgeOnFree ? 'P' : ' ',
				resource->notPurgable ? 'N' : ' ',
				resource->resSizeOfItem(),
				resource->lockCount,
				resource->handleCount());
	}

	return resource->resSizeOfItem();
}

} // End of namespace Grim
