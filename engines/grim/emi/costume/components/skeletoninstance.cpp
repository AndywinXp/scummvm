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

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/costume/components/skeletoncomponent.h"
#include "engines/grim/emi/costume/components/skeletoninstance.h"

namespace Grim {

SkeletonInstance::SkeletonInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	_active = false;
	_owner = actor;
	_rootNode = nullptr;

	SkeletonComponent *component = nullptr;

	if (_handle.lock())
		component = dynamic_cast<SkeletonComponent *>(_handle._typed);

	if (component && !_owner) {
		warning("SkeletonInstance::SkeletonInstance(): no owner");
		component = nullptr;
	}

	if (component) {
		_rootNode = component->buildNodeTree();
		_owner->attachNode(_rootNode);
		_active = true;
	}

	_handle.unlock();
}

SkeletonInstance::~SkeletonInstance() {
	if (_active)
		_owner->detachNode(_rootNode);

	if (_rootNode)
		freeNodeTree(_rootNode);
	else
		warning("SkeletonInstance::~SkeletonInstance(): no node tree");

	_rootNode = nullptr;
}

void SkeletonInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	bool nonZero = (value < 0.0f || value > 0.0f);

	if (_active) {
		if (!nonZero) {
			_owner->detachNode(_rootNode);
			_active = false;
		}
	} else if (nonZero) {
		if (!_owner) {
			warning("SkeletonInstance::setKey(): no owner");

			return;
		}

		_owner->attachNode(_rootNode);
		_active = true;
	}
}

void SkeletonInstance::reset() {
	if (_active) {
		_owner->detachNode(_rootNode);
		_active = false;
	}
}

void SkeletonInstance::freeNodeTree(CNode *node) {
	if (node->_firstChild)
		freeNodeTree(node->_firstChild);

	for (CNode *sibling = node->_nextSibling; sibling; sibling = node->_nextSibling)
		freeNodeTree(sibling);

	delete node;
}

} // End of namespace Grim
