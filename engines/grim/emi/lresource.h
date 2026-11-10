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

#ifndef GRIM_EMI_LRESOURCE_H
#define GRIM_EMI_LRESOURCE_H

#include "common/list.h"
#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class LResourceHandleBase;

class LResourceType {
public:
	LResourceType();
	LResourceType(const char *name);

	bool operator==(const LResourceType &other) const;

	uint id;
};

class LResource {
public:
	LResource();
	virtual ~LResource();

	void *operator new(size_t size);
	void operator delete(void *block);

	virtual void initializeRes();
	virtual bool loadResourceItem(const char *itemName);
	virtual bool loadResourceItem(const LString &itemName);
	virtual uint resSizeOfItem();
	virtual LResource *resCacheOut(LFile *file);
	virtual bool resCacheIn(LFile *file);

	bool resIsLoaded() const;
	bool resIsUnique() const;
	bool resIsPurgeOnFree() const;
	bool resIsNotPurgable() const;
	void resSetLoaded(bool value);
	void resSetUnique(bool value);
	void resSetPurgeOnFree(bool value);
	void resSetNotPurgable(bool value);
	LString &resGetName();
	const LResourceType &resGetType() const;
	uint handleCount() const;
	LResource *resLock();
	void resUnlock();
	int resLockCount() const;
	LResourceHandleBase *firstHandle();
	void addHandle(LResourceHandleBase &handle);
	void removeHandle(LResourceHandleBase &handle);
	void resSetName(const char *newName);
	void resSetType(const LResourceType &newType);
	void resReplaceResource(LResource &other);

	LResourceType type;
	LString name;
	Common::List<LResourceHandleBase *> handles;
	int lockCount;
	bool loaded;
	byte byte29;
	bool unique;
	bool purgeOnFree;
	bool notPurgable;

protected:
	virtual bool resLoadItem(const char *itemName);
};

class LResourceManager {
public:
	virtual ~LResourceManager();

	virtual bool resourceLoaded(LResource *resource, bool success);
	virtual bool notifyResource(LResource *resource, bool flag);
	virtual bool addResource(LResource *resource);
	virtual bool removeResource(LResource *resource);
	virtual LResource *findResource(const LString &name);
	virtual LResource *findResource(const char *name);
	virtual void resourceNameChange(LResource *resource, const LString &newName);
	virtual void resourceTypeChange(LResource *resource, const LResourceType &newType);
	virtual LResource *createResource(const LString &name);
	virtual LResource *createResource(const char *name);
	virtual int purgeResource(LResource *resource, bool logPurge);
	virtual void lockResource(LResource *resource);
	virtual void unlockResource(LResource *resource);
};

extern LResourceManager *gpResourceManager;

} // End of namespace Grim

#endif
