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

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/graphics/cmaterial.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lformatparser.h"

namespace Grim {

static const int kKeywordTexture = 1;
static const char kSpecialtyPrefix[] = "specialty";
static const int kSpecialtyPrefixSize = 9;

class CMaterialParser : public LFormatParser {
public:
	explicit CMaterialParser(EmiEngine *engine) : LFormatParser(engine) {
		_material = nullptr;
		addKeyword("TEX", kKeywordTexture, 3);
	}

	bool handleKeyword(int id, int numArgs, KeywordArg *args) override {
		if (id == kKeywordTexture)
			return parseTexture(id, numArgs, args);

		return false;
	}

	bool parseTexture(int token, int numArgs, KeywordArg *args);

	CMaterial *_material;
};

bool CMaterialParser::parseTexture(int token, int numArgs, KeywordArg *args) {
	if (!_material || numArgs < 1)
		return false;

	LABitmap bitmap(_engine);
	CTexture *texture;
	const char *name = args[0].stringValue;
	CDrawContext *drawContext = _engine->getDrawContext();

	if (!scumm_strnicmp(name, kSpecialtyPrefix, kSpecialtyPrefixSize)) {
		texture = drawContext->getCachedTexture(atoi(name + kSpecialtyPrefixSize));
	} else {
		if (bitmap.loadFile(name))
			texture = drawContext->createTexture(&bitmap);
		else
			texture = drawContext->_defaultTexture;

		if (bitmap.scanAlpha())
			_material->_flags |= 0x80;
	}

	_material->_textures.push_back(texture);

	return true;
}

CMaterial::CMaterial() {
	resetFlags();
}

CMaterial::~CMaterial() {
	CDrawContext *drawContext = _engine->getDrawContext();

	if (drawContext) {
		for (uint i = 0; i < _textures.size(); i++) {
			if (_textures[i])
				drawContext->freeTexture(_textures[i]);
		}
	}

	_textures.clear();
}

int CMaterial::getNumTextures() {
	return _textures.size();
}

CTexture *CMaterial::getTexture(int index) {
	if (index < 0 || index >= (int)_textures.size()) {
		warning("CMaterial::getTexture(): texture index %d out of range", index);

		return nullptr;
	}

	return _textures[index];
}

void CMaterial::resetFlags() {
	_flags = 7;
}

void CMaterial::setFlags(int newFlags) {
	_flags = newFlags;

	for (uint i = 0; i < _textures.size(); i++) {
		if (_textures[i]) {
			_textures[i]->_flags = newFlags;
			_textures[i]->applyFlags();
		}
	}
}

void CMaterial::addFlags(int addedFlags) {
	_flags |= addedFlags;
	setFlags(_flags);
}

uint CMaterial::resSizeOfItem() {
	uint size = sizeof(CMaterial) + _textures.size() * sizeof(CTexture *);

	if (_engine->getDrawContext()) {
		for (uint i = 0; i < _textures.size(); i++) {
			if (_textures[i])
				size += _textures[i]->getSize();
		}
	}

	return size;
}

bool CMaterial::resLoadItem(const char *itemName) {
	CDrawContext *drawContext = _engine->getDrawContext();

	if (!drawContext) {
		warning("CMaterial::resLoadItem(): no draw context");

		return false;
	}

	LString path(itemName);

	if (!path.contains(".sur") && !path.contains(".SUR")) {
		if (!scumm_strnicmp(itemName, kSpecialtyPrefix, kSpecialtyPrefixSize)) {
			_textures.push_back(drawContext->getCachedTexture(atoi(itemName + kSpecialtyPrefixSize)));
		} else {
			LABitmap bitmap(_engine);
			CTexture *texture;

			if (bitmap.loadFile(path.c_str()))
				texture = drawContext->createTexture(&bitmap);
			else
				texture = drawContext->_defaultTexture;

			if (bitmap.scanAlpha())
				_flags |= 0x80;

			_textures.push_back(texture);
		}
	} else {
		CMaterialParser parser(_engine);

		parser._material = this;

		if (!parser.parseFile(path.c_str(), 0, 0))
			parser.parseFile("default.sur", 0, 0);
	}

	setFlags(_flags);

	return true;
}

} // End of namespace Grim
