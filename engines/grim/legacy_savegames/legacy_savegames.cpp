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

#include "common/endian.h"
#include "common/memstream.h"
#include "common/textconsole.h"

#include "engines/grim/legacy_savegames/legacy_savegames.h"

namespace Grim {

static const uint32 kTagHeader = MKTAG('R', 'S', 'A', 'V');
static const uint32 kTagFooter = MKTAG('E', 'S', 'A', 'V');
static const uint32 kTagThumbnail = MKTAG('S', 'I', 'M', 'G');
static const uint32 kTagSaveStrings = MKTAG('S', 'U', 'B', 'S');
static const uint32 kTagBitmaps = MKTAG('V', 'B', 'U', 'F');
static const uint32 kTagBitmapFonts = MKTAG('F', 'O', 'N', 'T');
static const uint32 kTagTtfFonts = MKTAG('T', 'T', 'F', ' ');
static const uint32 kTagObjectStates = MKTAG('S', 'T', 'A', 'T');
static const uint32 kTagSets = MKTAG('S', 'E', 'T', ' ');
static const uint32 kTagTextObjects = MKTAG('T', 'E', 'X', 'T');
static const uint32 kTagPrimitives = MKTAG('P', 'R', 'I', 'M');
static const uint32 kTagGrim = MKTAG('G', 'R', 'I', 'M');
static const uint32 kTagDriver = MKTAG('D', 'R', 'V', 'R');
static const uint32 kTagSound = MKTAG('I', 'M', 'U', 'S');
static const uint32 kTagMovie = MKTAG('S', 'M', 'U', 'S');
static const uint32 kTagIris = MKTAG('I', 'R', 'I', 'S');
static const uint32 kTagLua = MKTAG('L', 'U', 'A', 'S');

static const float kDefaultSoundMaxDist = 8.0f;
static const float kDefaultSoundPanFactor = 1.5f;
static const float kDefaultSoundMaxRadius = 0.8f;

LegacyReader::LegacyReader(Common::SeekableReadStream *stream, uint32 minorVersion, const Common::String &context) :
		_stream(stream), _minor(minorVersion), _context(context), _failed(false) {
	if (!_stream)
		fail("section not found");
}

LegacyReader::~LegacyReader() {
	delete _stream;
	_stream = nullptr;
}

uint32 LegacyReader::remaining() const {
	if (!_stream)
		return 0;

	return (uint32)(_stream->size() - _stream->pos());
}

void LegacyReader::fail(const char *reason) {
	if (!_failed) {
		if (_stream)
			warning("Legacy savegame: section %s: %s (offset %d)", _context.c_str(), reason, (int)_stream->pos());
		else
			warning("Legacy savegame: section %s: %s", _context.c_str(), reason);
	}
	_failed = true;
}

bool LegacyReader::finish() {
	if (_failed)
		return false;

	if (remaining() != 0) {
		fail(Common::String::format("%u bytes left unread", remaining()).c_str());
		return false;
	}

	return true;
}

bool LegacyReader::canRead(uint32 size) {
	if (_failed)
		return false;

	if (remaining() < size) {
		fail("unexpected end of section");
		return false;
	}

	return true;
}

byte LegacyReader::readByte() {
	if (!canRead(1))
		return 0;

	return _stream->readByte();
}

bool LegacyReader::readBool() {
	return readByte() != 0;
}

uint16 LegacyReader::readUint16() {
	if (!canRead(2))
		return 0;

	return _stream->readUint16LE();
}

int32 LegacyReader::readSint32() {
	if (!canRead(4))
		return 0;

	return _stream->readSint32LE();
}

uint32 LegacyReader::readUint32() {
	if (!canRead(4))
		return 0;

	return _stream->readUint32LE();
}

float LegacyReader::readFloat() {
	if (!canRead(4))
		return 0.0f;

	return _stream->readFloatLE();
}

Common::String LegacyReader::readString() {
	int32 length = readSint32();
	if (_failed || !checkCount(length, 1) || length == 0)
		return Common::String();

	Common::Array<char> text(length);
	readBytes(text.data(), length);
	return Common::String(text.data(), length);
}

LegacyVector3 LegacyReader::readVector3() {
	LegacyVector3 vector;
	vector.x = readFloat();
	vector.y = readFloat();
	vector.z = readFloat();
	return vector;
}

LegacyColor LegacyReader::readColor() {
	LegacyColor color;
	color.r = readByte();
	color.g = readByte();
	color.b = readByte();
	return color;
}

LegacyFontRef LegacyReader::readFontRef() {
	LegacyFontRef font;
	int32 value = readSint32();
	if (value == -1) {
		font.kind = kLegacyFontNone;
		font.id = 0;
	} else if (value == -2) {
		font.kind = kLegacyFontTtf;
		font.id = readSint32();
	} else {
		font.kind = kLegacyFontBitmap;
		font.id = value;
	}
	return font;
}

void LegacyReader::readBytes(void *buffer, uint32 size) {
	if (!canRead(size))
		return;

	_stream->read(buffer, size);
}

void LegacyReader::skip(uint32 size) {
	if (!canRead(size))
		return;

	_stream->skip(size);
}

int32 LegacyReader::readSignedCount(uint32 minItemSize, bool negativeIsEmpty) {
	int32 count = readSint32();
	if (_failed)
		return 0;

	if (count < 0 && negativeIsEmpty)
		return 0;

	if (!checkCount(count, minItemSize))
		return 0;

	return count;
}

uint32 LegacyReader::readUnsignedCount(uint32 minItemSize) {
	uint32 count = readUint32();
	if (_failed)
		return 0;

	if (!checkCount(count, minItemSize))
		return 0;

	return count;
}

bool LegacyReader::checkCount(int64 count, uint32 minItemSize) {
	if (_failed)
		return false;

	if (count < 0 || (uint64)count * minItemSize > remaining()) {
		fail("count out of range");
		return false;
	}

	return true;
}

LegacySaveFile::LegacySaveFile(Common::SeekableReadStream *stream, DisposeAfterUse::Flag dispose) :
		_stream(stream), _dispose(dispose), _major(0), _minor(0) {
}

LegacySaveFile::~LegacySaveFile() {
	if (_dispose == DisposeAfterUse::YES)
		delete _stream;
	_stream = nullptr;
}

bool LegacySaveFile::isLegacySave(Common::SeekableReadStream *stream) {
	int64 position = stream->pos();
	byte magic[4];
	uint32 count = stream->read(magic, 4);
	stream->seek(position);

	return count == 4 && READ_BE_UINT32(magic) == kTagHeader;
}

Common::String LegacySaveFile::tagToString(uint32 tag) {
	Common::String text;
	for (int shift = 24; shift >= 0; shift -= 8) {
		byte c = (tag >> shift) & 0xFF;
		if (c >= 32 && c < 127)
			text += (char)c;
		else
			text += '?';
	}
	return text;
}

bool LegacySaveFile::open() {
	_major = 0;
	_minor = 0;
	_sections.clear();

	int64 size = _stream->size();
	if (size < 12 || size > 0x7FFFFFFF) {
		warning("Legacy savegame: bad file size %d", (int)size);
		return false;
	}

	if (!_stream->seek(0)) {
		warning("Legacy savegame: cannot seek to the start of the file");
		return false;
	}

	if (_stream->readUint32BE() != kTagHeader) {
		warning("Legacy savegame: missing RSAV header");
		return false;
	}

	_major = _stream->readUint32BE();
	_minor = _stream->readUint32BE();
	if (_major != kLegacySaveMajorVersion) {
		warning("Legacy savegame: unsupported savegame major version %u", _major);
		return false;
	}

	if (_minor > kLegacySaveMaxMinorVersion) {
		warning("Legacy savegame: unsupported savegame minor version %u", _minor);
		return false;
	}

	for (;;) {
		int64 position = _stream->pos();
		if (size - position < 4) {
			warning("Legacy savegame: missing ESAV footer");
			return false;
		}

		uint32 tag = _stream->readUint32BE();
		if (tag == kTagFooter)
			break;

		if (size - position < 8) {
			warning("Legacy savegame: truncated header of section %s", tagToString(tag).c_str());
			return false;
		}

		LegacySectionInfo section;
		section.tag = tag;
		section.size = _stream->readUint32BE();
		section.offset = (uint32)position + 8;
		if (section.size > size - section.offset) {
			warning("Legacy savegame: section %s is truncated", tagToString(tag).c_str());
			return false;
		}

		if (findSection(tag))
			warning("Legacy savegame: duplicate section %s ignored", tagToString(tag).c_str());
		else
			_sections.push_back(section);

		if (!_stream->seek(section.offset + section.size)) {
			warning("Legacy savegame: cannot seek past section %s", tagToString(tag).c_str());
			return false;
		}
	}

	return true;
}

const LegacySectionInfo *LegacySaveFile::findSection(uint32 tag) const {
	for (uint i = 0; i < _sections.size(); ++i) {
		if (_sections[i].tag == tag)
			return &_sections[i];
	}
	return nullptr;
}

bool LegacySaveFile::hasSection(uint32 tag) const {
	return findSection(tag) != nullptr;
}

uint32 LegacySaveFile::getSectionSize(uint32 tag) const {
	const LegacySectionInfo *section = findSection(tag);
	if (!section)
		return 0;

	return section->size;
}

Common::SeekableReadStream *LegacySaveFile::openSection(uint32 tag) {
	const LegacySectionInfo *section = findSection(tag);
	if (!section)
		return nullptr;

	byte *data = (byte *)malloc(section->size ? section->size : 1);
	if (!data) {
		warning("Legacy savegame: cannot allocate %u bytes for section %s", section->size, tagToString(tag).c_str());
		return nullptr;
	}

	if (!_stream->seek(section->offset) || _stream->read(data, section->size) != section->size) {
		warning("Legacy savegame: cannot read section %s", tagToString(tag).c_str());
		free(data);
		return nullptr;
	}

	return new Common::MemoryReadStream(data, section->size, DisposeAfterUse::YES);
}

Common::SeekableReadStream *LegacySaveFile::openLuaSection() {
	return openSection(kTagLua);
}

bool LegacySaveFile::readThumbnail(Common::Array<byte> &pixels) {
	LegacyReader reader(openSection(kTagThumbnail), _minor, "SIMG");
	uint32 size = reader.remaining();
	pixels.resize(size);
	if (size > 0)
		reader.readBytes(pixels.data(), size);

	return reader.finish();
}

bool LegacySaveFile::readSaveStrings(Common::Array<Common::String> &strings) {
	strings.clear();

	LegacyReader reader(openSection(kTagSaveStrings), _minor, "SUBS");
	while (reader.remaining() > 0 && !reader.failed()) {
		int32 length = reader.readSint32();
		if (!reader.checkCount(length, 1))
			break;

		Common::Array<char> text(length + 1);
		reader.readBytes(text.data(), length);
		text[length] = '\0';
		strings.push_back(Common::String(text.data()));
	}

	return reader.finish();
}

bool LegacySaveFile::readBitmaps(Common::Array<LegacyBitmap> &bitmaps) {
	bitmaps.clear();

	LegacyReader reader(openSection(kTagBitmaps), _minor, "VBUF");
	uint32 count = reader.readUnsignedCount(12);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyBitmap bitmap;
		bitmap.id = reader.readSint32();
		bitmap.fileName = reader.readString();
		bitmap.currImage = reader.readSint32();
		if (reader.failed())
			break;

		bitmaps.push_back(bitmap);
	}

	return reader.finish();
}

bool LegacySaveFile::readBitmapFonts(Common::Array<LegacyBitmapFont> &fonts) {
	fonts.clear();

	LegacyReader reader(openSection(kTagBitmapFonts), _minor, "FONT");
	uint32 count = reader.readUnsignedCount(8);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyBitmapFont font;
		font.id = reader.readSint32();
		font.fileName = reader.readString();
		if (reader.failed())
			break;

		fonts.push_back(font);
	}

	return reader.finish();
}

bool LegacySaveFile::readTtfFonts(Common::Array<LegacyTtfFont> &fonts) {
	fonts.clear();

	LegacyReader reader(openSection(kTagTtfFonts), _minor, "TTF ");
	uint32 count = reader.readUnsignedCount(12);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyTtfFont font;
		font.id = reader.readSint32();
		font.fileName = reader.readString();
		font.size = reader.readSint32();
		if (reader.failed())
			break;

		fonts.push_back(font);
	}

	return reader.finish();
}

bool LegacySaveFile::readObjectStates(Common::Array<LegacyObjectState> &states) {
	states.clear();

	LegacyReader reader(openSection(kTagObjectStates), _minor, "STAT");
	uint32 count = reader.readUnsignedCount(21);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyObjectState state;
		state.id = reader.readSint32();
		state.visibility = reader.readBool();
		state.setupId = reader.readSint32();
		state.position = reader.readSint32();
		state.bitmapId = reader.readSint32();
		state.zBitmapId = reader.readSint32();
		if (reader.failed())
			break;

		states.push_back(state);
	}

	return reader.finish();
}

static void readSetup(LegacyReader &reader, LegacySetup &setup) {
	setup.name = reader.readString();
	setup.bkgndBitmapId = reader.readSint32();
	setup.bkgndZBitmapId = reader.readSint32();
	setup.pos = reader.readVector3();
	setup.interest = reader.readVector3();
	setup.roll = reader.readFloat();
	setup.fov = reader.readFloat();
	setup.nclip = reader.readFloat();
	setup.fclip = reader.readFloat();
}

static void readSector(LegacyReader &reader, LegacySector &sector) {
	sector.numVertices = reader.readSint32();
	sector.id = reader.readSint32();
	sector.type = reader.readSint32();
	sector.visible = reader.readBool();
	sector.height = reader.readFloat();
	sector.name = reader.readString();
	sector.vertices.clear();
	sector.origVertices.clear();
	if (reader.failed())
		return;

	if (sector.numVertices < 0) {
		reader.fail("negative vertex count");
		return;
	}

	// The polygon is closed, one more vertex than numVertices
	int64 numPoints = (int64)sector.numVertices + 1;
	if (!reader.checkCount(numPoints, 12))
		return;

	for (int64 i = 0; i < numPoints; ++i)
		sector.vertices.push_back(reader.readVector3());

	sector.normal = reader.readVector3();
	sector.shrinkRadius = reader.readFloat();
	sector.invalid = reader.readBool();
	if (sector.shrinkRadius != 0.0f && !sector.invalid) {
		if (!reader.checkCount(numPoints, 12))
			return;

		for (int64 i = 0; i < numPoints; ++i)
			sector.origVertices.push_back(reader.readVector3());
	}
}

static void readLight(LegacyReader &reader, LegacyLight &light) {
	light.name = reader.readString();
	light.enabled = reader.readBool();
	if (reader.minorVersion() > 7) {
		if (reader.minorVersion() >= 12) {
			light.type = (int32)reader.readUint32();
		} else {
			uint32 oldType = reader.readUint32();
			if (oldType == 1)
				light.type = kLegacyLightSpot;
			else if (oldType == 2)
				light.type = kLegacyLightDirect;
			else if (oldType == 3)
				light.type = kLegacyLightOmni;
			else if (oldType == 4)
				light.type = kLegacyLightAmbient;
			else
				light.type = kLegacyLightUnset;
		}
	} else {
		Common::String type = reader.readString();
		if (type == "spot")
			light.type = kLegacyLightSpot;
		else if (type == "omni")
			light.type = kLegacyLightOmni;
		else if (type == "direct")
			light.type = kLegacyLightDirect;
		else
			light.type = kLegacyLightUnset;
	}

	light.pos = reader.readVector3();
	light.dir = reader.readVector3();
	light.color = reader.readColor();
	light.intensity = reader.readFloat();
	light.umbraAngle = reader.readFloat();
	light.penumbraAngle = reader.readFloat();
	if (reader.minorVersion() >= 20) {
		light.falloffNear = reader.readFloat();
		light.falloffFar = reader.readFloat();
	} else {
		light.falloffNear = 0.0f;
		light.falloffFar = 0.0f;
	}
}

static void readSetShadow(LegacyReader &reader, LegacySetShadow &shadow) {
	shadow.name = reader.readString();
	shadow.shadowPoint = reader.readVector3();
	shadow.numSectors = reader.readSint32();
	shadow.sectorNames.clear();
	uint32 count = reader.readUnsignedCount(4);
	for (uint32 i = 0; i < count && !reader.failed(); ++i)
		shadow.sectorNames.push_back(reader.readString());
	shadow.color = reader.readColor();
}

static void readSet(LegacyReader &reader, LegacySet &set) {
	set.id = reader.readSint32();
	set.name = reader.readString();

	set.colormapNames.clear();
	int32 numCmaps = reader.readSignedCount(4, false);
	for (int32 i = 0; i < numCmaps && !reader.failed(); ++i)
		set.colormapNames.push_back(reader.readString());

	set.currSetup = reader.readUint32();
	set.locked = reader.readBool();
	set.enableLights = reader.readBool();
	set.minVolume = reader.readSint32();
	set.maxVolume = reader.readSint32();
	if (reader.minorVersion() >= 30) {
		set.soundMaxDist = reader.readFloat();
		set.soundPanFactor = reader.readFloat();
		set.soundMaxRadius = reader.readFloat();
	} else {
		set.soundMaxDist = kDefaultSoundMaxDist;
		set.soundPanFactor = kDefaultSoundPanFactor;
		set.soundMaxRadius = kDefaultSoundMaxRadius;
	}

	set.objectStateIds.clear();
	int32 numStates = reader.readSignedCount(4, false);
	for (int32 i = 0; i < numStates && !reader.failed(); ++i)
		set.objectStateIds.push_back(reader.readSint32());

	set.setups.clear();
	int32 numSetups = reader.readSignedCount(52, false);
	for (int32 i = 0; i < numSetups && !reader.failed(); ++i) {
		LegacySetup setup;
		readSetup(reader, setup);
		if (reader.failed())
			break;

		set.setups.push_back(setup);
	}

	// A set without sectors or lights can have been saved with a count of -1
	set.sectors.clear();
	int32 numSectors = reader.readSignedCount(48, true);
	for (int32 i = 0; i < numSectors && !reader.failed(); ++i) {
		LegacySector sector;
		readSector(reader, sector);
		if (reader.failed())
			break;

		set.sectors.push_back(sector);
	}

	set.lights.clear();
	int32 numLights = reader.readSignedCount(48, true);
	for (int32 i = 0; i < numLights && !reader.failed(); ++i) {
		LegacyLight light;
		readLight(reader, light);
		if (reader.failed())
			break;

		set.lights.push_back(light);
	}

	set.shadows.clear();
	if (reader.minorVersion() >= 19) {
		int32 numShadows = reader.readSignedCount(27, false);
		for (int32 i = 0; i < numShadows && !reader.failed(); ++i) {
			LegacySetShadow shadow;
			readSetShadow(reader, shadow);
			if (reader.failed())
				break;

			set.shadows.push_back(shadow);
		}
	}
}

bool LegacySaveFile::readSets(Common::Array<LegacySet> &sets) {
	sets.clear();

	LegacyReader reader(openSection(kTagSets), _minor, "SET ");
	uint32 count = reader.readUnsignedCount(40);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacySet set;
		readSet(reader, set);
		if (reader.failed())
			break;

		sets.push_back(set);
	}

	return reader.finish();
}

bool LegacySaveFile::readTextObjects(Common::Array<LegacyTextObject> &texts) {
	texts.clear();

	LegacyReader reader(openSection(kTagTextObjects), _minor, "TEXT");
	uint32 count = reader.readUnsignedCount(40);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyTextObject text;
		text.id = reader.readSint32();
		text.fgColor = reader.readColor();
		text.x = reader.readSint32();
		text.y = reader.readSint32();
		text.width = reader.readSint32();
		text.height = reader.readSint32();
		text.justify = reader.readSint32();
		text.numberLines = reader.readSint32();
		text.duration = reader.readSint32();
		text.blastDraw = reader.readBool();
		text.isSpeech = reader.readBool();
		text.elapsedTime = reader.readSint32();
		text.font = reader.readFontRef();
		text.textId = reader.readString();
		if (reader.failed())
			break;

		texts.push_back(text);
	}

	return reader.finish();
}

bool LegacySaveFile::readPrimitives(Common::Array<LegacyPrimitive> &primitives) {
	primitives.clear();

	LegacyReader reader(openSection(kTagPrimitives), _minor, "PRIM");
	uint32 count = reader.readUnsignedCount(31);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyPrimitive primitive;
		primitive.id = reader.readSint32();
		primitive.type = reader.readSint32();
		primitive.color = reader.readColor();
		primitive.filled = reader.readUint32();
		for (int j = 0; j < 4; ++j) {
			primitive.points[j].x = (int16)reader.readUint16();
			primitive.points[j].y = (int16)reader.readUint16();
		}
		if (reader.failed())
			break;

		primitives.push_back(primitive);
	}

	return reader.finish();
}

bool LegacySaveFile::readGrimState(LegacyGrimState &grim) {
	LegacyReader reader(openSection(kTagGrim), _minor, "GRIM");
	grim.mode = reader.readUint32();
	grim.previousMode = reader.readUint32();
	grim.selectedActorId = reader.readSint32();
	grim.sayLineDefaults.fgColor = reader.readColor();
	grim.sayLineDefaults.font = reader.readFontRef();
	grim.sayLineDefaults.height = reader.readSint32();
	grim.sayLineDefaults.justify = reader.readSint32();
	grim.sayLineDefaults.width = reader.readSint32();
	grim.sayLineDefaults.x = reader.readSint32();
	grim.sayLineDefaults.y = reader.readSint32();
	grim.sayLineDefaults.duration = reader.readSint32();
	if (_minor > 5)
		grim.movieSubtitleTextObjectId = reader.readSint32();
	else
		grim.movieSubtitleTextObjectId = 0;

	grim.currSetId = reader.readSint32();
	if (_minor > 4) {
		grim.hasMovieSetup = true;
		grim.movieSetup = reader.readString();
	} else {
		grim.hasMovieSetup = false;
		grim.movieSetup.clear();
	}

	return reader.finish();
}

bool LegacySaveFile::readDriverState(LegacyDriverState &driver) {
	LegacyReader reader(openSection(kTagDriver), _minor, "DRVR");
	driver.shadowColor = reader.readColor();
	driver.renderBitmaps = reader.readBool();
	driver.renderZBitmaps = reader.readBool();

	return reader.finish();
}

bool LegacySaveFile::readSoundData(Common::Array<byte> &data) {
	LegacyReader reader(openSection(kTagSound), _minor, "IMUS");
	uint32 size = reader.remaining();
	data.resize(size);
	if (size > 0)
		reader.readBytes(data.data(), size);

	return reader.finish();
}

bool LegacySaveFile::readMovieState(LegacyMovieState &movie) {
	LegacyReader reader(openSection(kTagMovie), _minor, "SMUS");
	movie.fileName = reader.readString();
	movie.frame = reader.readSint32();
	movie.movieTime = reader.readFloat();
	movie.videoFinished = reader.readBool();
	movie.videoLooping = reader.readBool();
	movie.x = reader.readSint32();
	movie.y = reader.readSint32();

	return reader.finish();
}

bool LegacySaveFile::readIrisState(LegacyIrisState &iris) {
	LegacyReader reader(openSection(kTagIris), _minor, "IRIS");
	iris.playing = reader.readBool();
	iris.direction = reader.readUint32();
	iris.x1 = reader.readSint32();
	iris.y1 = reader.readSint32();
	iris.x2 = reader.readSint32();
	iris.y2 = reader.readSint32();
	iris.length = reader.readSint32();
	iris.currTime = reader.readSint32();

	return reader.finish();
}

bool LegacySaveFile::readAll(LegacyCostumeProvider *costumes, LegacySaveData &data) {
	data.minorVersion = _minor;
	data.thumbnail.clear();
	data.saveStrings.clear();

	if (hasSection(kTagThumbnail) && !readThumbnail(data.thumbnail))
		return false;

	if (hasSection(kTagSaveStrings) && !readSaveStrings(data.saveStrings))
		return false;

	if (!readBitmaps(data.bitmaps))
		return false;

	if (!readBitmapFonts(data.bitmapFonts))
		return false;

	data.ttfFonts.clear();
	if (_minor >= 28 && !readTtfFonts(data.ttfFonts))
		return false;

	if (!readObjectStates(data.objectStates))
		return false;

	if (!readSets(data.sets))
		return false;

	if (!readTextObjects(data.textObjects))
		return false;

	if (!readPrimitives(data.primitives))
		return false;

	if (!readActors(costumes, data.isTalkingBackground, data.actors))
		return false;

	if (!readGrimState(data.grim))
		return false;

	if (!readDriverState(data.driver))
		return false;

	if (!readSoundData(data.soundData))
		return false;

	if (!readMovieState(data.movie))
		return false;

	if (!readIrisState(data.iris))
		return false;

	if (!data.grim.hasMovieSetup) {
		const LegacySet *currSet = nullptr;
		for (uint i = 0; i < data.sets.size(); ++i) {
			if (data.sets[i].id == data.grim.currSetId)
				currSet = &data.sets[i];
		}

		if (!currSet || currSet->currSetup >= currSet->setups.size()) {
			warning("Legacy savegame: cannot find the current setup of set %d", data.grim.currSetId);
			return false;
		}

		data.grim.movieSetup = currSet->setups[currSet->currSetup].name;
	}

	return true;
}

static Common::String truncateText(const Common::String &text, uint maxLength) {
	if (text.size() <= maxLength)
		return text;

	return Common::String(text.c_str(), maxLength) + "...";
}

Common::String describeLegacySave(const LegacySaveFile &file, const LegacySaveData &data) {
	static const uint kMaxActors = 25;
	static const uint kMaxCostumes = 6;

	Common::String text;
	text += Common::String::format("major %u\n", file.getMajor());
	text += Common::String::format("minor %u\n", file.getMinor());
	for (uint i = 0; i < file.getNumSections(); ++i) {
		const LegacySectionInfo &section = file.getSectionInfo(i);
		text += Common::String::format("section %s %u\n", LegacySaveFile::tagToString(section.tag).c_str(), section.size);
	}

	for (uint i = 0; i < data.saveStrings.size(); ++i)
		text += Common::String::format("saveString %u %s\n", i, truncateText(data.saveStrings[i], 100).c_str());

	text += Common::String::format("thumbnail %u\n", data.thumbnail.size());
	text += Common::String::format("bitmaps %u\n", data.bitmaps.size());
	text += Common::String::format("bitmapFonts %u\n", data.bitmapFonts.size());
	text += Common::String::format("ttfFonts %u\n", data.ttfFonts.size());
	text += Common::String::format("objectStates %u\n", data.objectStates.size());
	text += Common::String::format("sets %u\n", data.sets.size());
	text += Common::String::format("textObjects %u\n", data.textObjects.size());
	text += Common::String::format("primitives %u\n", data.primitives.size());
	text += Common::String::format("actors %u\n", data.actors.size());

	for (uint i = 0; i < data.sets.size(); ++i) {
		const LegacySet &set = data.sets[i];
		text += Common::String::format("set id=%d name=%s setups=%u sectors=%u lights=%u shadows=%u states=%u currSetup=%u\n",
				set.id, set.name.c_str(), set.setups.size(), set.sectors.size(), set.lights.size(),
				set.shadows.size(), set.objectStateIds.size(), set.currSetup);
	}

	Common::String currSetName = "?";
	for (uint i = 0; i < data.sets.size(); ++i) {
		if (data.sets[i].id == data.grim.currSetId)
			currSetName = data.sets[i].name;
	}
	text += Common::String::format("currentSet id=%d name=%s\n", data.grim.currSetId, currSetName.c_str());
	text += Common::String::format("mode %u previousMode %u\n", data.grim.mode, data.grim.previousMode);
	text += Common::String::format("selectedActor %d\n", data.grim.selectedActorId);
	text += Common::String::format("movieSetup %s\n", data.grim.movieSetup.c_str());

	for (uint i = 0; i < data.actors.size() && i < kMaxActors; ++i) {
		const LegacyActor &actor = data.actors[i];
		Common::String costumes;
		for (uint j = 0; j < actor.costumes.size() && j < kMaxCostumes; ++j) {
			if (j > 0)
				costumes += ",";
			costumes += actor.costumes[j].fileName;
		}
		if (actor.costumes.size() > kMaxCostumes)
			costumes += ",...";

		text += Common::String::format("actor id=%d name=%s set=%s costumes=%s\n", actor.id,
				truncateText(actor.name, 40).c_str(), actor.setName.c_str(), costumes.c_str());
	}
	if (data.actors.size() > kMaxActors)
		text += Common::String::format("actorsOmitted %u\n", data.actors.size() - kMaxActors);

	text += Common::String::format("driver shadow=%d,%d,%d bitmaps=%d zBitmaps=%d\n",
			data.driver.shadowColor.r, data.driver.shadowColor.g, data.driver.shadowColor.b,
			data.driver.renderBitmaps ? 1 : 0, data.driver.renderZBitmaps ? 1 : 0);
	text += Common::String::format("soundData %u\n", data.soundData.size());
	text += Common::String::format("movie file=%s frame=%d finished=%d looping=%d x=%d y=%d\n",
			data.movie.fileName.c_str(), data.movie.frame, data.movie.videoFinished ? 1 : 0,
			data.movie.videoLooping ? 1 : 0, data.movie.x, data.movie.y);
	text += Common::String::format("iris playing=%d direction=%u rect=%d,%d,%d,%d length=%d time=%d\n",
			data.iris.playing ? 1 : 0, data.iris.direction, data.iris.x1, data.iris.y1,
			data.iris.x2, data.iris.y2, data.iris.length, data.iris.currTime);

	return text;
}

bool legacyReadSaveStrings(Common::SeekableReadStream *stream, Common::Array<Common::String> &strings) {
	LegacySaveFile file(stream, DisposeAfterUse::NO);

	return file.open() && file.readSaveStrings(strings);
}

bool legacyReadThumbnail(Common::SeekableReadStream *stream, Common::Array<byte> &pixels) {
	LegacySaveFile file(stream, DisposeAfterUse::NO);

	return file.open() && file.readThumbnail(pixels);
}

} // End of namespace Grim
