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

#ifndef GRIM_LAB_H
#define GRIM_LAB_H

#include "common/archive.h"

namespace Common {
	class File;
}

namespace Grim {

class Lab;
class SysStruct;

class LabEntry : public Common::ArchiveMember {
	Lab *_parent;
	Common::Path _name;
	uint32 _offset, _len;
public:
	LabEntry(const Common::Path &name, uint32 offset, uint32 len, Lab *parent);
	Common::String getName() const override { return _name.baseName(); }
	Common::String getFileName() const override { return _name.baseName(); }
	Common::Path getPathInArchive() const override { return _name; }
	Common::SeekableReadStream *createReadStream() const override;
	Common::SeekableReadStream *createReadStreamForAltStream(Common::AltStreamType altStreamType) const override { return nullptr; }
	friend class Lab;
};

class Lab : public Common::Archive {
public:
	bool open(const Common::Path &filename, bool keepStream = false);
	// The entries of a lab created with a host struct are allocated through its memory manager
	explicit Lab(SysStruct *hs = nullptr);
	~Lab();
	// Common::Archive implementation
	bool hasFile(const Common::Path &path) const override;
	int listMembers(Common::ArchiveMemberList &list) const override;
	const Common::ArchiveMemberPtr getMember(const Common::Path &path) const override;
	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override;

private:
	typedef Common::SharedPtr<LabEntry> LabEntryPtr;

	void parseGrimFileTable(Common::File *_f);
	void parseMonkey4FileTable(Common::File *_f);
	LabEntryPtr newEntry(const Common::Path &name, uint32 offset, uint32 len);

	SysStruct *_hs;
	Common::Path _labFileName;
	typedef Common::HashMap<Common::Path, LabEntryPtr, Common::Path::IgnoreCase_Hash, Common::Path::IgnoreCase_EqualTo> LabMap;
	LabMap _entries;
	Common::SeekableReadStream *_stream;
};

} // end of namespace Grim

#endif
