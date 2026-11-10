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

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/cmaterial.h"
#include "engines/grim/emi/lformatparser.h"

namespace Grim {

static const int kKeywordTexture = 1;
static const char kSpecialtyPrefix[] = "specialty";
static const int kSpecialtyPrefixSize = 9;

class CMaterialParser : public LFormatParser {
public:
	CMaterialParser() {
		material = nullptr;
		addKeyword("TEX", kKeywordTexture, 3);
	}

	bool handleKeyword(int id, int numArgs, KeywordArg *args) override {
		if (id == kKeywordTexture)
			return parseTexture(id, numArgs, args);

		return false;
	}

	bool parseTexture(int token, int numArgs, KeywordArg *args);

	CMaterial *material;
};

bool CMaterialParser::parseTexture(int token, int numArgs, KeywordArg *args) {
	if (!material || numArgs < 1)
		return false;

	LABitmap bitmap;
	CTexture *texture;
	const char *name = args[0].stringValue;

	if (!scumm_strnicmp(name, kSpecialtyPrefix, kSpecialtyPrefixSize)) {
		texture = CDrawContext::context->getCachedTexture(atoi(name + kSpecialtyPrefixSize));
	} else {
		if (bitmap.loadFile(name))
			texture = CDrawContext::context->createTexture(&bitmap);
		else
			texture = CDrawContext::context->defaultTexture;

		if (bitmap.scanAlpha())
			material->flags |= 0x80;
	}

	material->textures.push_back(texture);

	return true;
}

CMaterial::CMaterial() {
	resetFlags();
}

CMaterial::~CMaterial() {
	if (CDrawContext::context) {
		for (uint i = 0; i < textures.size(); i++) {
			if (textures[i])
				CDrawContext::context->freeTexture(textures[i]);
		}
	}

	textures.clear();
}

int CMaterial::getNumTextures() {
	return textures.size();
}

CTexture *CMaterial::getTexture(int index) {
	if (index < 0 || index >= (int)textures.size()) {
		warning("CMaterial::getTexture(): texture index %d out of range", index);

		return nullptr;
	}

	return textures[index];
}

void CMaterial::resetFlags() {
	flags = 7;
}

void CMaterial::setFlags(int newFlags) {
	flags = newFlags;

	for (uint i = 0; i < textures.size(); i++) {
		if (textures[i]) {
			textures[i]->flags = newFlags;
			textures[i]->applyFlags();
		}
	}
}

void CMaterial::addFlags(int addedFlags) {
	flags |= addedFlags;
	setFlags(flags);
}

uint CMaterial::resSizeOfItem() {
	uint size = sizeof(CMaterial) + textures.size() * sizeof(CTexture *);

	if (CDrawContext::context) {
		for (uint i = 0; i < textures.size(); i++) {
			if (textures[i])
				size += textures[i]->getSize();
		}
	}

	return size;
}

bool CMaterial::resLoadItem(const char *itemName) {
	if (!CDrawContext::context) {
		warning("CMaterial::resLoadItem(): no draw context");

		return false;
	}

	LString path(itemName);

	if (!path.contains(".sur") && !path.contains(".SUR")) {
		if (!scumm_strnicmp(itemName, kSpecialtyPrefix, kSpecialtyPrefixSize)) {
			textures.push_back(CDrawContext::context->getCachedTexture(atoi(itemName + kSpecialtyPrefixSize)));
		} else {
			LABitmap bitmap;
			CTexture *texture;

			if (bitmap.loadFile(path.c_str()))
				texture = CDrawContext::context->createTexture(&bitmap);
			else
				texture = CDrawContext::context->defaultTexture;

			if (bitmap.scanAlpha())
				flags |= 0x80;

			textures.push_back(texture);
		}
	} else {
		CMaterialParser parser;

		parser.material = this;

		if (!parser.parseFile(path.c_str(), 0, 0))
			parser.parseFile("default.sur", 0, 0);
	}

	setFlags(flags);

	return true;
}

} // End of namespace Grim
