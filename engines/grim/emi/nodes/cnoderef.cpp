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
	prev = nullptr;
	next = nullptr;
	node = observedNode;
	linkToNode();
}

CNodeRef::~CNodeRef() {
	unlinkFromNode();
}

void CNodeRef::onNodeDestroyed() {
}

void CNodeRef::setNode(CNode *newNode) {
	unlinkFromNode();
	node = newNode;
	linkToNode();
}

void CNodeRef::linkToNode() {
	if (!node)
		return;

	if (node->observerTail)
		node->observerTail->next = this;

	next = nullptr;
	prev = node->observerTail;

	CNodeRef *oldHead = node->observerHead;

	node->observerTail = this;

	if (!oldHead)
		node->observerHead = this;

	node->observerCount++;
}

void CNodeRef::unlinkFromNode() {
	if (!node)
		return;

	CNodeRef *head = node->observerHead;

	if (this == head) {
		node->observerHead = head->next;

		if (head->next)
			head->next->prev = nullptr;
		else
			node->observerTail = nullptr;

		head->prev = nullptr;
		head->next = nullptr;
	} else if (this == node->observerTail) {
		CNodeRef *tail = node->observerTail;

		node->observerTail = tail->prev;

		if (tail->prev)
			tail->prev->next = nullptr;
		else
			node->observerHead = nullptr;

		tail->prev = nullptr;
		tail->next = nullptr;
	} else if (next && prev) {
		next->prev = prev;
		prev->next = next;
	} else {
		if (next || prev)
			warning("CNodeRef::unlinkFromNode(): inconsistent observer list");

		return;
	}

	node->observerCount--;
	prev = nullptr;
	next = nullptr;
}

} // End of namespace Grim
