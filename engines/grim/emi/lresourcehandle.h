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

namespace Grim {

class LResource;

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

	int lockCount;
	LResource *resource;
};

template<class T>
class LResourceHandle : public LResourceHandleBase {
public:
	LResourceHandle() {
		typed = nullptr;
	}

	LResourceHandle(const LResourceHandle<T> &other) : LResourceHandleBase(other) {
		typed = static_cast<T *>(resource);
	}

	LResourceHandle<T> &operator=(const LResourceHandle<T> &other) {
		LResourceHandleBase::operator=(other);
		typed = static_cast<T *>(resource);

		return *this;
	}

	bool allocateResource() override {
		T *created = new T();

		resource = created;
		setResource(created);

		return resource != nullptr;
	}

	void setResource(LResource *newResource) override {
		LResourceHandleBase::setResource(newResource);

		typed = static_cast<T *>(resource);

		if (resource && !typed)
			warning("LResourceHandle::setResource(): the resource has the wrong type");
	}

	T *typed;
};

} // End of namespace Grim

#endif
