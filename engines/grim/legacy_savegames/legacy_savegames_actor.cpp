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
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/legacy_savegames/legacy_savegames.h"

namespace Grim {

static const uint32 kTagActors = MKTAG('A', 'C', 'T', 'R');

static const uint32 kTagMainModel = MKTAG('M', 'M', 'D', 'L');
static const uint32 kTagModel = MKTAG('M', 'O', 'D', 'L');
static const uint32 kTagColormap = MKTAG('C', 'M', 'A', 'P');
static const uint32 kTagKeyframe = MKTAG('K', 'E', 'Y', 'F');
static const uint32 kTagMesh = MKTAG('M', 'E', 'S', 'H');
static const uint32 kTagLuaVar = MKTAG('L', 'U', 'A', 'V');
static const uint32 kTagSoundLoop = MKTAG('I', 'M', 'L', 'S');
static const uint32 kTagSoundSingle = MKTAG('I', 'M', 'S', 'S');
static const uint32 kTagBitmap = MKTAG('B', 'K', 'N', 'D');
static const uint32 kTagMaterial = MKTAG('M', 'A', 'T', ' ');
static const uint32 kTagSprite = MKTAG('S', 'P', 'R', 'T');
static const uint32 kTagAnim = MKTAG('A', 'N', 'I', 'M');

typedef Common::HashMap<Common::String, LegacyCostumeLayout> LegacyLayoutCache;

static bool isKnownComponentTag(uint32 tag) {
	return tag == kTagMainModel || tag == kTagModel || tag == kTagColormap ||
		tag == kTagKeyframe || tag == kTagMesh || tag == kTagLuaVar ||
		tag == kTagSoundLoop || tag == kTagSoundSingle || tag == kTagBitmap ||
		tag == kTagMaterial || tag == kTagSprite || tag == kTagAnim;
}

static bool costumeFailure(const Common::String &name, const char *reason) {
	warning("Legacy savegame: costume %s: %s", name.c_str(), reason);
	return false;
}

static bool loadCostumeLines(Common::SeekableReadStream *stream, Common::Array<Common::String> &lines) {
	int64 size = stream->size();
	if (size < 0 || size > 0x7FFFFFFF)
		return false;

	Common::Array<char> data((uint)size + 1);
	if (size > 0 && stream->read(data.data(), (uint32)size) != (uint32)size)
		return false;

	// A last line without a newline is not part of the file
	uint start = 0;
	for (uint i = 0; i < (uint)size; ++i) {
		if (data[i] != '\n')
			continue;

		uint end = start;
		while (end < i && data[end] != '#')
			++end;
		while (end > start && Common::isSpace(data[end - 1]))
			--end;

		if (end > start) {
			Common::String line(data.data() + start, end - start);
			line.toLowercase();
			lines.push_back(line);
		}
		start = i + 1;
	}

	return true;
}

static bool nextToken(const Common::String &line, uint &pos, const char *separators, Common::String &token) {
	while (pos < line.size() && strchr(separators, line[pos]))
		++pos;

	if (pos >= line.size())
		return false;

	uint start = pos;
	while (pos < line.size() && !strchr(separators, line[pos]))
		++pos;

	token = Common::String(line.c_str() + start, pos - start);
	return true;
}

static bool parseInteger(const Common::String &token, int32 &value) {
	uint i = 0;
	bool negative = false;
	if (i < token.size() && (token[i] == '-' || token[i] == '+')) {
		negative = (token[i] == '-');
		++i;
	}

	if (i >= token.size())
		return false;

	int64 result = 0;
	for (; i < token.size(); ++i) {
		if (token[i] < '0' || token[i] > '9')
			return false;

		result = result * 10 + (token[i] - '0');
		if (result > 0x7FFFFFFF)
			return false;
	}

	value = (int32)(negative ? -result : result);
	return true;
}

static bool expectLine(const Common::Array<Common::String> &lines, uint &index, const char *expected, const Common::String &name) {
	if (index >= lines.size() || lines[index] != expected)
		return costumeFailure(name, Common::String::format("expected \"%s\"", expected).c_str());

	++index;
	return true;
}

// Lines like " numtags 3"
static bool scanCountLine(const Common::Array<Common::String> &lines, uint &index, const char *keyword, int32 &count, const Common::String &name) {
	uint pos = 0;
	Common::String token;
	if (index >= lines.size() || !nextToken(lines[index], pos, " \t", token) || token != keyword ||
			!nextToken(lines[index], pos, " \t,:", token) || !parseInteger(token, count) || count < 0)
		return costumeFailure(name, Common::String::format("bad \"%s\" line", keyword).c_str());

	++index;
	if ((uint32)count > lines.size() - index)
		return costumeFailure(name, Common::String::format("\"%s\" is larger than the file", keyword).c_str());

	return true;
}

// Lines like " 0 'mmdl'"
static bool scanTagLine(const Common::String &line, int32 &which, uint32 &tag) {
	uint pos = 0;
	Common::String token;
	if (!nextToken(line, pos, " \t,:", token) || !parseInteger(token, which))
		return false;

	while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
		++pos;

	if (pos + 6 > line.size() || line[pos] != '\'' || line[pos + 5] != '\'')
		return false;

	tag = 0;
	for (uint i = 0; i < 4; ++i)
		tag = (tag << 8) | (byte)toupper((byte)line[pos + 1 + i]);

	return true;
}

bool parseLegacyCostumeFile(Common::SeekableReadStream *stream, const Common::String &name, LegacyCostumeLayout &layout) {
	layout.componentTags.clear();
	layout.components.clear();
	layout.chores.clear();

	Common::Array<Common::String> lines;
	if (!loadCostumeLines(stream, lines))
		return costumeFailure(name, "cannot read the file");

	uint index = 0;
	if (!expectLine(lines, index, "costume v0.1", name) || !expectLine(lines, index, "section tags", name))
		return false;

	int32 numTags;
	if (!scanCountLine(lines, index, "numtags", numTags, name))
		return false;

	Common::Array<uint32> tags;
	for (int32 i = 0; i < numTags; ++i)
		tags.push_back(0);

	for (int32 i = 0; i < numTags; ++i) {
		int32 which;
		uint32 tag;
		if (!scanTagLine(lines[index], which, tag) || which < 0 || which >= numTags)
			return costumeFailure(name, "bad tag line");

		tags[which] = tag;
		++index;
	}

	if (!expectLine(lines, index, "section components", name))
		return false;

	int32 numComponents;
	if (!scanCountLine(lines, index, "numcomponents", numComponents, name))
		return false;

	for (int32 i = 0; i < numComponents; ++i) {
		LegacyCosComponent component;
		component.parent = -1;

		layout.componentTags.push_back(0);
		layout.components.push_back(component);
	}

	for (int32 i = 0; i < numComponents; ++i) {
		const Common::String &line = lines[index];
		uint pos = 0;
		Common::String tokens[4];
		for (int j = 0; j < 4; ++j) {
			if (!nextToken(line, pos, " \t", tokens[j]))
				return costumeFailure(name, "bad component line");
		}

		int32 id, tagId;
		if (!parseInteger(tokens[0], id) || !parseInteger(tokens[1], tagId) ||
				id < 0 || id >= numComponents || tagId < 0 || tagId >= numTags)
			return costumeFailure(name, "bad component line");

		uint32 tag = tags[tagId];
		if (!isKnownComponentTag(tag)) {
			warning("Legacy savegame: costume %s: unknown component tag %s", name.c_str(), LegacySaveFile::tagToString(tag).c_str());
			tag = 0;
		}

		layout.componentTags[id] = tag;

		// The parent and the name only matter to the importer, a bad parent is the same as none
		int32 parent;
		if (parseInteger(tokens[3], parent) && parent >= 0 && parent < numComponents)
			layout.components[id].parent = parent;

		while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
			++pos;

		layout.components[id].name = Common::String(line.c_str() + pos, line.size() - pos);
		++index;
	}

	if (!expectLine(lines, index, "section chores", name))
		return false;

	int32 numChores;
	if (!scanCountLine(lines, index, "numchores", numChores, name))
		return false;

	Common::Array<bool> seen;
	for (int32 i = 0; i < numChores; ++i)
		seen.push_back(false);

	for (int32 i = 0; i < numChores; ++i) {
		const Common::String &line = lines[index];
		uint pos = 0;
		Common::String tokens[4];
		for (int j = 0; j < 4; ++j) {
			if (!nextToken(line, pos, " \t,:", tokens[j]))
				return costumeFailure(name, "bad chore line");
		}

		LegacyCosChore chore;
		if (!parseInteger(tokens[0], chore.id) || !parseInteger(tokens[1], chore.length) ||
				!parseInteger(tokens[2], chore.tracks) || chore.id < 0 || chore.id >= numChores)
			return costumeFailure(name, "bad chore line");

		if (seen[chore.id])
			return costumeFailure(name, "duplicate chore id");

		seen[chore.id] = true;
		chore.name = tokens[3];
		layout.chores.push_back(chore);
		++index;
	}

	return true;
}

static const LegacyCostumeLayout *getCostumeLayout(LegacyLayoutCache &cache, LegacyCostumeProvider *provider, const Common::String &fileName) {
	Common::String key = fileName;
	key.toLowercase();

	LegacyLayoutCache::iterator it = cache.find(key);
	if (it != cache.end())
		return &it->_value;

	if (!provider) {
		warning("Legacy savegame: no costume provider to open %s", fileName.c_str());
		return nullptr;
	}

	Common::SeekableReadStream *stream = provider->openCostumeFile(fileName);
	if (!stream) {
		warning("Legacy savegame: cannot open costume file %s", fileName.c_str());
		return nullptr;
	}

	LegacyCostumeLayout layout;
	bool parsed = parseLegacyCostumeFile(stream, fileName, layout);
	delete stream;
	stream = nullptr;
	if (!parsed)
		return nullptr;

	cache.setVal(key, layout);
	return &cache.find(key)->_value;
}

static void readAnimState(LegacyReader &reader, LegacyAnimState &anim) {
	anim.active = reader.readBool();
	anim.repeatMode = reader.readSint32();
	anim.time = reader.readSint32();
	anim.fadeMode = reader.readSint32();
	anim.fade = reader.readFloat();
	anim.fadeLength = reader.readSint32();
	anim.paused = reader.readBool();
}

static void readComponent(LegacyReader &reader, int32 index, uint32 tag, LegacyComponent &component) {
	component.index = index;
	component.tag = tag;
	component.visible = reader.readBool();
	if (reader.minorVersion() < 14)
		reader.readVector3();

	component.meshVisible = false;
	component.hasMatrixPosition = false;
	component.matrixPosition.x = 0.0f;
	component.matrixPosition.y = 0.0f;
	component.matrixPosition.z = 0.0f;
	component.anim.active = false;
	component.anim.repeatMode = 0;
	component.anim.time = 0;
	component.anim.fadeMode = 0;
	component.anim.fade = 0.0f;
	component.anim.fadeLength = 0;
	component.anim.paused = false;
	component.activeTexture = 0;
	component.spriteVisible = false;

	if (tag == kTagMesh) {
		component.meshVisible = reader.readBool();
		if (reader.minorVersion() >= 14) {
			component.hasMatrixPosition = true;
			component.matrixPosition = reader.readVector3();
		}
	} else if (tag == kTagKeyframe) {
		readAnimState(reader, component.anim);
	} else if (tag == kTagMaterial) {
		component.activeTexture = reader.readSint32();
	} else if (tag == kTagSprite) {
		component.spriteVisible = reader.readBool();
		component.activeTexture = reader.readSint32();
	}
}

static void readHead(LegacyReader &reader, LegacyHead &head) {
	head.joint1Node = reader.readSint32();
	head.joint2Node = reader.readSint32();
	head.joint3Node = reader.readSint32();
	head.maxPitch = reader.readFloat();
	head.maxYaw = reader.readFloat();
	head.maxRoll = reader.readFloat();
	if (reader.minorVersion() >= 2) {
		head.hasJointAngles = true;
		for (int i = 0; i < 3; ++i) {
			head.joints[i].pitch = reader.readFloat();
			head.joints[i].yaw = reader.readFloat();
			head.joints[i].roll = reader.readFloat();
		}
	} else {
		head.hasJointAngles = false;
		reader.readFloat();
		reader.readFloat();
		for (int i = 0; i < 3; ++i) {
			head.joints[i].pitch = 0.0f;
			head.joints[i].yaw = 0.0f;
			head.joints[i].roll = 0.0f;
		}
	}
}

static void readCostume(LegacyReader &reader, const LegacyCostumeLayout &layout, LegacyCostume &costume) {
	costume.hasColormap = reader.readBool();
	if (costume.hasColormap)
		costume.colormapName = reader.readString();
	else
		costume.colormapName.clear();

	costume.chores.clear();
	if (!reader.checkCount(layout.chores.size(), 7))
		return;

	for (uint i = 0; i < layout.chores.size(); ++i) {
		LegacyChore chore;
		chore.hasPlayed = reader.readBool();
		chore.playing = reader.readBool();
		chore.looping = reader.readBool();
		chore.currTime = reader.readSint32();
		if (reader.minorVersion() >= 10)
			chore.paused = reader.readBool();
		else
			chore.paused = false;

		costume.chores.push_back(chore);
	}

	costume.components.clear();
	for (uint i = 0; i < layout.componentTags.size() && !reader.failed(); ++i) {
		if (layout.componentTags[i] == 0)
			continue;

		LegacyComponent component;
		readComponent(reader, i, layout.componentTags[i], component);
		costume.components.push_back(component);
	}

	costume.playingChoreIds.clear();
	uint32 numPlayingChores = reader.readUnsignedCount(4);
	for (uint32 i = 0; i < numPlayingChores && !reader.failed(); ++i) {
		int32 id = reader.readSint32();
		if (id < 0 || (uint)id >= layout.chores.size()) {
			reader.fail("playing chore id out of range");
			return;
		}

		costume.playingChoreIds.push_back(id);
	}

	costume.lookAtRate = reader.readFloat();
	readHead(reader, costume.head);
}

static void readActionChore(LegacyReader &reader, LegacyActionChore &chore) {
	chore.hasCostume = reader.readBool();
	if (chore.hasCostume)
		chore.costumeFileName = reader.readString();
	else
		chore.costumeFileName.clear();

	chore.chore = reader.readSint32();
}

static void readActorShadow(LegacyReader &reader, LegacyActorShadow &shadow) {
	shadow.name = reader.readString();
	shadow.pos = reader.readVector3();

	shadow.planes.clear();
	uint32 numPlanes = reader.readUnsignedCount(8);
	for (uint32 i = 0; i < numPlanes && !reader.failed(); ++i) {
		LegacyShadowPlane plane;
		plane.setName = reader.readString();
		plane.sectorName = reader.readString();
		shadow.planes.push_back(plane);
	}

	int32 maskSize = reader.readSignedCount(1, false);
	reader.skip(maskSize);
	shadow.active = reader.readBool();
	shadow.dontNegate = reader.readBool();
}

static void readActor(LegacyReader &reader, LegacyCostumeProvider *provider, LegacyLayoutCache &cache, LegacyActor &actor) {
	actor.name = reader.readString();
	actor.setName = reader.readString();
	actor.talkColor = reader.readColor();
	actor.pos = reader.readVector3();
	actor.pitch = reader.readFloat();
	actor.yaw = reader.readFloat();
	actor.roll = reader.readFloat();
	actor.walkRate = reader.readFloat();
	actor.turnRate = reader.readFloat();
	if (reader.minorVersion() > 6)
		actor.turnRateMultiplier = reader.readFloat();
	else
		actor.turnRateMultiplier = 1.0f;

	actor.followBoxes = reader.readBool();
	actor.reflectionAngle = reader.readFloat();
	actor.visible = reader.readBool();
	actor.lookingMode = reader.readBool();
	actor.scale = reader.readFloat();
	actor.timeScale = reader.readFloat();
	actor.puckOrient = reader.readBool();
	actor.talkSoundName = reader.readString();
	actor.talking = reader.readBool();
	actor.backgroundTalk = reader.readBool();
	if (reader.minorVersion() >= 30)
		actor.talkSoundId = reader.readSint32();
	else
		actor.talkSoundId = 0;

	actor.collisionMode = reader.readUint32();
	actor.collisionScale = reader.readFloat();
	actor.hasLipSync = reader.readBool();
	if (actor.hasLipSync)
		actor.lipSyncFileName = reader.readString();
	else
		actor.lipSyncFileName.clear();

	actor.costumes.clear();
	uint32 numCostumes = reader.readUnsignedCount(40);
	for (uint32 i = 0; i < numCostumes && !reader.failed(); ++i) {
		LegacyCostume costume;
		costume.fileName = reader.readString();

		costume.prevFileNames.clear();
		int32 depth = reader.readSignedCount(4, false);
		for (int32 j = 0; j < depth && !reader.failed(); ++j)
			costume.prevFileNames.push_back(reader.readString());

		if (reader.failed())
			return;

		const LegacyCostumeLayout *layout = getCostumeLayout(cache, provider, costume.fileName);
		if (!layout) {
			reader.fail(Common::String::format("actor %s: cannot get the layout of costume %s", actor.name.c_str(), costume.fileName.c_str()).c_str());
			return;
		}

		readCostume(reader, *layout, costume);
		if (reader.failed())
			return;

		actor.costumes.push_back(costume);
	}

	actor.turning = reader.readBool();
	if (reader.minorVersion() > 25)
		actor.singleTurning = reader.readBool();
	else
		actor.singleTurning = false;

	actor.moveYaw = reader.readFloat();
	if (reader.minorVersion() > 6) {
		actor.movePitch = reader.readFloat();
		actor.moveRoll = reader.readFloat();
	} else {
		actor.movePitch = actor.pitch;
		actor.moveRoll = actor.roll;
	}

	actor.walking = reader.readBool();
	actor.destPos = reader.readVector3();
	readActionChore(reader, actor.restChore);
	readActionChore(reader, actor.walkChore);
	actor.walkedLast = reader.readBool();
	actor.walkedCur = reader.readBool();
	readActionChore(reader, actor.leftTurnChore);
	readActionChore(reader, actor.rightTurnChore);
	actor.lastTurnDir = reader.readSint32();
	actor.currTurnDir = reader.readSint32();
	for (int i = 0; i < 10; ++i)
		readActionChore(reader, actor.talkChores[i]);

	actor.talkAnim = reader.readSint32();
	readActionChore(reader, actor.mumbleChore);

	actor.shadows.clear();
	int numShadows = 8;
	if (reader.minorVersion() < 24)
		numShadows = 5;

	for (int i = 0; i < numShadows && !reader.failed(); ++i) {
		LegacyActorShadow shadow;
		readActorShadow(reader, shadow);
		if (reader.failed())
			return;

		actor.shadows.push_back(shadow);
	}

	actor.activeShadowSlot = reader.readSint32();
	actor.sayLineText = reader.readSint32();
	actor.lookAtVector = reader.readVector3();

	actor.path.clear();
	uint32 numPathPoints = reader.readUnsignedCount(12);
	for (uint32 i = 0; i < numPathPoints && !reader.failed(); ++i)
		actor.path.push_back(reader.readVector3());

	if (reader.minorVersion() >= 4)
		actor.drawnToClean = reader.readBool();
	else
		actor.drawnToClean = false;

	if (reader.minorVersion() >= 27)
		actor.fakeUnbound = reader.readBool();
	else
		actor.fakeUnbound = false;
}

bool LegacySaveFile::readActors(LegacyCostumeProvider *costumes, bool &isTalkingBackground, Common::Array<LegacyActor> &actors) {
	actors.clear();

	LegacyReader reader(openSection(kTagActors), _minor, "ACTR");
	isTalkingBackground = reader.readBool();

	LegacyLayoutCache cache;
	uint32 count = reader.readUnsignedCount(64);
	for (uint32 i = 0; i < count && !reader.failed(); ++i) {
		LegacyActor actor;
		actor.id = reader.readSint32();
		readActor(reader, costumes, cache, actor);
		if (reader.failed())
			break;

		actors.push_back(actor);
	}

	return reader.finish();
}

} // End of namespace Grim
