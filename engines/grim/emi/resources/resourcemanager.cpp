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

#include "engines/grim/emi/costume/components/animcomponent.h"
#include "engines/grim/emi/costume/components/animwgtcomponent.h"
#include "engines/grim/emi/costume/components/cubecomponent.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/costume/components/globalalphacomponent.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/costume/components/localalphacomponent.h"
#include "engines/grim/emi/resources/lresourcehandle.h"
#include "engines/grim/emi/costume/components/luacommandcomponent.h"
#include "engines/grim/emi/costume/components/luavarcomponent.h"
#include "engines/grim/emi/costume/components/meshcomponent.h"
#include "engines/grim/emi/resources/resourcemanager.h"
#include "engines/grim/emi/costume/components/shadowmeshcomponent.h"
#include "engines/grim/emi/costume/components/skeletoncomponent.h"
#include "engines/grim/emi/costume/components/snd3dcomponent.h"
#include "engines/grim/emi/costume/components/sndcomponent.h"
#include "engines/grim/emi/sound/sounds.h"
#include "engines/grim/emi/costume/components/spritecomponent.h"
#include "engines/grim/emi/costume/components/textureindexcomponent.h"

namespace Grim {

MemoryHandler::MemoryHandler(ResourceManager *manager) {
	_manager = manager;
}

uint MemoryHandler::purgeAmount(uint bytes) {
	EmiEngine *engine = _manager->getEngine();

	if (engine->getResourceManager() != _manager)
		return 0;

	engine->getMemoryServices()->activateMemoryCaps(false);
	uint result = _manager->purgeAmount(bytes);
	engine->getMemoryServices()->activateMemoryCaps(true);

	return result;
}

ResourceManager::ResourceManager(EmiEngine *engine) {
	_engine = engine;
	_previousResourceManager = nullptr;
	_memoryHandler = nullptr;
	_updateTime = 0.0f;
	_lockedMessageShown = false;
	_logFile = nullptr;
}

EmiEngine *ResourceManager::getEngine() const {
	return _engine;
}

bool ResourceManager::resourceLoaded(LResource *resource, bool success) {
	if (_logFile && !success)
		_logFile->writeFormat("Resource %s failed to load.\n", resource->_name.c_str());

	return true;
}

bool ResourceManager::addResource(LResource *resource) {
	if (!resource)
		return false;

	if (resource->_name.empty())
		return false;

	Common::List<LResource *>::iterator it;

	for (it = _allResources.begin(); it != _allResources.end(); ++it) {
		if (*it == resource)
			return false;
	}

	_allResources.push_front(resource);

	if (!resource->_name.empty())
		resourceNameChange(resource, resource->_name);

	return true;
}

bool ResourceManager::removeResource(LResource *resource) {
	Common::List<LResource *>::iterator it;

	for (it = _allResources.begin(); it != _allResources.end(); ++it) {
		if (*it == resource)
			break;
	}

	if (it == _allResources.end())
		return false;

	_resourcesByName.erase(resource->_name);
	_allResources.erase(it);

	return true;
}

LResource *ResourceManager::findResource(const LString &name) {
	ResourceMap::iterator it = _resourcesByName.find(name);

	if (it == _resourcesByName.end())
		return nullptr;

	return it->_value;
}

LResource *ResourceManager::findResource(const char *name) {
	return findResource(LString(name));
}

void ResourceManager::resourceNameChange(LResource *resource, const LString &newName) {
	_resourcesByName.erase(resource->_name);

	if (_resourcesByName.contains(newName)) {
		LConsoleServices *console = _engine->getConsoleServices();

		if (console)
			console->logError("ResourceManager::ResNameChange - Resource name %s is not unique!\n", newName.c_str());
		else
			warning("ResourceManager::resourceNameChange(): resource name %s is not unique", newName.c_str());
	} else {
		_resourcesByName[newName] = resource;
	}
}

LResource *ResourceManager::createResource(const LString &name) {
	const char *resourceName = name.c_str();
	LResource *created = nullptr;

	if (*resourceName == '!') {
		const char *tag = resourceName + 1;

		if (!scumm_strnicmp(tag, "anim", 4))
			created = new AnimComponent(_engine);
		else if (!scumm_strnicmp(tag, "awgt", 4))
			created = new AnimWgtComponent();
		else if (!scumm_strnicmp(tag, "cube", 4))
			created = new CubeComponent();
		else if (!scumm_strnicmp(tag, "galp", 4))
			created = new GlobalAlphaComponent();
		else if (!scumm_strnicmp(tag, "lalp", 4))
			created = new LocalAlphaComponent();
		else if (!scumm_strnicmp(tag, "luac", 4))
			created = new LuaCommandComponent();
		else if (!scumm_strnicmp(tag, "luav", 4))
			created = new LuaVarComponent();
		else if (!scumm_strnicmp(tag, "mesh", 4))
			created = new MeshComponent(_engine);
		else if (!scumm_strnicmp(tag, "shad", 4))
			created = new ShadowMeshComponent(_engine);
		else if (!scumm_strnicmp(tag, "skel", 4))
			created = new SkeletonComponent(_engine);
		else if (!scumm_strnicmp(tag, "snd2", 4))
			created = new SndComponent();
		else if (!scumm_strnicmp(tag, "snd3", 4))
			created = new Snd3DComponent();
		else if (!scumm_strnicmp(tag, "sprt", 4))
			created = new SpriteComponent(_engine);
		else if (!scumm_strnicmp(tag, "texi", 4))
			created = new TextureIndexComponent();
	} else if (*resourceName == '@') {
		created = _engine->getSounds()->createWaveSoundResource();
	}

	if (created)
		created->bindEngine(_engine);

	return created;
}

LResource *ResourceManager::createResource(const char *name) {
	return createResource(LString(name));
}

int ResourceManager::purgeResource(LResource *resource, bool logPurge) {
	uint replaceDelta = 0;
	LMemoryServices *memoryServices = _engine->getMemoryServices();

	LConsoleServices *console = _engine->getConsoleServices();

	if (resource->_lockCount) {
		if (_engine->isDemo() && console)
			console->logError("Attempted to purge the locked resource %s.\n", resource->_name.c_str());

		return -1;
	}

	if (resource->_notPurgable) {
		if (_engine->isDemo() && console)
			console->logError("Attempted to purge the unpurgable resource %s.\n", resource->_name.c_str());

		return -1;
	}

	if (resource->handleCount() != 0) {
		uint bytesBefore = memoryServices->_bytesAllocated;

		removeResource(resource);

		LResourceHandleBase *handle = resource->firstHandle();
		handle->setResource(nullptr);

		LResource *replacement = _engine->getResourceManager()->createResource(resource->_name);

		if (!replacement) {
			handle->bindEngine(_engine);
			handle->allocateResource();
			replacement = handle->_resource;
		}

		replacement->resSetName(resource->_name.c_str());
		resource->resReplaceResource(*replacement);
		addResource(replacement);

		replaceDelta = bytesBefore - memoryServices->_bytesAllocated;
	}

	LString resourceName = resource->_name;
	uint bytesAfterReplace = memoryServices->_bytesAllocated + replaceDelta;

	Common::List<LResource *>::iterator it = _lruResources.begin();

	while (it != _lruResources.end()) {
		if (*it == resource)
			it = _lruResources.erase(it);
		else
			++it;
	}

	delete resource;

	int reclaimed = (int)(bytesAfterReplace - memoryServices->_bytesAllocated);

	if (_engine->isDemo()) {
		// The demo always logs the purge, to the log file or else to the console
		if (_logFile)
			_logFile->writeFormat("PURGED RESOURCE %s: %d bytes reclaimed.\n", resourceName.c_str(), reclaimed);
		else if (console)
			console->logStatus("PURGED RESOURCE %s: %d bytes reclaimed.\n", resourceName.c_str(), reclaimed);

		if (reclaimed < 0 && console)
			console->logError("Negative number of bytes reclaimed for the resource %s.\n", resourceName.c_str());
	} else if (logPurge && _logFile) {
		_logFile->writeFormat("PURGED RESOURCE %s: %d bytes reclaimed.\n", resourceName.c_str(), reclaimed);
	}

	return reclaimed;
}

void ResourceManager::lockResource(LResource *resource) {
	Common::List<LResource *>::iterator it = _lruResources.begin();

	while (it != _lruResources.end()) {
		if (*it == resource)
			it = _lruResources.erase(it);
		else
			++it;
	}
}

void ResourceManager::unlockResource(LResource *resource) {
	if (resource->_lockCount == 0)
		_lruResources.push_back(resource);
}

uint ResourceManager::purgeAmount(uint bytes) {
	uint freed = 0;

	if (_logFile)
		_logFile->writeFormat("Purge Amount: %d requested.\n", bytes);

	while (!_lruResources.empty() && freed < bytes) {
		LResource *resource = _lruResources.front();

		freed += (uint)purgeResource(resource, true);

		Common::List<LResource *>::iterator it = _lruResources.begin();

		while (it != _lruResources.end()) {
			if (*it == resource)
				it = _lruResources.erase(it);
			else
				++it;
		}
	}

	if (freed < bytes) {
		if (_logFile)
			_logFile->writeFormat("!!!!! Purge Amount unsuccessful: only %d bytes freed!\n", freed);

		return freed;
	}

	if (_logFile)
		_logFile->writeFormat("Purge Amount successful: %d bytes freed.\n", freed);

	return freed;
}

int ResourceManager::purgeAll() {
	int total = 0;
	int reclaimed;

	for (reclaimed = purgeUnused(); reclaimed != 0; reclaimed = purgeUnused())
		total += reclaimed;

	if (_allResources.size() != 0) {
		LConsoleServices *console = _engine->getConsoleServices();

		if (console)
			console->logError("ResourceManager::PurgeAll(): Remaining resources are locked or have dangling handles!\n");
		else
			warning("ResourceManager::purgeAll(): remaining resources are locked or have dangling handles");

		for (reclaimed = purgeUnlocked(); reclaimed != 0; reclaimed = purgeUnlocked())
			total += reclaimed;

		if (_allResources.size() != 0) {
			if (console)
				console->logError("ResourceManager::PurgeAll(): Remaining resources are locked!\n");
			else
				warning("ResourceManager::purgeAll(): remaining resources are locked");
		}
	}

	return total;
}

int ResourceManager::purgeUnused() {
	int count = 0;
	int bytes = 0;

	Common::List<LResource *>::iterator it = _allResources.begin();

	while (it != _allResources.end()) {
		LResource *resource = *it;
		++it;

		if (resource->_handles.empty()) {
			int reclaimed = purgeResource(resource, true);

			if (reclaimed < 0)
				reclaimed = 0;

			count++;
			bytes += reclaimed;
		}
	}

	if (_logFile)
		_logFile->writeFormat("Purged %d unused items for %d bytes.\n", count, bytes);

	return bytes;
}

int ResourceManager::purgeUnlocked() {
	int count = 0;
	int bytes = 0;

	Common::List<LResource *>::iterator it = _allResources.begin();

	while (it != _allResources.end()) {
		LResource *resource = *it;
		++it;

		if (!resource->_lockCount) {
			int reclaimed = purgeResource(resource, true);

			if (reclaimed < 0)
				reclaimed = 0;

			count++;
			bytes += reclaimed;
		}
	}

	if (_logFile)
		_logFile->writeFormat("Purged %d unlocked items for %d bytes.\n", count, bytes);

	return bytes;
}

void ResourceManager::setLogFile(LFile *file) {
	_logFile = file;
}

LFile *ResourceManager::detachLogFile() {
	LFile *file = _logFile;
	_logFile = nullptr;

	return file;
}

void ResourceManager::dumpLru(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (_logFile) {
		int bytes = 0;
		int count = 0;

		_logFile->writeFormat("Resource Dump\n");
		_logFile->writeFormat("Flags:\n");
		_logFile->writeFormat("L - Loaded\n");
		_logFile->writeFormat("U - Unique\n");
		_logFile->writeFormat("P - Purge on free\n");
		_logFile->writeFormat("\n");
		_logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		_logFile->writeFormat("=============================================================\n");

		for (it = _lruResources.begin(); it != _lruResources.end(); ++it) {
			if (!type || (*it)->_type._id == type->_id) {
				count++;
				bytes += dumpResource(*it, _logFile);
			}
		}

		_logFile->writeFormat("\n");
		_logFile->writeFormat("Total Bytes allocated: %d\n", bytes);
		_logFile->writeFormat("Total items allocated: %d\n", count);
	} else {
		for (it = _lruResources.begin(); it != _lruResources.end(); ++it) {
			if (!type || (*it)->_type._id == type->_id)
				dumpResource(*it, nullptr);
		}
	}
}

void ResourceManager::dumpAll(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (_logFile) {
		int bytes = 0;
		int count = 0;

		_logFile->writeFormat("Resource Dump\n");
		_logFile->writeFormat("Flags:\n");
		_logFile->writeFormat("L - Loaded\n");
		_logFile->writeFormat("U - Unique\n");
		_logFile->writeFormat("P - Purge on free\n");
		_logFile->writeFormat("\n");
		_logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		_logFile->writeFormat("=============================================================\n");

		for (it = _allResources.begin(); it != _allResources.end(); ++it) {
			if (!type || (*it)->_type._id == type->_id) {
				count++;
				bytes += dumpResource(*it, _logFile);
			}
		}

		_logFile->writeFormat("\n");
		_logFile->writeFormat("Total Bytes allocated: %d\n", bytes);
		_logFile->writeFormat("Total items allocated: %d\n", count);
	} else {
		for (it = _allResources.begin(); it != _allResources.end(); ++it) {
			if (!type || (*it)->_type._id == type->_id)
				dumpResource(*it, nullptr);
		}
	}
}

void ResourceManager::dumpUsed(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (_logFile) {
		int bytes = 0;
		int count = 0;

		_logFile->writeFormat("Resource Dump Used\n");
		_logFile->writeFormat("Flags:\n");
		_logFile->writeFormat("L - Loaded\n");
		_logFile->writeFormat("U - Unique\n");
		_logFile->writeFormat("P - Purge on free\n");
		_logFile->writeFormat("\n");
		_logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		_logFile->writeFormat("=============================================================\n");

		for (it = _allResources.begin(); it != _allResources.end(); ++it) {
			if (!(*it)->_handles.empty() && (!type || (*it)->_type._id == type->_id)) {
				count++;
				bytes += dumpResource(*it, _logFile);
			}
		}

		_logFile->writeFormat("\n");
		_logFile->writeFormat("Total Bytes allocated: %d.\n", bytes);
		_logFile->writeFormat("Total items allocated: %d.\n", count);
	} else {
		for (it = _allResources.begin(); it != _allResources.end(); ++it) {
			if (!(*it)->_handles.empty() && (!type || (*it)->_type._id == type->_id))
				dumpResource(*it, nullptr);
		}
	}
}

void ResourceManager::dumpUnused(const LResourceType *type) {
	Common::List<LResource *>::iterator it;

	if (_logFile) {
		int bytes = 0;
		int count = 0;

		_logFile->writeFormat("Resource Dump Unused\n");
		_logFile->writeFormat("Flags:\n");
		_logFile->writeFormat("L - Loaded\n");
		_logFile->writeFormat("U - Unique\n");
		_logFile->writeFormat("P - Purge on free\n");
		_logFile->writeFormat("N - Not Purgable\n");
		_logFile->writeFormat("\n");
		_logFile->writeFormat("NAME                           FLAGS       SIZE LOCKS HANDLES\n");
		_logFile->writeFormat("=============================================================\n");

		for (it = _allResources.begin(); it != _allResources.end(); ++it) {
			if ((*it)->_handles.empty() && (!type || (*it)->_type._id == type->_id)) {
				count++;
				bytes += dumpResource(*it, _logFile);
			}
		}

		_logFile->writeFormat("\n");
		_logFile->writeFormat("Total Bytes allocated: %d.\n", bytes);
		_logFile->writeFormat("Total items allocated: %d.\n", count);
	} else {
		for (it = _allResources.begin(); it != _allResources.end(); ++it) {
			if ((*it)->_handles.empty() && (!type || (*it)->_type._id == type->_id))
				dumpResource(*it, nullptr);
		}
	}
}

bool ResourceManager::startup() {
	_previousResourceManager = _engine->getResourceManager();
	_engine->setResourceManager(this);

	_memoryHandler = new MemoryHandler(this);
	_engine->getMemoryServices()->registerMemoryHandler(_memoryHandler);

	return true;
}

void ResourceManager::shutdown() {
	_engine->getMemoryServices()->unregisterMemoryHandler(_memoryHandler);
	purgeAll();

	delete _memoryHandler;
	_memoryHandler = nullptr;

	_engine->setResourceManager(_previousResourceManager);
}

void ResourceManager::update(float frameTime) {
	uint bytes = _engine->getMemoryServices()->_bytesAllocated;

	// The demo only keeps the resources under 12 MB: no warning, no sweep of the idle ones
	if (_engine->isDemo()) {
		if (bytes > 0xC00000)
			purgeAmount(bytes - 0xC00000);

		return;
	}

	if (bytes > 0xC00000 && purgeAmount(bytes - 0xC00000) < bytes - 0xC00000 && !_lockedMessageShown) {
		_engine->showMessage("More than 12MB of resources have been left locked.\n"
				"Please record your location, report this to the proper authorities, and exit the game.\n"
				"\n"
				"\n"
				"(Then get back to work.)",
				"Congratulations, you have found a bug!");
		_lockedMessageShown = true;
	}

	double elapsed = _updateTime + frameTime;
	_updateTime = (float)elapsed;

	if (elapsed > 10.0) {
		Common::List<LResource *>::iterator it = _allResources.begin();

		while (it != _allResources.end()) {
			LResource *resource = *it;
			++it;

			if (resource && resource->_purgeOnFree && !resource->_loaded && resource->_handles.empty())
				purgeResource(resource, false);
		}

		_updateTime = 0.0f;
	}
}

int ResourceManager::dumpResource(LResource *resource, LFile *file) {
	if (!resource)
		return 0;

	if (file) {
		int nameLength = (int)resource->_name.size() - 30;

		file->writeFormat("%-30s %c%c%c%c  %10d%6d%8d\n",
				resource->_name.c_str() + (nameLength <= 0 ? 0 : nameLength),
				resource->_loaded ? 'L' : ' ',
				resource->_unique ? 'U' : ' ',
				resource->_purgeOnFree ? 'P' : ' ',
				resource->_notPurgable ? 'N' : ' ',
				resource->resSizeOfItem(),
				resource->_lockCount,
				resource->handleCount());
	}

	return resource->resSizeOfItem();
}

} // End of namespace Grim
