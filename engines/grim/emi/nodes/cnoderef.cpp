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

#include "engines/grim/emi/nodes/cnoderef.h"

namespace Grim {

CNodeRef::CNodeRef(CNode *observedNode) {
	_prev = nullptr;
	_next = nullptr;
	_node = observedNode;
	linkToNode();
}

CNodeRef::~CNodeRef() {
	unlinkFromNode();
}

void CNodeRef::onNodeDestroyed() {
}

void CNodeRef::setNode(CNode *newNode) {
	unlinkFromNode();
	_node = newNode;
	linkToNode();
}

void CNodeRef::linkToNode() {
	if (!_node)
		return;

	if (_node->_observerTail)
		_node->_observerTail->_next = this;

	_next = nullptr;
	_prev = _node->_observerTail;

	CNodeRef *oldHead = _node->_observerHead;

	_node->_observerTail = this;

	if (!oldHead)
		_node->_observerHead = this;

	_node->_observerCount++;
}

void CNodeRef::unlinkFromNode() {
	if (!_node)
		return;

	CNodeRef *head = _node->_observerHead;

	if (this == head) {
		_node->_observerHead = head->_next;

		if (head->_next)
			head->_next->_prev = nullptr;
		else
			_node->_observerTail = nullptr;

		head->_prev = nullptr;
		head->_next = nullptr;
	} else if (this == _node->_observerTail) {
		CNodeRef *tail = _node->_observerTail;

		_node->_observerTail = tail->_prev;

		if (tail->_prev)
			tail->_prev->_next = nullptr;
		else
			_node->_observerHead = nullptr;

		tail->_prev = nullptr;
		tail->_next = nullptr;
	} else if (_next && _prev) {
		_next->_prev = _prev;
		_prev->_next = _next;
	} else {
		if (_next || _prev)
			warning("CNodeRef::unlinkFromNode(): inconsistent observer list");

		return;
	}

	_node->_observerCount--;
	_prev = nullptr;
	_next = nullptr;
}

} // End of namespace Grim
