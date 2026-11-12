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

#include "common/algorithm.h"
#include "common/endian.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/legacy_savegames/legacy_savegames_import.h"
#include "engines/grim/puck.h"
#include "engines/grim/res.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/costume/sputcostume_components.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/walkplane.h"

namespace Grim {

static const uint32 kTagMainModel = MKTAG('M', 'M', 'D', 'L');
static const uint32 kTagModel = MKTAG('M', 'O', 'D', 'L');
static const uint32 kTagKeyframe = MKTAG('K', 'E', 'Y', 'F');
static const uint32 kTagMesh = MKTAG('M', 'E', 'S', 'H');
static const uint32 kTagMaterial = MKTAG('M', 'A', 'T', ' ');
static const uint32 kTagSprite = MKTAG('S', 'P', 'R', 'T');

// Values of the legacy engine
static const int kLegacyRepeatOnce = 0;
static const int kLegacyRepeatLooping = 1;
static const int kLegacyRepeatPauseAtEnd = 2;
static const int kLegacyRepeatFadeAtEnd = 3;
static const int kLegacyFadeNone = 0;
static const int kLegacyFadeIn = 1;
static const int kLegacyFadeOut = 2;
static const int kLegacyMaxCollisionMode = 2;

// Bits of SputActor::flags and SputActor::mode
static const int kActorHidden = 2;
static const int kActorPuckOrient = 0x10;
static const int kActorWalking = 1;
static const int kActorLooking = 2;

// Bits of the status of a puppet track
static const uint kTrackInUse = 1;
static const uint kTrackPlaying = 2;
static const uint kTrackFadingIn = 4;
static const uint kTrackFadingOut = 8;
static const uint kTrackPaused = 0x10;
static const uint kTrackRemoveAtEnd = 0x20;
static const uint kTrackHoldAtEnd = 0x40;
static const uint kTrackFadeOutAtEnd = 0x80;

static const int kNumShadows = 6;
static const int kMaxPathPoints = 30;
static const uint kMaxSetNameSize = 32;
static const int kNumTalkChores = 10;

static bool tagsMatch(int fourcc, uint32 legacyTag) {
	char letters[5];

	WRITE_LE_UINT32(letters, (uint32)fourcc);
	letters[4] = '\0';

	return LegacySaveFile::tagToString(legacyTag).equalsIgnoreCase(letters);
}

static bool namesMatch(const char *name, const Common::String &legacyName) {
	Common::String text(name);

	while (!text.empty() && Common::isSpace(text.lastChar()))
		text.deleteLastChar();

	return text.equalsIgnoreCase(legacyName);
}

static int packColor(const LegacyColor &color) {
	return color.r | (color.g << 8) | (color.b << 16);
}

static bool hasStateToImport(uint32 tag) {
	return tag == kTagMainModel || tag == kTagModel || tag == kTagKeyframe || tag == kTagMesh || tag == kTagMaterial || tag == kTagSprite;
}

// The chores of the legacy engine are numbered by the id of the .cos file, the new ones by the position in the file
static int findChoreIndex(const LegacyCostumeLayout &layout, int32 oldId) {
	for (uint i = 0; i < layout.chores.size(); i++) {
		if (layout.chores[i].id == oldId)
			return (int)i;
	}

	return -1;
}

static bool getTrackStatus(const LegacyAnimState &anim, uint *statusPointer) {
	uint status = kTrackInUse | kTrackPlaying;

	if (anim.repeatMode == kLegacyRepeatOnce)
		status |= kTrackRemoveAtEnd;
	else if (anim.repeatMode == kLegacyRepeatPauseAtEnd)
		status |= kTrackHoldAtEnd;
	else if (anim.repeatMode == kLegacyRepeatFadeAtEnd)
		status |= kTrackFadeOutAtEnd;
	else if (anim.repeatMode != kLegacyRepeatLooping)
		return false;

	if (anim.fadeMode == kLegacyFadeIn)
		status |= kTrackFadingIn;
	else if (anim.fadeMode == kLegacyFadeOut)
		status |= kTrackFadingOut;
	else if (anim.fadeMode != kLegacyFadeNone)
		return false;

	if (anim.paused)
		status |= kTrackPaused;

	*statusPointer = status;

	return true;
}

namespace {

class LegacyActorImporter {
public:
	LegacyActorImporter(LegacyImporter *importer, SputActorModule *actors, SputCostume *sputCostume, SputRoom *sputRoom,
			Res *res, SputResource *sputResource, WalkSystem *walkSystem);

	void run();

private:
	typedef Common::HashMap<Common::String, LegacyCostumeLayout> LayoutCache;

	bool importActor(const LegacyActor &actor);
	void importPlacement(const LegacyActor &actor, SputActor *actorPointer);
	bool importCostumes(const LegacyActor &actor, SputActor *actorPointer, Common::Array<CostumeNode *> &nodes, Common::Array<const LegacyCostumeLayout *> &layouts);
	void importCostumeState(const LegacyActor &actor, const LegacyCostume &costume, const LegacyCostumeLayout &layout, SputActor *actorPointer, CostumeNode *node);
	void importChores(const LegacyCostume &costume, const LegacyCostumeLayout &layout, int inst);
	void importComponents(const LegacyActor &actor, const LegacyCostume &costume, const LegacyCostumeLayout &layout, int inst);
	void importMesh(const LegacyComponent &comp, MeshInstData *mesh);
	void importMaterial(const LegacyComponent &comp, MaterialInst *material);
	void importSprite(const LegacyComponent &comp, ModelInst *sprite);
	void importKeyframe(const LegacyActor &actor, const LegacyCostume &costume, const LegacyComponent &comp, KeyframeInst *keyframe);
	void importHead(const LegacyCostume &costume, SputActor *actor, CostumeNode *node);
	void importColormap(const LegacyActor &actor, SputActor *actorPointer);
	void importActionChores(const LegacyActor &actor, const Common::Array<CostumeNode *> &nodes, const Common::Array<const LegacyCostumeLayout *> &layouts);
	void importWalk(const LegacyActor &actor, SputActor *actorPointer);
	void importShadows(const LegacyActor &actor, SputActor *actorPointer);
	void importShadow(const LegacyActor &actor, int slot, const LegacyActorShadow &shadow, SputActor *actorPointer);

	const LegacyCostumeLayout *getLayout(const Common::String &fileName);
	void mapComponents(int inst, const LegacyCostumeLayout &layout, Common::Array<int> &newIndices);
	bool isCurrentSet(const Common::String &setName);
	// The chore index of a node and the node itself, -1 if the action chore points to nothing
	int resolveActionChore(const LegacyActor &actor, const LegacyActionChore &chore, const Common::Array<CostumeNode *> &nodes,
			const Common::Array<const LegacyCostumeLayout *> &layouts, CostumeNode **node);

	LegacyImporter *_importer;
	SputActorModule *_actors;
	SputCostume *_sputCostume;
	SputRoom *_sputRoom;
	Res *_res;
	SputResource *_sputResource;
	WalkSystem *_walkSystem;
	Cos *_cos;
	LayoutCache _layouts;
};

LegacyActorImporter::LegacyActorImporter(LegacyImporter *importer, SputActorModule *actors, SputCostume *sputCostume, SputRoom *sputRoom,
		Res *res, SputResource *sputResource, WalkSystem *walkSystem) :
		_importer(importer), _actors(actors), _sputCostume(sputCostume), _sputRoom(sputRoom), _res(res),
		_sputResource(sputResource), _walkSystem(walkSystem), _cos(sputCostume->getCos()) {
}

void LegacyActorImporter::run() {
	const Common::Array<LegacyActor> &actors = _importer->getData().actors;
	Common::HashMap<int32, uint> positions;
	Common::Array<int32> ids;

	for (uint i = 0; i < actors.size(); i++) {
		if (positions.contains(actors[i].id)) {
			warning("LegacyImporter::importActors(): the actor %d is twice in the savegame, the second one is skipped", actors[i].id);
			continue;
		}

		positions.setVal(actors[i].id, i);
		ids.push_back(actors[i].id);
	}

	// The actors are made in the order of their ids, like the scripts made them
	Common::sort(ids.begin(), ids.end());

	uint numImported = 0;

	for (uint i = 0; i < ids.size(); i++) {
		if (importActor(actors[positions.getVal(ids[i])]))
			numImported++;
	}

	if (numImported != ids.size())
		warning("LegacyImporter::importActors(): %u of the %u actors were imported", numImported, ids.size());
}

bool LegacyActorImporter::importActor(const LegacyActor &actor) {
	int index = _actors->allocateActor();
	SputActor *actorPointer = _actors->actorDeref(index);

	if (!actorPointer)
		return false;

	importPlacement(actor, actorPointer);

	Common::Array<CostumeNode *> nodes;
	Common::Array<const LegacyCostumeLayout *> layouts;

	if (!importCostumes(actor, actorPointer, nodes, layouts)) {
		warning("LegacyImporter::importActors(): the actor %d (%s) is skipped", actor.id, actor.name.c_str());

		while (actorPointer->costumeStack)
			_actors->popCostume(actorPointer, 1);

		_actors->freeActor(actorPointer);

		return false;
	}

	importColormap(actor, actorPointer);
	importActionChores(actor, nodes, layouts);
	importWalk(actor, actorPointer);
	importShadows(actor, actorPointer);

	_importer->getContext().actorIndices.setVal(actor.id, index);

	return true;
}

void LegacyActorImporter::importPlacement(const LegacyActor &actor, SputActor *actorPointer) {
	actorPointer->actorPos[0] = actor.pos.x;
	actorPointer->actorPos[1] = actor.pos.y;
	actorPointer->actorPos[2] = actor.pos.z;
	actorPointer->prevPosition[0] = actor.pos.x;
	actorPointer->prevPosition[1] = actor.pos.y;
	actorPointer->prevPosition[2] = actor.pos.z;
	actorPointer->actorRot[0] = actor.pitch;
	actorPointer->actorRot[1] = actor.yaw;
	actorPointer->actorRot[2] = actor.roll;
	actorPointer->scale = actor.scale;
	actorPointer->matrixValid = 0;

	if (!actor.visible)
		actorPointer->flags |= kActorHidden;

	if (actor.puckOrient)
		actorPointer->flags |= kActorPuckOrient;

	if (!actor.setName.empty()) {
		if (actor.setName.size() >= kMaxSetNameSize)
			warning("LegacyImporter::importActors(): the set name %s of the actor %d is too long, the actor is in no set", actor.setName.c_str(), actor.id);
		else
			_sputRoom->addActor(actor.setName.c_str(), actorPointer);
	}

	_actors->allocateActorExtra(actorPointer);

	Extra *extra = actorPointer->extra;

	extra->walkRate = actor.walkRate;
	extra->turnRate = actor.turnRate;
	extra->reflectionAngle = actor.reflectionAngle;
	extra->timeScale = actor.timeScale;
	extra->talkColor = packColor(actor.talkColor);
	extra->collisionScale = actor.collisionScale;

	if (actor.collisionMode > (uint32)kLegacyMaxCollisionMode) {
		warning("LegacyImporter::importActors(): the actor %d has the collision mode %u, it is switched off", actor.id, actor.collisionMode);
		extra->collisionMode = 0;
	} else {
		extra->collisionMode = (int)actor.collisionMode;
	}

	if (actor.lookingMode) {
		extra->lookCarrot[0] = actor.lookAtVector.x;
		extra->lookCarrot[1] = actor.lookAtVector.y;
		extra->lookCarrot[2] = actor.lookAtVector.z;
		actorPointer->mode |= kActorLooking;
	}
}

const LegacyCostumeLayout *LegacyActorImporter::getLayout(const Common::String &fileName) {
	Common::String key = fileName;

	key.toLowercase();

	LayoutCache::iterator it = _layouts.find(key);

	if (it != _layouts.end())
		return &it->_value;

	Common::SeekableReadStream *stream = _importer->openCostumeFile(fileName);

	if (!stream) {
		warning("LegacyImporter::importActors(): cannot open the costume file %s", fileName.c_str());
		return nullptr;
	}

	LegacyCostumeLayout layout;
	bool parsed = parseLegacyCostumeFile(stream, fileName, layout);

	delete stream;
	stream = nullptr;

	if (!parsed)
		return nullptr;

	_layouts.setVal(key, layout);

	return &_layouts.find(key)->_value;
}

bool LegacyActorImporter::importCostumes(const LegacyActor &actor, SputActor *actorPointer, Common::Array<CostumeNode *> &nodes,
		Common::Array<const LegacyCostumeLayout *> &layouts) {
	// The cache must not grow while the layouts are in use
	for (uint i = 0; i < actor.costumes.size(); i++) {
		if (!getLayout(actor.costumes[i].fileName))
			return false;
	}

	for (uint i = 0; i < actor.costumes.size(); i++) {
		const LegacyCostume &costume = actor.costumes[i];
		const LegacyCostumeLayout *layout = getLayout(costume.fileName);
		int pushed;

		if (i == 0)
			pushed = _actors->setCostume(actorPointer, costume.fileName.c_str(), 0);
		else
			pushed = _actors->pushCostume(actorPointer, costume.fileName.c_str(), 0);

		CostumeNode *node = actorPointer->costumeStack;

		if (!pushed || !node || !node->costumeInstance) {
			warning("LegacyImporter::importActors(): the actor %d cannot load the costume %s", actor.id, costume.fileName.c_str());
			return false;
		}

		nodes.push_back(node);
		layouts.push_back(layout);

		if (node->rCostume.valid) {
			Common::String key = costume.fileName;

			key.toLowercase();
			_importer->getContext().costumeSerials.setVal(key, node->rCostume.serial);
		}

		importCostumeState(actor, costume, *layout, actorPointer, node);
	}

	return true;
}

void LegacyActorImporter::importCostumeState(const LegacyActor &actor, const LegacyCostume &costume, const LegacyCostumeLayout &layout, SputActor *actorPointer, CostumeNode *node) {
	int inst = node->costumeInstance;

	if (_cos->instCountChores(inst) != (int)layout.chores.size()) {
		warning("LegacyImporter::importActors(): the costume %s of the actor %d has %d chores in the game data and %u in the savegame, its state is skipped",
			costume.fileName.c_str(), actor.id, _cos->instCountChores(inst), layout.chores.size());
		return;
	}

	importChores(costume, layout, inst);
	importComponents(actor, costume, layout, inst);
	importHead(costume, actorPointer, node);
}

void LegacyActorImporter::importChores(const LegacyCostume &costume, const LegacyCostumeLayout &layout, int inst) {
	for (uint i = 0; i < layout.chores.size(); i++) {
		int32 oldId = layout.chores[i].id;

		if (oldId < 0 || (uint)oldId >= costume.chores.size())
			continue;

		const LegacyChore &chore = costume.chores[oldId];
		uint32 state = 0;
		int time = 0;

		if (chore.playing) {
			// The update of the costume only advances the chores that have the third bit
			state = kCosChoreStatePlaying | kCosChoreStateNoAdvance;

			if (chore.looping)
				state |= kCosChoreStateLooping;

			if (chore.paused)
				state &= ~(uint32)kCosChoreStatePlaying;

			if (chore.currTime > 0)
				time = chore.currTime;
		}

		LegacyImporter::setChoreInstance(_cos, inst, (int)i, state, time);
	}
}

void LegacyActorImporter::mapComponents(int inst, const LegacyCostumeLayout &layout, Common::Array<int> &newIndices) {
	uint numLegacy = layout.components.size();
	Common::Array<uint> depths;

	for (uint i = 0; i < numLegacy; i++) {
		uint depth = 0;
		int32 parent = layout.components[i].parent;

		while (parent >= 0 && depth <= numLegacy) {
			depth++;
			parent = layout.components[parent].parent;
		}

		depths.push_back(depth);
		newIndices.push_back(-1);
	}

	int numNew = _cos->instCountComponents(inst);

	// The new loader adds the components by depth in the tree and numbers the ones with the same name in that order
	for (uint i = 0; i < numLegacy; i++) {
		uint32 tag = layout.componentTags[i];

		if (!hasStateToImport(tag))
			continue;

		const Common::String &name = layout.components[i].name;
		int rank = 0;

		for (uint k = 0; k < numLegacy; k++) {
			if (k != i && layout.componentTags[k] == tag && layout.components[k].name == name && (depths[k] < depths[i] || (depths[k] == depths[i] && k < i)))
				rank++;
		}

		for (int j = 0; j < numNew; j++) {
			int fourcc = 0;
			CosComponent *comp = _cos->instComponentInfo(inst, j, &fourcc, nullptr, nullptr);

			if (comp && comp->depthIdx == rank && tagsMatch(fourcc, tag) && namesMatch(comp->name, name)) {
				newIndices[i] = j;
				break;
			}
		}
	}
}

void LegacyActorImporter::importComponents(const LegacyActor &actor, const LegacyCostume &costume, const LegacyCostumeLayout &layout, int inst) {
	Common::Array<int> newIndices;

	mapComponents(inst, layout, newIndices);

	for (uint i = 0; i < costume.components.size(); i++) {
		const LegacyComponent &comp = costume.components[i];

		if (!hasStateToImport(comp.tag))
			continue;

		if (comp.index < 0 || (uint)comp.index >= newIndices.size() || newIndices[comp.index] < 0) {
			warning("LegacyImporter::importActors(): the component %d of the costume %s of the actor %d is not in the game data", comp.index, costume.fileName.c_str(), actor.id);
			continue;
		}

		void *data = _cos->instGetInstanceData(inst, newIndices[comp.index], nullptr);

		if (!data) {
			warning("LegacyImporter::importActors(): the component %d of the costume %s of the actor %d has no instance", comp.index, costume.fileName.c_str(), actor.id);
			continue;
		}

		if (comp.tag == kTagMainModel || comp.tag == kTagModel)
			static_cast<ModelInst *>(data)->visible = comp.visible ? 1 : 0;
		else if (comp.tag == kTagMesh)
			importMesh(comp, static_cast<MeshInstData *>(data));
		else if (comp.tag == kTagMaterial)
			importMaterial(comp, static_cast<MaterialInst *>(data));
		else if (comp.tag == kTagSprite)
			importSprite(comp, static_cast<ModelInst *>(data));
		else if (comp.tag == kTagKeyframe)
			importKeyframe(actor, costume, comp, static_cast<KeyframeInst *>(data));
	}
}

void LegacyActorImporter::importMesh(const LegacyComponent &comp, MeshInstData *mesh) {
	RdThing *thing = mesh->modelInst ? mesh->modelInst->thing : nullptr;

	if (!thing || thing->type != 1 || !thing->abMeshVisible || mesh->meshIndex < 0 || (uint)mesh->meshIndex >= thing->data.model3->numHNodes) {
		warning("LegacyImporter::importActors(): the mesh component %d has no mesh", comp.index);
		return;
	}

	mesh->visible = comp.meshVisible ? 1 : 0;
	thing->abMeshVisible[mesh->meshIndex] = mesh->visible;
}

void LegacyActorImporter::importMaterial(const LegacyComponent &comp, MaterialInst *material) {
	RdThing *thing = material->modelInst ? material->modelInst->thing : nullptr;

	if (!thing || thing->type != 1) {
		warning("LegacyImporter::importActors(): the material component %d has no model", comp.index);
		return;
	}

	RdModel3 *model = thing->data.model3;
	uint materialIndex = (uint)(intptr)material->compRes;

	if (materialIndex >= model->numMaterials || !model->materials[materialIndex]) {
		warning("LegacyImporter::importActors(): the material component %d has no material", comp.index);
		return;
	}

	if (comp.activeTexture < 0 || (uint)comp.activeTexture >= model->materials[materialIndex]->numCels) {
		warning("LegacyImporter::importActors(): the material component %d has the texture %d, the material has %u", comp.index, comp.activeTexture, model->materials[materialIndex]->numCels);
		return;
	}

	material->key = comp.activeTexture;
}

void LegacyActorImporter::importSprite(const LegacyComponent &comp, ModelInst *sprite) {
	if (!sprite->sprite || !sprite->sprite->face.material) {
		warning("LegacyImporter::importActors(): the sprite component %d has no sprite", comp.index);
		return;
	}

	RdMaterial *material = sprite->sprite->face.material;

	sprite->visible = comp.spriteVisible ? 1 : 0;

	if (comp.activeTexture < 0 || (uint)comp.activeTexture >= material->numCels)
		warning("LegacyImporter::importActors(): the sprite component %d has the texture %d, the sprite has %u", comp.index, comp.activeTexture, material->numCels);
	else
		material->curCel = comp.activeTexture;
}

void LegacyActorImporter::importKeyframe(const LegacyActor &actor, const LegacyCostume &costume, const LegacyComponent &comp, KeyframeInst *keyframe) {
	const LegacyAnimState &anim = comp.anim;

	// Only the animations that are in the list of the model have a track
	if (!anim.active)
		return;

	if (!keyframe->modelInst || !keyframe->ref.valid) {
		warning("LegacyImporter::importActors(): the keyframe component %d of the costume %s of the actor %d has no keyframe", comp.index, costume.fileName.c_str(), actor.id);
		return;
	}

	uint status;

	if (!getTrackStatus(anim, &status)) {
		warning("LegacyImporter::importActors(): the keyframe component %d of the costume %s of the actor %d has the repeat mode %d and the fade mode %d",
			comp.index, costume.fileName.c_str(), actor.id, anim.repeatMode, anim.fadeMode);
		return;
	}

	RdKeyframe *data = static_cast<RdKeyframe *>(keyframe->ref.handle);
	float frame = 0.0f;

	if (anim.time > 0 && data->fps > 0.0f) {
		frame = (float)((double)anim.time * data->fps / 1000.0);

		if (frame > (float)data->numFrames)
			frame = (float)data->numFrames;
	}

	// The legacy animation sets the fade to 1 at every update when it does not fade
	float fade = 1.0f;
	float fadeSpeed = 0.0f;

	if (anim.fadeMode != kLegacyFadeNone) {
		fade = CLIP(anim.fade, 0.0f, 1.0f);

		if (anim.fadeLength > 0) {
			fadeSpeed = 1000.0f / (float)anim.fadeLength;
		} else {
			warning("LegacyImporter::importActors(): the keyframe component %d of the costume %s of the actor %d has the fade length %d", comp.index, costume.fileName.c_str(), actor.id, anim.fadeLength);
			fadeSpeed = 1.0f;
		}
	}

	if (LegacyImporter::addPuppetTrack(_actors, keyframe->modelInst, data, keyframe->priority, keyframe->priorityMasked, status, frame, fade, fadeSpeed) < 0)
		warning("LegacyImporter::importActors(): the animation %s of the costume %s of the actor %d is lost", data->name, costume.fileName.c_str(), actor.id);
}

void LegacyActorImporter::importHead(const LegacyCostume &costume, SputActor *actor, CostumeNode *node) {
	const LegacyHead &head = costume.head;

	node->headTurnRate = costume.lookAtRate;

	// A costume without head has -1 in the joints
	if (head.joint1Node < 0 || head.joint2Node < 0 || head.joint3Node < 0)
		return;

	if (!node->headThing || node->headThing->type != 1) {
		warning("LegacyImporter::importActors(): the costume %s has a head and no model", costume.fileName.c_str());
		return;
	}

	uint numHNodes = node->headThing->data.model3->numHNodes;

	if ((uint)head.joint1Node >= numHNodes || (uint)head.joint2Node >= numHNodes || (uint)head.joint3Node >= numHNodes) {
		warning("LegacyImporter::importActors(): the head joints of the costume %s are out of the model", costume.fileName.c_str());
		return;
	}

	// The same call as the script opcode, the node is the top of the stack
	_actors->setActorHead(actor, (uint)head.joint1Node, (uint)head.joint2Node, (uint)head.joint3Node, head.maxRoll, head.maxPitch, head.maxYaw);
}

void LegacyActorImporter::importColormap(const LegacyActor &actor, SputActor *actorPointer) {
	int found = -1;

	for (uint i = 0; i < actor.costumes.size(); i++) {
		if (actor.costumes[i].hasColormap && !actor.costumes[i].colormapName.empty())
			found = (int)i;
	}

	if (found < 0)
		return;

	// The legacy engine has the colormap in the costume that was on the top when the script set it, the new one in the actor
	const Common::String &name = actor.costumes[found].colormapName;

	for (uint i = 0; i < actor.costumes.size(); i++) {
		if (actor.costumes[i].hasColormap && !actor.costumes[i].colormapName.equalsIgnoreCase(name))
			warning("LegacyImporter::importActors(): the actor %d has more than one colormap, %s is used", actor.id, name.c_str());
	}

	void *handle = _sputResource->alloc(kSputTagCmap, name.c_str());

	if (!handle) {
		warning("LegacyImporter::importActors(): the colormap %s of the actor %d cannot be loaded", name.c_str(), actor.id);
		return;
	}

	_res->setReference(actorPointer->colorMap, handle);
}

int LegacyActorImporter::resolveActionChore(const LegacyActor &actor, const LegacyActionChore &chore, const Common::Array<CostumeNode *> &nodes,
		const Common::Array<const LegacyCostumeLayout *> &layouts, CostumeNode **node) {
	*node = nullptr;

	if (!chore.hasCostume || chore.chore < 0)
		return -1;

	for (uint i = 0; i < actor.costumes.size(); i++) {
		if (!actor.costumes[i].fileName.equalsIgnoreCase(chore.costumeFileName))
			continue;

		int index = findChoreIndex(*layouts[i], chore.chore);

		if (index < 0) {
			warning("LegacyImporter::importActors(): the actor %d has the chore %d of the costume %s, which does not exist", actor.id, chore.chore, chore.costumeFileName.c_str());
			return -1;
		}

		*node = nodes[i];

		return index;
	}

	warning("LegacyImporter::importActors(): the chore of the actor %d is for the costume %s, which is not in its stack", actor.id, chore.costumeFileName.c_str());

	return -1;
}

void LegacyActorImporter::importActionChores(const LegacyActor &actor, const Common::Array<CostumeNode *> &nodes, const Common::Array<const LegacyCostumeLayout *> &layouts) {
	CostumeNode *node;
	int index;

	index = resolveActionChore(actor, actor.restChore, nodes, layouts, &node);

	if (node)
		node->restChore = index;

	index = resolveActionChore(actor, actor.walkChore, nodes, layouts, &node);

	if (node)
		node->walkChore = index;

	index = resolveActionChore(actor, actor.leftTurnChore, nodes, layouts, &node);

	if (node)
		node->turnLeftChore = index;

	index = resolveActionChore(actor, actor.rightTurnChore, nodes, layouts, &node);

	if (node)
		node->turnRightChore = index;

	index = resolveActionChore(actor, actor.mumbleChore, nodes, layouts, &node);

	if (node)
		node->mumbleChore = index;

	for (int i = 0; i < kNumTalkChores; i++) {
		index = resolveActionChore(actor, actor.talkChores[i], nodes, layouts, &node);

		if (node)
			node->chore[i] = index;
	}
}

bool LegacyActorImporter::isCurrentSet(const Common::String &setName) {
	return setName.equalsIgnoreCase(_sputRoom->getCurrentRoomName());
}

void LegacyActorImporter::importWalk(const LegacyActor &actor, SputActor *actorPointer) {
	Extra *extra = actorPointer->extra;
	bool walks = actor.walking && !actor.path.empty();

	// The path of the legacy actor is the list of the points still to go, the last one first. The new one starts with the start point
	if (walks && actor.path.size() >= (uint)kMaxPathPoints) {
		warning("LegacyImporter::importActors(): the actor %d has a path of %u points, it stops", actor.id, actor.path.size());
		walks = false;
	}

	if (!actor.followBoxes && !walks)
		return;

	if (actor.followBoxes)
		extra->inBoxes = 1;

	_actors->enablePuck(actorPointer, 1);

	// The walk planes are the ones of the current room, an actor in another room is checked when it comes in
	if (actor.followBoxes && isCurrentSet(actor.setName))
		extra->puck->validate();

	if (!walks)
		return;

	uint numPoints = actor.path.size();

	extra->numPathPoints = (int)numPoints + 1;
	extra->path[0][0] = actorPointer->actorPos[0];
	extra->path[0][1] = actorPointer->actorPos[1];
	extra->path[0][2] = actorPointer->actorPos[2];

	for (uint i = 0; i < numPoints; i++) {
		const LegacyVector3 &point = actor.path[numPoints - 1 - i];

		extra->path[i + 1][0] = point.x;
		extra->path[i + 1][1] = point.y;
		extra->path[i + 1][2] = point.z;
	}

	extra->pathIndex = 1;
	extra->walkTarget[0] = extra->path[1][0];
	extra->walkTarget[1] = extra->path[1][1];
	extra->walkTarget[2] = extra->path[1][2];
	extra->goal[0] = actor.destPos.x;
	extra->goal[1] = actor.destPos.y;
	extra->goal[2] = actor.destPos.z;
	actorPointer->mode |= kActorWalking;
}

void LegacyActorImporter::importShadows(const LegacyActor &actor, SputActor *actorPointer) {
	bool hasShadow = false;

	for (uint i = 0; i < actor.shadows.size(); i++) {
		if (!actor.shadows[i].name.empty() && !actor.shadows[i].planes.empty())
			hasShadow = true;
	}

	if (!hasShadow)
		return;

	// The planes are found by name in the walk planes of the current room only
	if (!isCurrentSet(actor.setName)) {
		warning("LegacyImporter::importActors(): the shadows of the actor %d are lost, it is not in the current room", actor.id);
		return;
	}

	for (uint i = 0; i < actor.shadows.size(); i++) {
		const LegacyActorShadow &shadow = actor.shadows[i];

		// A shadow without planes is never drawn
		if (shadow.name.empty() || shadow.planes.empty())
			continue;

		if (i >= (uint)kNumShadows) {
			warning("LegacyImporter::importActors(): the shadow %u of the actor %d is lost, there are %d", i, actor.id, kNumShadows);
			continue;
		}

		importShadow(actor, (int)i, shadow, actorPointer);
	}

	if (actor.activeShadowSlot >= 0 && actor.activeShadowSlot < kNumShadows)
		actorPointer->extra->activeShadow = actor.activeShadowSlot;
	else
		actorPointer->extra->activeShadow = 0;
}

void LegacyActorImporter::importShadow(const LegacyActor &actor, int slot, const LegacyActorShadow &shadow, SputActor *actorPointer) {
	if (!_walkSystem->planeNamed(shadow.name.c_str())) {
		warning("LegacyImporter::importActors(): the shadow %d of the actor %d is lost, the plane %s is not in the current room", slot, actor.id, shadow.name.c_str());
		return;
	}

	// The same calls as the script opcodes, in the order of the scripts
	actorPointer->extra->activeShadow = slot;
	_actors->setShadowPoint(actorPointer, shadow.pos.x, shadow.pos.y, shadow.pos.z);
	_actors->setShadowPlane(actorPointer, shadow.name.c_str());

	for (uint i = 0; i < shadow.planes.size(); i++) {
		const LegacyShadowPlane &plane = shadow.planes[i];

		if (!isCurrentSet(plane.setName) || !_walkSystem->planeNamed(plane.sectorName.c_str())) {
			warning("LegacyImporter::importActors(): the plane %s of the shadow %d of the actor %d is lost", plane.sectorName.c_str(), slot, actor.id);
			continue;
		}

		_actors->addShadowClipPlane(actorPointer, slot, plane.sectorName.c_str());
	}

	if (shadow.dontNegate)
		_actors->setShadowValid(actorPointer, slot, -1);

	_actors->activateActorShadow(actorPointer, slot, shadow.active ? 1 : 0);
}

} // End of anonymous namespace

bool LegacyImporter::importActors() {
	LegacyActorImporter importer(this, _grim->_sputActor, _grim->_sputCostume, _grim->_sputRoom, _grim->_res, _grim->_sputResource, _grim->_walkSystem);

	importer.run();

	return true;
}

} // End of namespace Grim
