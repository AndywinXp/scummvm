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

#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/grim.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_conffile.h"
#include "engines/grim/walkplane.h"

namespace Grim {

int SputRoom::readSections() {
	readSectionFunc("colormaps", &SputRoom::readColorMapsSection);
	readSectionFunc("setups", &SputRoom::readSetupsSection);
	readSectionFunc("lights", &SputRoom::readLightSection);
	readSectionFunc("sectors", &SputRoom::readSectors);

	if (_pGrim->getGameFlags() & ADGF_DEMO)
		readSectionFunc("objectstates", &SputRoom::readObjectsSection);

	_bStarted = 1;

	return 0;
}

int SputRoom::readSectionFunc(const char *name, SectionFunc pfnRead) {
	int index = sectionNameCompare(name);

	if (index == -1) {
		if (_numSections >= 32)
			return 0;

		index = _numSections++;
	}

	Common::strlcpy(_aSections[index].name, name, 32);
	_aSections[index].handler = pfnRead;

	return 1;
}

int SputRoom::sectionNameCompare(const char *name) {
	if (_numSections <= 0)
		return -1;

	int index = 0;

	while (scumm_stricmp(_aSections[index].name, name)) {
		if (++index >= _numSections)
			return -1;
	}

	return index;
}

int SputRoom::loadSections(const char *fileName) {
	char name[1024];

	if (!_pStdConffile->open(fileName))
		return 0;

	if (_pStdConffile->readLine()) {
		while (sscanf(_pStdConffile->getLine(), "section: %s", name) != 1 || findSectionHandler(name)) {
			if (!_pStdConffile->readLine()) {
				_pStdConffile->close();
				return 1;
			}
		}

		_pStdConffile->close();
		return 0;
	}

	_pStdConffile->close();

	return 1;
}

int SputRoom::findSectionHandler(const char *name) {
	int index = sectionNameCompare(name);

	return index == -1 || (this->*_aSections[index].handler)() != 0;
}

int SputRoom::readLightSection() {
	Room *pRoom = _pLoadingRoom;
	RoomLight *pLight = nullptr;
	int lightIndex = -1;
	bool bNumLights = false;
	char name[1024];

	pRoom->numLights = 0;

	if (!_pStdConffile->readLine())
		return 1;

	while (true) {
		const char *line = _pStdConffile->getLine();

		if (*line) {
			int numLights;

			name[0] = 0;

			if (sscanf(line, " section: %s\n", name)) {
				if (!findSectionHandler(name))
					return 0;
			} else if (sscanf(line, " numlights %d\n", &numLights) == 1) {
				if (numLights < 0 || numLights > 64) {
					warning("SputRoom::readLightSection(): invalid number of lights %d", numLights);
					numLights = CLIP(numLights, 0, 64);
				}

				pRoom->numLights = numLights;
				bNumLights = true;
				pLight = nullptr;
				lightIndex = -1;
				memset(pRoom->aLights, 0, sizeof(pRoom->aLights));
			} else if (bNumLights) {
				if (sscanf(line, " light %s", name) == 1) {
					lightIndex++;

					if (lightIndex >= 64) {
						warning("SputRoom::readLightSection(): more than 64 lights");
						pLight = nullptr;
					} else {
						pLight = &pRoom->aLights[lightIndex];
						_pRd->_light.newEntry(&pLight->light);

						if (strlen(name) >= 32)
							warning("SputRoom::readLightSection(): light name %s is too long", name);

						Common::strlcpy(pLight->aName, name, 32);
					}
				}

				if (!pLight) {
					warning("SputRoom::readLightSection(): line outside a light ignored");
				} else {
					if (sscanf(line, " type %s", name) == 1) {
						if (!scumm_stricmp(name, "omni"))
							pLight->light.type = 2;
						else if (!strcmp(name, "spot"))
							pLight->light.type = 3;
						else
							pLight->light.type = 0;
					}

					if (sscanf(line, " direction %f %f %f", &pLight->light.direction[0], &pLight->light.direction[1], &pLight->light.direction[2]) == 3)
						_pRd->normalize3Acc(pLight->light.direction);

					sscanf(line, " position %f %f %f", &pLight->position[0], &pLight->position[1], &pLight->position[2]);
					sscanf(line, " intensity %f", &pLight->light.intensity);
					sscanf(line, " umbraangle %f", &pLight->light.angleX);
					sscanf(line, " penumbraangle %f", &pLight->light.angleY);

					int red;
					int green;
					int blue;

					if (sscanf(line, " color %d %d %d", &red, &green, &blue) == 3)
						pLight->light.color = (uint)(blue | ((green | (red << 8)) << 8));
				}
			}
		}

		if (!_pStdConffile->readLine())
			return 1;
	}
}

int SputRoom::readSetupsSection() {
	Room *pRoom = _pLoadingRoom;
	bool bDemo = (_pGrim->getGameFlags() & ADGF_DEMO) != 0;
	int setupIndex = -1;
	char name[1024];
	char fileName[1024];

	pRoom->pSetups = nullptr;

	if (!_pStdConffile->readLine())
		return 1;

	while (true) {
		const char *line = _pStdConffile->getLine();

		if (*line) {
			int numSetups;

			name[0] = 0;

			if (sscanf(line, " section: %s\n", name)) {
				if (!findSectionHandler(name))
					return 0;
			} else if (sscanf(line, " numsetups %d\n", &numSetups) == 1) {
				if (numSetups < 0) {
					warning("SputRoom::readSetupsSection(): invalid number of setups %d", numSetups);
					return 0;
				}

				pRoom->numSetups = numSetups;
				pRoom->pSetups = new RoomSetup[numSetups];
				memset(pRoom->pSetups, 0, sizeof(RoomSetup) * numSetups);
				setupIndex = -1;

				if (bDemo && pRoom->numObjects) {
					for (int i = 0; i < numSetups; i++) {
						pRoom->pSetups[i].pObjectArt = new RoomObjectArt[pRoom->numObjects];
						memset(pRoom->pSetups[i].pObjectArt, 0, sizeof(RoomObjectArt) * pRoom->numObjects);
					}
				}
			} else {
				if (!pRoom->pSetups)
					return 0;

				if (sscanf(line, " setup %s", name) == 1) {
					setupIndex++;

					if (setupIndex >= (int)pRoom->numSetups)
						return 0;

					if (strlen(name) >= 32)
						warning("SputRoom::readSetupsSection(): setup name %s is too long", name);

					Common::strlcpy(pRoom->pSetups[setupIndex].aName, name, 32);
				} else if (setupIndex < 0) {
					return 0;
				} else {
					RoomSetup *pSetup = &pRoom->pSetups[setupIndex];

					if (bDemo && sscanf(line, " object_art %s %s", name, fileName) == 2) {
						for (uint i = 0; i < pRoom->numObjects; i++) {
							if (strcmp(name, pRoom->pObjects[i].name))
								continue;

							if (strlen(fileName) >= 32)
								warning("SputRoom::readSetupsSection(): object art name %s is too long", fileName);

							Common::strlcpy(pSetup->pObjectArt[i].name, name, 32);
							Common::strlcpy(pSetup->pObjectArt[i].aBitmapName, fileName, 32);
						}
					} else if (bDemo && sscanf(line, " object_z %s %s", name, fileName) == 2) {
						for (uint i = 0; i < pRoom->numObjects; i++) {
							if (strcmp(name, pRoom->pObjects[i].name))
								continue;

							if (strlen(fileName) >= 32)
								warning("SputRoom::readSetupsSection(): object z buffer name %s is too long", fileName);

							Common::strlcpy(pSetup->pObjectArt[i].name, name, 32);
							Common::strlcpy(pSetup->pObjectArt[i].aZBufferName, fileName, 32);
						}
					} else if (sscanf(line, " background %s", name) == 1) {
						if (strlen(name) >= 32)
							warning("SputRoom::readSetupsSection(): background name %s is too long", name);

						Common::strlcpy(pSetup->aBitmapName, name, 32);
					} else if (sscanf(line, " zbuffer %s", name) == 1) {
						if (strlen(name) >= 32)
							warning("SputRoom::readSetupsSection(): zbuffer name %s is too long", name);

						Common::strlcpy(pSetup->aZBufferName, name, 32);
					} else if (sscanf(line, " position %f %f %f", &pSetup->camera.position[0], &pSetup->camera.position[1], &pSetup->camera.position[2]) != 3 &&
							sscanf(line, " interest %f %f %f", &pSetup->camera.interest[0], &pSetup->camera.interest[1], &pSetup->camera.interest[2]) != 3 &&
							sscanf(line, " roll %f", &pSetup->camera.roll) != 1 &&
							sscanf(line, " fov %f", &pSetup->camera.fov) != 1 &&
							sscanf(line, " nclip %f", &pSetup->camera.nearClip) != 1 &&
							sscanf(line, " fclip %f", &pSetup->camera.farClip) != 1) {
						return 0;
					}
				}
			}
		}

		if (!_pStdConffile->readLine())
			return 1;
	}
}

int SputRoom::readObjectsSection() {
	Room *pRoom = _pLoadingRoom;
	RoomObject *pObject = nullptr;
	char name[1024];

	pRoom->numObjects = 0;
	pRoom->pObjects = nullptr;

	if (!_pStdConffile->readLine())
		return 1;

	while (true) {
		const char *line = _pStdConffile->getLine();

		if (*line) {
			int numObjects;

			name[0] = 0;

			if (sscanf(line, " section: %s\n", name)) {
				if (!findSectionHandler(name))
					return 0;
			} else {
				if (sscanf(line, " tot_objects %d\n", &numObjects) == 1) {
					delete[] pRoom->pObjects;
					pRoom->pObjects = nullptr;
					pRoom->numObjects = 0;

					if (numObjects > 0) {
						pRoom->numObjects = numObjects;
						pRoom->pObjects = new RoomObject[numObjects];
						memset(pRoom->pObjects, 0, sizeof(RoomObject) * numObjects);
					}

					pObject = pRoom->pObjects;
				}

				if (pObject) {
					RdVector3 position;

					if (sscanf(line, " object %s %f %f %f\n", name, &position[0], &position[1], &position[2]) == 4) {
						if ((uint)(pObject - pRoom->pObjects) >= pRoom->numObjects) {
							warning("SputRoom::readObjectsSection(): more than %u objects", pRoom->numObjects);
							return 0;
						}

						if (strlen(name) >= 32)
							warning("SputRoom::readObjectsSection(): object name %s is too long", name);

						Common::strlcpy(pObject->name, name, 32);
						memcpy(pObject->position, position, sizeof(RdVector3));
						pObject++;
					}
				}
			}
		}

		if (!_pStdConffile->readLine())
			return 1;
	}
}

int SputRoom::readColorMapsSection() {
	Room *pRoom = _pLoadingRoom;
	char name[1024];

	if (pRoom->apColormaps)
		warning("SputRoom::readColorMapsSection(): pRoom->apColormaps == NULL");

	if (!_pStdConffile->readLine())
		return 1;

	while (true) {
		const char *line = _pStdConffile->getLine();

		if (*line) {
			int numColormaps;
			bool bReadName = true;

			name[0] = 0;

			if (sscanf(line, " section: %s\n", name)) {
				if (!findSectionHandler(name))
					return 0;
			} else {
				if (sscanf(line, " numcolormaps %d\n", &numColormaps) == 1) {
					if (numColormaps < 0) {
						warning("SputRoom::readColorMapsSection(): invalid number of colormaps %d", numColormaps);
						return 0;
					}

					pRoom->numcolormaps = numColormaps;

					if (numColormaps) {
						pRoom->apColormaps = new ResRef[numColormaps];
						memset(pRoom->apColormaps, 0, sizeof(ResRef) * numColormaps);
					} else {
						bReadName = false;
					}
				}

				if (bReadName && pRoom->apColormaps) {
					if (sscanf(line, " colormap %s", name) == 1) {
						if (strlen(name) >= 32)
							warning("SputRoom::readColorMapsSection(): colormap name %s is too long", name);

						Common::strlcpy(pRoom->colormapName, name, 32);
					}
				}
			}
		}

		if (!_pStdConffile->readLine())
			return 1;
	}
}

int SputRoom::readSectors() {
	const char *pNextSection = _pWalkSystem->parseCollection(_pLoadingRoom->name);

	if (pNextSection)
		findSectionHandler(pNextSection);

	return 1;
}

} // End of namespace Grim
