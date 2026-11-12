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

#ifndef GRIM_EMI_LRESOURCEHANDLE_H
#define GRIM_EMI_LRESOURCEHANDLE_H

#include "common/scummsys.h"
#include "common/textconsole.h"

#include "engines/grim/emi/resources/lresource.h"

namespace Grim {

class LResourceHandleBase {
public:
	LResourceHandleBase();
	LResourceHandleBase(const LResourceHandleBase &other);
	virtual ~LResourceHandleBase();

	virtual bool allocateResource() = 0;
	virtual void setResource(LResource *newResource);

	LResourceHandleBase &operator=(const LResourceHandleBase &other);

	bool lock();
	void unlock();

	void bindEngine(EmiEngine *engine);

	int lockCount;
	LResource *resource;

protected:
	LResourceManager *getManager() const;

	EmiEngine *_engine;
};

template<class T>
class LResourceHandle : public LResourceHandleBase {
public:
	LResourceHandle() {
		typed = nullptr;
	}

	explicit LResourceHandle(EmiEngine *engine) {
		_engine = engine;
		typed = nullptr;
	}

	explicit LResourceHandle(EmiEngine *engine, const LString &name) {
		_engine = engine;
		typed = nullptr;
		resource = getManager()->findResource(name);

		if (!resource) {
			resource = getManager()->createResource(name);

			if (!resource)
				allocateResource();

			if (resource) {
				resource->resSetName(name.c_str());
				getManager()->addResource(resource);
			}
		}

		LResourceHandleBase::setResource(resource);
		typed = dynamic_cast<T *>(resource);

		if (resource && !typed)
			warning("LResourceHandle::LResourceHandle(): the resource has the wrong type");
	}

	LResourceHandle(const LResourceHandle<T> &other) : LResourceHandleBase(other) {
		typed = dynamic_cast<T *>(resource);
	}

	LResourceHandle<T> &operator=(const LResourceHandle<T> &other) {
		LResourceHandleBase::operator=(other);
		typed = dynamic_cast<T *>(resource);

		return *this;
	}

	bool allocateResource() override {
		T *created = new T();

		created->bindEngine(_engine);
		resource = created;
		setResource(created);

		return resource != nullptr;
	}

	void setResource(LResource *newResource) override {
		LResourceHandleBase::setResource(newResource);

		typed = dynamic_cast<T *>(resource);

		if (resource && !typed)
			warning("LResourceHandle::setResource(): the resource has the wrong type");
	}

	T *typed;
};

} // End of namespace Grim

#endif
