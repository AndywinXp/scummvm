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

#ifndef GRIM_EMI_RESOURCEMANAGER_H
#define GRIM_EMI_RESOURCEMANAGER_H

#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/list.h"
#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lresource.h"

namespace Grim {

class MemoryHandler : public LMemoryServices::LMemoryHandler {
public:
	uint purgeAmount(uint bytes) override;
};

class ResourceManager : public LResourceManager {
public:
	typedef Common::HashMap<LString, LResource *> ResourceMap;

	ResourceManager();

	bool resourceLoaded(LResource *resource, bool success) override;
	bool addResource(LResource *resource) override;
	bool removeResource(LResource *resource) override;
	LResource *findResource(const LString &name) override;
	LResource *findResource(const char *name) override;
	void resourceNameChange(LResource *resource, const LString &newName) override;
	LResource *createResource(const LString &name) override;
	LResource *createResource(const char *name) override;
	int purgeResource(LResource *resource, bool logPurge) override;
	void lockResource(LResource *resource) override;
	void unlockResource(LResource *resource) override;

	uint purgeAmount(uint bytes);
	int purgeAll();
	int purgeUnused();
	int purgeUnlocked();
	void setLogFile(LFile *file);
	LFile *detachLogFile();
	void dumpLru(const LResourceType *type);
	void dumpAll(const LResourceType *type);
	void dumpUsed(const LResourceType *type);
	void dumpUnused(const LResourceType *type);

	static bool startup();
	static void shutdown();
	static void update(float frameTime);
	static int dumpResource(LResource *resource, LFile *file);

	LFile *logFile;
	Common::List<LResource *> allResources;
	Common::List<LResource *> lruResources;
	ResourceMap resourcesByName;
};

} // End of namespace Grim

#endif
