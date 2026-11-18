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

#ifndef GRIM_EMI_LCONTAINERS_H
#define GRIM_EMI_LCONTAINERS_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/list.h"

#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

// Containers whose memory is counted by the memory services, like the containers of the game. Each one makes the services
// count what the container of Common holds: the capacity of the array, a block for each node of the list and of the map,
// and the head node of the list

template<class T>
class LArray : public Common::Array<T> {
public:
	typedef Common::Array<T> Base;
	typedef typename Base::size_type size_type;
	typedef typename Base::iterator iterator;

	LArray() : Base() {
	}

	LArray(const LArray &other) : Base(other) {
		sync();
	}

	LArray(const Base &other) : Base(other) {
		sync();
	}

	explicit LArray(size_type count) : Base(count) {
		sync();
	}

	LArray(size_type count, const T &value) : Base(count, value) {
		sync();
	}

	LArray &operator=(const LArray &other) {
		Base::operator=(other);
		sync();

		return *this;
	}

	LArray &operator=(const Base &other) {
		Base::operator=(other);
		sync();

		return *this;
	}

	void push_back(const T &element) {
		Base::push_back(element);
		sync();
	}

	void push_back(const Base &array) {
		Base::push_back(array);
		sync();
	}

	void insert_at(size_type idx, const T &element) {
		Base::insert_at(idx, element);
		sync();
	}

	void insert_at(size_type idx, const Base &array) {
		Base::insert_at(idx, array);
		sync();
	}

	iterator insert(iterator pos, const T &element) {
		iterator result = Base::insert(pos, element);
		sync();

		return result;
	}

	void reserve(size_type newCapacity) {
		Base::reserve(newCapacity);
		sync();
	}

	void resize(size_type newSize) {
		Base::resize(newSize);
		sync();
	}

	void resize(size_type newSize, const T value) {
		Base::resize(newSize, value);
		sync();
	}

	void clear() {
		Base::clear();
		sync();
	}

private:
	void sync() {
		_counted.setSingleBlock(this->_capacity * sizeof(T));
	}

	LCountedBlocks _counted;
};

template<class T>
class LList : public Common::List<T> {
public:
	typedef Common::List<T> Base;
	typedef typename Base::iterator iterator;
	typedef typename Base::Node Node;

	LList() : Base() {
		_head.setSingleBlock(sizeof(Common::ListInternal::NodeBase));
	}

	LList(const LList &other) : Base(other) {
		_head.setSingleBlock(sizeof(Common::ListInternal::NodeBase));
		sync();
	}

	LList &operator=(const LList &other) {
		Base::operator=(other);
		sync();

		return *this;
	}

	iterator insert(iterator pos, const T &element) {
		iterator result = Base::insert(pos, element);
		sync();

		return result;
	}

	template<class iterator2>
	void insert(iterator pos, iterator2 first, iterator2 last) {
		Base::insert(pos, first, last);
		sync();
	}

	iterator erase(iterator pos) {
		iterator result = Base::erase(pos);
		sync();

		return result;
	}

	iterator reverse_erase(iterator pos) {
		iterator result = Base::reverse_erase(pos);
		sync();

		return result;
	}

	iterator erase(iterator first, iterator last) {
		iterator result = Base::erase(first, last);
		sync();

		return result;
	}

	void remove(const T &val) {
		Base::remove(val);
		sync();
	}

	void push_front(const T &element) {
		Base::push_front(element);
		sync();
	}

	void push_back(const T &element) {
		Base::push_back(element);
		sync();
	}

	void pop_front() {
		Base::pop_front();
		sync();
	}

	void pop_back() {
		Base::pop_back();
		sync();
	}

	void clear() {
		Base::clear();
		sync();
	}

private:
	void sync() {
		_nodes.setBlockCount(this->size(), sizeof(Node));
	}

	LCountedBlocks _head;
	LCountedBlocks _nodes;
};

template<class Key, class Val, class HashFunc = Common::Hash<Key>, class EqualFunc = Common::EqualTo<Key> >
class LHashMap : public Common::HashMap<Key, Val, HashFunc, EqualFunc> {
public:
	typedef Common::HashMap<Key, Val, HashFunc, EqualFunc> Base;
	typedef typename Base::iterator iterator;

	LHashMap() : Base() {
	}

	LHashMap(const LHashMap &other) : Base(other) {
		sync();
	}

	LHashMap &operator=(const LHashMap &other) {
		Base::operator=(other);
		sync();

		return *this;
	}

	Val &operator[](const Key &key) {
		Val &result = Base::operator[](key);
		sync();

		return result;
	}

	const Val &operator[](const Key &key) const {
		return Base::operator[](key);
	}

	void setVal(const Key &key, const Val &val) {
		Base::setVal(key, val);
		sync();
	}

	void clear(bool shrinkArray = false) {
		Base::clear(shrinkArray);
		sync();
	}

	void erase(iterator entry) {
		Base::erase(entry);
		sync();
	}

	void erase(const Key &key) {
		Base::erase(key);
		sync();
	}

private:
	void sync() {
		_nodes.setBlockCount(this->size(), sizeof(typename Base::Node));
	}

	LCountedBlocks _nodes;
};

} // End of namespace Grim

#endif
