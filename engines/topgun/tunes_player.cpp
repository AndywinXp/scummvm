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

#include "audio/decoders/wave.h"
#include "audio/mixer.h"
#include "backends/audiocd/audiocd.h"
#include "common/archive.h"
#include "common/compression/unzip.h"
#include "common/endian.h"
#include "common/path.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "engines/util.h"
#include "graphics/cursorman.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/wincursor.h"
#include "image/bmp.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/tunes_player.h"

#define TG_TUNES_COMBO_X              80
#define TG_TUNES_COMBO_Y              80
#define TG_TUNES_COMBO_WIDTH          280
#define TG_TUNES_COMBO_CLOSED_HEIGHT  24
#define TG_TUNES_COMBO_TOTAL_HEIGHT   200
#define TG_TUNES_SCROLLBAR_WIDTH      16
#define TG_TUNES_COMBO_BUTTON_WIDTH   17
#define TG_TUNES_NUM_DISC_TRACKS      98
#define TG_TUNES_MAX_VOLUME_LEVEL     5
#define TG_TUNES_VOLUME_STEP          0x3333
#define TG_TUNES_COMMAND_GET_POSITION 105
#define TG_TUNES_COMMAND_GET_VOLUME   106
#define TG_TUNES_COMMAND_GET_TRACKS   107
#define TG_TUNES_COMMAND_PLAY_RANGE   110
#define TG_TUNES_COMMAND_PLAY_TRACK   111
#define TG_TUNES_COMMAND_SET_VOLUME   114
#define TG_TUNES_COMMAND_STOP_CD      115
#define TG_TUNES_FIRST_GOODBYE_WAVE   20
#define TG_TUNES_NUM_GOODBYE_WAVES    9
#define TG_TUNES_MASCOT_WAVE          30
#define TG_TUNES_FIRST_TUNE_TRACK     3

namespace Topgun {

const TunesPlayerEngine::Tune TunesPlayerEngine::kTunes[TG_TUNES_NUM_TUNES] = {
	{ 40, 37200, "Baa Baa Black Sheep" },
	{ 36, 55300, "B-I-N-G-O" },
	{ 28, 38500, "Down By The Station" },
	{ 38, 45000, "Down In The Valley" },
	{ 22, 45000, "Five Little Fishes" },
	{ 45, 35000, "Good Night Baby" },
	{ 26, 39400, "Head And Shoulders" },
	{ 43, 33700, "Hey Diddle Diddle" },
	{ 11, 46400, "Hickory Dickory Dock" },
	{ 10, 41900, "Hot Cross Buns" },
	{ 41, 41300, "Hush Little Baby" },
	{ 29, 75300, "If You're Happy" },
	{ 8, 40100, "I'm A Little Teapot" },
	{ 27, 50300, "Itsy Bitsy Spider" },
	{ 33, 45000, "I've Been Working" },
	{ 35, 28900, "Jack And Jill" },
	{ 31, 26100, "Little Bo Peep" },
	{ 37, 26200, "Little Bo Peep reprise" },
	{ 21, 34300, "Little Green Frogs" },
	{ 20, 24800, "Looby Loo" },
	{ 39, 32200, "Mary Had A Little Lamb" },
	{ 46, 38000, "Merrily We Roll Along" },
	{ 16, 30400, "Oh! Susanna" },
	{ 30, 35600, "Oh Where Has My Little Dog Gone?" },
	{ 3, 66100, "Old MacDonald Had A Farm" },
	{ 25, 21400, "One, Two, Buckle My Shoe" },
	{ 23, 45100, "Over The River" },
	{ 9, 30500, "Pat-a-Cake" },
	{ 6, 26100, "Polly Put The Kettle On" },
	{ 18, 35200, "Row Your Boat" },
	{ 19, 34900, "Sailor Went To Sea" },
	{ 34, 43600, "She'll Be Comin' Round The Mountain" },
	{ 17, 44900, "Shoe Fly" },
	{ 13, 25100, "Skip To My Loo" },
	{ 7, 27300, "Sooky Take It Off Again" },
	{ 12, 43600, "SourWood Mountain" },
	{ 14, 43300, "The Old Gray Mare" },
	{ 32, 48300, "This Little Pig Went To Market" },
	{ 24, 39900, "This Old Man" },
	{ 5, 31800, "Three Blind Mice" },
	{ 44, 17300, "Tisket a Tasket" },
	{ 15, 49300, "Turkey In The Straw" },
	{ 42, 56400, "Twinkle Little Star" }
};

TunesPlayerEngine::TunesPlayerEngine(OSystem *syst, const ADGameDescription *gameDesc) :
	TopgunEngine(syst, gameDesc, 0) {
}

TunesPlayerEngine::~TunesPlayerEngine() {
	delete _surface;
}

Common::Error TunesPlayerEngine::run() {
	Graphics::PixelFormat format(4, 8, 8, 8, 8, 24, 16, 8, 0);

	initGraphics(TG_TUNES_SCREEN_WIDTH, TG_TUNES_SCREEN_HEIGHT, &format);

	if (g_system->getScreenFormat() != format)
		return Common::kUnsupportedColorMode;

	_format = format;
	_surface = new Graphics::Screen();
	_window.create(TG_TUNES_CLIENT_WIDTH, TG_TUNES_CLIENT_HEIGHT, format);
	_backdrop.create(TG_TUNES_SCREEN_WIDTH, TG_TUNES_SCREEN_HEIGHT, format);

	if (rtStartup(true))
		mainLoop();

	return Common::kNoError;
}

bool TunesPlayerEngine::rtStartup(bool runChecks) {
	if (!existExtractedCDAudioFiles(TG_TUNES_FIRST_TUNE_TRACK) && !isDataAndCDAudioReadFromSameCD())
		warnMissingExtractedCDAudio();

	g_system->getAudioCDManager()->open();

	Graphics::Cursor *cursor = Graphics::makeDefaultWinCursor();
	if (cursor) {
		CursorMan.replaceCursor(cursor);
		delete cursor;
	}

	CursorMan.showMouse(true);

	if (!_resources.loadFromEXE(Common::Path("TUNES.EXE"))) {
		GUIErrorMessage("Can't open file 'TUNES.EXE'.");
		return false;
	}

	if (!loadBitmaps())
		return false;

	createControls();

	if (audioLibrary()->audioInitLibrary(0x100) != 1)
		return false;

	audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);

	uint16 level = (uint16)((audioLibrary()->audioCommand(TG_TUNES_COMMAND_GET_VOLUME, 0, 0, nullptr) & 0xFFFF) / TG_TUNES_VOLUME_STEP);
	_buttons[kButtonVolumeIndicator].frame = (byte)level;
	_volumeLevel = level;

	Common::Rect outer((TG_TUNES_SCREEN_WIDTH - TG_TUNES_WINDOW_WIDTH) / 2, (TG_TUNES_SCREEN_HEIGHT - TG_TUNES_WINDOW_HEIGHT) / 2,
	                   (TG_TUNES_SCREEN_WIDTH + TG_TUNES_WINDOW_WIDTH) / 2, (TG_TUNES_SCREEN_HEIGHT + TG_TUNES_WINDOW_HEIGHT) / 2);

	_origin = Common::Point(outer.left + TG_TUNES_CLIENT_LEFT, outer.top + TG_TUNES_CLIENT_TOP);

	drawDesktop();
	drawFrame(outer);
	_surface->blitFrom(_backdrop);
	drawWindow();
	setButtonState(_buttons[kButtonStop], true);
	presentScreen();
	checkCdPresent();
	return true;
}

uint16 TunesPlayerEngine::mainLoop() {
	while (!shouldQuit() && !_closing) {
		pollEvents();

		if (_playing && !_closing)
			autoAdvanceTick();

		presentScreen();
	}

	if (!_closing) {
		_closing = true;
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
		audioLibrary()->audioExitLibrary();
	}

	return 0;
}

bool TunesPlayerEngine::loadBitmap(uint16 id, Graphics::ManagedSurface &surface) {
	Common::SeekableReadStream *stream = _resources.getResource(Common::kWinBitmap, id);
	if (!stream) {
		warning("TunesPlayerEngine::loadBitmap(): bitmap %d is missing", id);
		return false;
	}

	uint32 headerSize = stream->readUint32LE();
	int32 width = stream->readSint32LE();
	int32 height = stream->readSint32LE();
	stream->readUint16LE();
	uint16 bitCount = stream->readUint16LE();
	uint32 compression = stream->readUint32LE();
	stream->skip(12);
	uint32 colorsUsed = stream->readUint32LE();
	stream->readUint32LE();

	if (headerSize != 40 || compression != 0 || (bitCount != 4 && bitCount != 8) || width <= 0 || height <= 0) {
		warning("TunesPlayerEngine::loadBitmap(): bitmap %d has an unsupported format", id);
		delete stream;
		return false;
	}

	uint32 numColors = (colorsUsed != 0) ? colorsUsed : (1U << bitCount);
	uint32 palette[256];

	for (uint32 i = 0; i < numColors && i < 256; i++) {
		byte blue = stream->readByte();
		byte green = stream->readByte();
		byte red = stream->readByte();
		stream->readByte();
		palette[i] = _format.ARGBToColor(0xFF, red, green, blue);
	}

	surface.create(width, height, _format);

	uint32 rowBytes = (((uint32)width * bitCount + 31) / 32) * 4;
	byte *row = new byte[rowBytes];

	for (int32 sourceRow = 0; sourceRow < height; sourceRow++) {
		stream->read(row, rowBytes);

		uint32 *destination = (uint32 *)surface.getBasePtr(0, height - 1 - sourceRow);

		for (int32 x = 0; x < width; x++) {
			byte index = (bitCount == 8) ? row[x] : ((x & 1) ? (row[x >> 1] & 0x0F) : (row[x >> 1] >> 4));

			destination[x] = palette[index];
		}
	}

	delete[] row;
	delete stream;
	return true;
}

bool TunesPlayerEngine::loadBitmaps() {
	for (uint16 i = 0; i < TG_TUNES_NUM_BITMAPS; i++) {
		if (!loadBitmap(TG_TUNES_FIRST_BITMAP + i, _bitmaps[i]))
			return false;
	}

	return true;
}

void TunesPlayerEngine::playWaveResource(uint16 id) {
	Common::SeekableReadStream *data = _resources.getResource(Common::WinResourceID("WAVE"), Common::WinResourceID((uint32)id));
	if (!data) {
		warning("TunesPlayerEngine::playWaveResource(): wave %d is missing", id);
		return;
	}

	Audio::SeekableAudioStream *stream = Audio::makeWAVStream(data, DisposeAfterUse::YES);
	if (!stream) {
		warning("TunesPlayerEngine::playWaveResource(): wave %d cannot be decoded", id);
		return;
	}

	Audio::Mixer *mixer = g_system->getMixer();
	Audio::SoundHandle handle;

	mixer->playStream(Audio::Mixer::kSFXSoundType, &handle, stream);

	while (mixer->isSoundHandleActive(handle) && !shouldQuit()) {
		Common::Event event;

		while (g_system->getEventManager()->pollEvent(event)) {
		}

		g_system->delayMillis(10);
	}
}

uint32 TunesPlayerEngine::color(byte red, byte green, byte blue) const {
	return _format.ARGBToColor(0xFF, red, green, blue);
}

void TunesPlayerEngine::createControls() {
	struct ButtonLayout {
		int16 x;
		int16 y;
		int16 width;
		int16 height;
		byte bitmap;
		byte row;
		bool latching;
		bool interactive;
		bool displayOnly;
	};

	static const ButtonLayout layouts[TG_TUNES_NUM_BUTTONS] = {
		{ 80, 20, 42, 38, 0, 0, true, true, false },
		{ 122, 20, 42, 38, 0, 1, true, true, false },
		{ 164, 20, 42, 38, 0, 2, true, true, false },
		{ 206, 20, 42, 38, 0, 3, false, true, false },
		{ 248, 20, 42, 38, 0, 4, false, true, false },
		{ 300, 20, 20, 19, 5, 0, false, true, false },
		{ 300, 39, 20, 19, 5, 1, false, true, false },
		{ 4, 10, 60, 101, 1, 0, false, true, false },
		{ 319, 20, 40, 38, 2, 0, false, false, true },
		{ 4, 128, 100, 29, 4, 0, false, false, false },
		{ 244, 126, 116, 35, 3, 0, false, true, false },
		{ 150, 126, 69, 35, 6, 0, false, true, false }
	};

	for (int i = 0; i < TG_TUNES_NUM_BUTTONS; i++) {
		Button &button = _buttons[i];

		button.rect = Common::Rect(layouts[i].x, layouts[i].y, layouts[i].x + layouts[i].width, layouts[i].y + layouts[i].height);
		button.bitmap = layouts[i].bitmap;
		button.row = layouts[i].row;
		button.latching = layouts[i].latching;
		button.interactive = layouts[i].interactive;
		button.displayOnly = layouts[i].displayOnly;
	}

	_combo.closedRect = Common::Rect(TG_TUNES_COMBO_X, TG_TUNES_COMBO_Y, TG_TUNES_COMBO_X + TG_TUNES_COMBO_WIDTH,
	                                 TG_TUNES_COMBO_Y + TG_TUNES_COMBO_CLOSED_HEIGHT);
	_combo.listRect = Common::Rect(TG_TUNES_COMBO_X, _combo.closedRect.bottom, TG_TUNES_COMBO_X + TG_TUNES_COMBO_WIDTH,
	                               TG_TUNES_COMBO_Y + TG_TUNES_COMBO_TOTAL_HEIGHT);
	_popup.create(_combo.listRect.width(), _combo.listRect.height(), _format);
	_combo.selected = 0;
}

void TunesPlayerEngine::drawDesktop() {
	Graphics::Surface *tile = nullptr;
	Common::Archive *bundle = Common::makeZipArchive(Common::Path("macgui.dat"));

	if (bundle) {
		Common::SeekableReadStream *file = bundle->createReadStreamForMember(Common::Path("scummvm_background.bmp", Common::Path::kNoSeparator));

		if (file) {
			Image::BitmapDecoder decoder;

			if (decoder.loadStream(*file))
				tile = decoder.getSurface()->convertTo(_format);

			delete file;
		}

		delete bundle;
	}

	if (!tile) {
		_backdrop.fillRect(Common::Rect(0, 0, TG_TUNES_SCREEN_WIDTH, TG_TUNES_SCREEN_HEIGHT), color(0, 0x80, 0x80));
		return;
	}

	for (int16 y = 0; y < TG_TUNES_SCREEN_HEIGHT; y += tile->h) {
		for (int16 x = 0; x < TG_TUNES_SCREEN_WIDTH; x += tile->w)
			_backdrop.blitFrom(*tile, Common::Point(x, y));
	}

	tile->free();
	delete tile;
}

void TunesPlayerEngine::drawFrame(const Common::Rect &outer) {
	// Let's draw a cute Windows 3.1 window frame! :-)
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	uint32 gray = color(0xC0, 0xC0, 0xC0);
	uint32 white = color(0xFF, 0xFF, 0xFF);
	uint32 dark = color(0x80, 0x80, 0x80);
	uint32 black = color(0, 0, 0);
	uint32 navy = color(0, 0, 0x80);

	int16 left = outer.left;
	int16 top = outer.top;
	int16 right = outer.right - 1;
	int16 bottom = outer.bottom - 1;

	_backdrop.fillRect(outer, gray);
	_backdrop.frameRect(outer, black);

	// Caption: 18 rows, a black line under it, then the window bevel
	_backdrop.fillRect(Common::Rect(left + 1, top + 1, right, top + 19), navy);
	_backdrop.hLine(left + 1, top + 19, right - 1, black);

	for (int16 row = top + 20; row <= top + 21; row++)
		_backdrop.hLine(left + 1, row, right - 1, white);

	_backdrop.vLine(left + 1, top + 20, bottom - 1, white);
	_backdrop.vLine(right - 1, top + 22, bottom - 1, dark);
	_backdrop.hLine(left + 2, bottom - 1, right - 1, dark);

	// System menu box with the recessed bar
	int16 boxLeft = left + 1;
	int16 barTop = top + 8;

	_backdrop.fillRect(Common::Rect(boxLeft, top + 1, boxLeft + 18, top + 19), gray);
	_backdrop.vLine(boxLeft + 18, top + 1, top + 18, black);
	_backdrop.fillRect(Common::Rect(boxLeft + 2, barTop, boxLeft + 15, barTop + 3), black);
	_backdrop.hLine(boxLeft + 3, barTop + 1, boxLeft + 13, white);
	_backdrop.hLine(boxLeft + 3, barTop + 3, boxLeft + 15, dark);
	_backdrop.vLine(boxLeft + 15, barTop + 1, barTop + 3, dark);

	// Minimize box with the down arrow
	int16 minLeft = right - 18;

	_backdrop.fillRect(Common::Rect(minLeft, top + 1, right, top + 19), gray);
	_backdrop.vLine(minLeft - 1, top + 1, top + 18, black);
	_backdrop.hLine(minLeft, top + 1, right - 2, white);
	_backdrop.vLine(minLeft, top + 1, top + 17, white);
	_backdrop.vLine(right - 2, top + 2, top + 18, dark);
	_backdrop.vLine(right - 1, top + 1, top + 18, dark);
	_backdrop.hLine(minLeft + 1, top + 17, right - 1, dark);
	_backdrop.hLine(minLeft, top + 18, right - 1, dark);

	for (int16 i = 0; i < 4; i++)
		_backdrop.hLine(minLeft + 5 + i, top + 8 + i, minLeft + 11 - i, black);

	int16 titleLeft = boxLeft + 19;
	int16 titleWidth = (minLeft - 1) - titleLeft;
	int16 titleY = top + 1 + (18 - font->getFontHeight()) / 2;

	font->drawString(&_backdrop, "TuneLand Tunes Player", titleLeft, titleY, titleWidth, white, Graphics::kTextAlignCenter);
}

void TunesPlayerEngine::drawWindow() {
	_window.fillRect(Common::Rect(0, 0, TG_TUNES_CLIENT_WIDTH, TG_TUNES_CLIENT_HEIGHT), color(0xC0, 0xC0, 0xC0));

	// First row and column of the client area are the window's highlight
	_window.hLine(0, 0, TG_TUNES_CLIENT_WIDTH - 1, color(0xFF, 0xFF, 0xFF));
	_window.vLine(0, 0, TG_TUNES_CLIENT_HEIGHT - 1, color(0xFF, 0xFF, 0xFF));

	for (int i = 0; i < TG_TUNES_NUM_BUTTONS; i++)
		drawButton(_buttons[i], true);

	drawCombo();
}

void TunesPlayerEngine::drawButton(Button &button, bool force) {
	byte frame;

	if (button.displayOnly)
		frame = button.frame;
	else if (button.pressed)
		frame = 1;
	else
		frame = button.state ? 2 : 0;

	if (!force && frame == button.frame)
		return;

	const Graphics::ManagedSurface &bitmap = _bitmaps[button.bitmap];
	Common::Rect source(button.rect.width() * frame, button.row * button.rect.height(),
	                    button.rect.width() * (frame + 1), (button.row + 1) * button.rect.height());

	if (source.right <= bitmap.w && source.bottom <= bitmap.h)
		_window.blitFrom(bitmap, source, Common::Point(button.rect.left, button.rect.top));

	button.frame = frame;
}

void TunesPlayerEngine::drawCombo() {
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	uint32 white = color(0xFF, 0xFF, 0xFF);
	uint32 black = color(0, 0, 0);
	uint32 gray = color(0xC0, 0xC0, 0xC0);
	uint32 dark = color(0x80, 0x80, 0x80);

	_window.fillRect(_combo.closedRect, white);
	_window.frameRect(_combo.closedRect, black);

	// The drop button covers the right end of the box, borders included
	Common::Rect arrow(_combo.closedRect.right - TG_TUNES_COMBO_BUTTON_WIDTH, _combo.closedRect.top,
	                   _combo.closedRect.right, _combo.closedRect.bottom);
	_window.fillRect(arrow, gray);
	_window.hLine(arrow.left, arrow.top, arrow.right - 2, white);
	_window.vLine(arrow.left, arrow.top, arrow.bottom - 2, white);
	_window.vLine(arrow.right - 2, arrow.top + 1, arrow.bottom - 1, dark);
	_window.vLine(arrow.right - 1, arrow.top, arrow.bottom - 1, dark);
	_window.hLine(arrow.left + 1, arrow.bottom - 2, arrow.right - 1, dark);
	_window.hLine(arrow.left, arrow.bottom - 1, arrow.right - 1, dark);

	// Down arrow with an underline bar
	int16 centerX = arrow.left + 7;
	int16 glyphTop = arrow.top + 6;

	_window.fillRect(Common::Rect(centerX - 1, glyphTop, centerX + 2, glyphTop + 3), black);

	for (int16 i = 0; i < 4; i++)
		_window.hLine(centerX - 3 + i, glyphTop + 3 + i, centerX + 3 - i, black);

	_window.hLine(centerX - 3, glyphTop + 8, centerX + 3, black);

	if (_combo.selected >= 0 && _combo.selected < TG_TUNES_NUM_TUNES) {
		int16 textY = _combo.closedRect.top + (_combo.closedRect.height() - font->getFontHeight()) / 2;

		font->drawString(&_window, kTunes[_combo.selected].name, _combo.closedRect.left + 4, textY,
		                 arrow.left - _combo.closedRect.left - 6, black);
	}

	if (_combo.dropped)
		drawComboList();
}

void TunesPlayerEngine::drawComboList() {
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	uint32 white = color(0xFF, 0xFF, 0xFF);
	uint32 black = color(0, 0, 0);
	uint32 navy = color(0, 0, 0x80);
	uint32 gray = color(0xC0, 0xC0, 0xC0);
	uint32 dark = color(0x80, 0x80, 0x80);
	int visible = comboVisibleItems();
	int itemHeight = font->getFontHeight() + 2;
	bool scrollbar = (TG_TUNES_NUM_TUNES > visible);
	Common::Rect list(0, 0, _combo.listRect.width(), _combo.listRect.height());
	int16 textRight = list.right - 1 - (scrollbar ? TG_TUNES_SCROLLBAR_WIDTH : 0);

	_popup.fillRect(list, white);
	_popup.frameRect(list, black);

	for (int i = 0; i < visible && _combo.top + i < TG_TUNES_NUM_TUNES; i++) {
		int index = _combo.top + i;
		int16 y = list.top + 1 + i * itemHeight;
		Common::Rect row(list.left + 1, y, textRight, y + itemHeight);
		bool highlighted = (index == _combo.hover);

		if (highlighted)
			_popup.fillRect(row, navy);

		font->drawString(&_popup, kTunes[index].name, row.left + 3, y + 1, row.width() - 4, highlighted ? white : black);
	}

	if (!scrollbar)
		return;

	Common::Rect strip(textRight, list.top + 1, list.right - 1, list.bottom - 1);
	int16 buttonHeight = TG_TUNES_SCROLLBAR_WIDTH;
	Common::Rect up(strip.left, strip.top, strip.right, strip.top + buttonHeight);
	Common::Rect down(strip.left, strip.bottom - buttonHeight, strip.right, strip.bottom);
	int16 trackHeight = strip.height() - 2 * buttonHeight;
	int16 thumbHeight = MAX<int16>(8, (int16)(trackHeight * visible / TG_TUNES_NUM_TUNES));
	int16 thumbTop = up.bottom + (int16)((trackHeight - thumbHeight) * _combo.top / (TG_TUNES_NUM_TUNES - visible));
	Common::Rect thumb(strip.left, thumbTop, strip.right, thumbTop + thumbHeight);

	_popup.fillRect(strip, gray);
	_popup.frameRect(up, dark);
	_popup.frameRect(down, dark);
	_popup.fillRect(thumb, dark);
	_popup.frameRect(thumb, black);
}

void TunesPlayerEngine::presentScreen() {
	_surface->blitFrom(_window, _origin);

	if (_combo.dropped)
		_surface->blitFrom(_popup, Common::Point(_origin.x + _combo.listRect.left, _origin.y + _combo.listRect.top));

	if (_surface->isDirty())
		_surface->update();
	else
		g_system->updateScreen();

	g_system->delayMillis(10);
}

void TunesPlayerEngine::setButtonState(Button &button, bool on) {
	button.state = on;
	button.pressed = false;
	drawButton(button, false);
}

void TunesPlayerEngine::setButtonPressed(Button &button, bool pressed) {
	if (button.pressed == pressed)
		return;

	button.pressed = pressed;
	drawButton(button, false);
}

void TunesPlayerEngine::setVolumeIndicator(uint16 level) {
	Button &indicator = _buttons[kButtonVolumeIndicator];

	indicator.frame = (byte)level;
	_volumeLevel = level;
	drawButton(indicator, true);
}

void TunesPlayerEngine::pollEvents() {
	Common::EventManager *eventManager = g_system->getEventManager();
	Common::Event event;

	while (eventManager->pollEvent(event)) {
		Common::Point position = event.mouse - _origin;

		switch (event.type) {
		case Common::EVENT_MOUSEMOVE:
			onMouseMove(position);
			break;

		case Common::EVENT_LBUTTONDOWN:
			onMouseDown(position);
			break;

		case Common::EVENT_LBUTTONUP:
			onMouseUp(position);
			break;

		case Common::EVENT_WHEELUP:
		case Common::EVENT_WHEELDOWN:
			if (_combo.dropped) {
				int maxTop = MAX(0, TG_TUNES_NUM_TUNES - comboVisibleItems());

				_combo.top = CLIP(_combo.top + ((event.type == Common::EVENT_WHEELUP) ? -1 : 1), 0, maxTop);
				drawCombo();
			}

			break;

		default:
			break;
		}
	}
}

void TunesPlayerEngine::onMouseDown(const Common::Point &position) {
	if (_combo.dropped) {
		handleComboMouseDown(position);
		return;
	}

	if (_combo.closedRect.contains(position)) {
		int visible = comboVisibleItems();

		_combo.dropped = true;
		_combo.hover = _combo.selected;
		_combo.top = CLIP(_combo.selected - visible / 2, 0, MAX(0, TG_TUNES_NUM_TUNES - visible));
		drawCombo();
		return;
	}

	for (int i = 0; i < TG_TUNES_NUM_BUTTONS; i++) {
		Button &button = _buttons[i];

		if (button.interactive && button.rect.contains(position)) {
			_captured = &button;
			setButtonPressed(button, true);
			return;
		}
	}
}

void TunesPlayerEngine::onMouseMove(const Common::Point &position) {
	if (_captured)
		setButtonPressed(*_captured, _captured->rect.contains(position));

	if (_combo.dropped) {
		int item = comboItemAt(position);

		if (item != _combo.hover) {
			_combo.hover = item;
			drawCombo();
		}
	}
}

void TunesPlayerEngine::onMouseUp(const Common::Point &position) {
	if (!_captured)
		return;

	Button *button = _captured;
	int index = (int)(button - _buttons);
	bool wasPressed = button->pressed;

	_captured = nullptr;
	button->pressed = false;

	if (!wasPressed)
		return;

	if (button->latching) {
		if (button->state) {
			drawButton(*button, false);
			return;
		}

		button->state = true;
	}

	drawButton(*button, false);
	onButtonClick(index);
}

bool TunesPlayerEngine::handleComboMouseDown(const Common::Point &position) {
	if (_combo.closedRect.contains(position)) {
		closeCombo();
		return true;
	}

	int item = comboItemAt(position);
	if (item >= 0) {
		bool changed = (item != _combo.selected);

		setComboSelection(item);
		closeCombo();

		if (changed)
			onSelectionChanged();

		return true;
	}

	int visible = comboVisibleItems();
	bool scrollbar = (TG_TUNES_NUM_TUNES > visible);
	int16 stripLeft = _combo.listRect.right - 1 - TG_TUNES_SCROLLBAR_WIDTH;

	if (scrollbar && _combo.listRect.contains(position) && position.x >= stripLeft) {
		int maxTop = TG_TUNES_NUM_TUNES - visible;

		if (position.y < _combo.listRect.top + 1 + TG_TUNES_SCROLLBAR_WIDTH)
			_combo.top = MAX(0, _combo.top - 1);
		else if (position.y >= _combo.listRect.bottom - 1 - TG_TUNES_SCROLLBAR_WIDTH)
			_combo.top = MIN(maxTop, _combo.top + 1);
		else if (position.y < _combo.listRect.top + 1 + TG_TUNES_SCROLLBAR_WIDTH + (_combo.listRect.height() - 2 - 2 * TG_TUNES_SCROLLBAR_WIDTH) * _combo.top / TG_TUNES_NUM_TUNES)
			_combo.top = MAX(0, _combo.top - visible);
		else
			_combo.top = MIN(maxTop, _combo.top + visible);

		drawCombo();
		return true;
	}

	closeCombo();
	return true;
}

int TunesPlayerEngine::comboVisibleItems() const {
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);

	return (_combo.listRect.height() - 2) / (font->getFontHeight() + 2);
}

int TunesPlayerEngine::comboItemAt(const Common::Point &position) const {
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	int visible = comboVisibleItems();
	int16 right = _combo.listRect.right - 1 - ((TG_TUNES_NUM_TUNES > visible) ? TG_TUNES_SCROLLBAR_WIDTH : 0);

	if (!_combo.listRect.contains(position) || position.x >= right || position.y < _combo.listRect.top + 1)
		return -1;

	int item = _combo.top + (position.y - _combo.listRect.top - 1) / (font->getFontHeight() + 2);

	return (item < TG_TUNES_NUM_TUNES && item < _combo.top + visible) ? item : -1;
}

void TunesPlayerEngine::setComboSelection(int index) {
	_combo.selected = index;
	_combo.hover = index;
}

void TunesPlayerEngine::closeCombo() {
	Common::Rect list(_origin.x + _combo.listRect.left, _origin.y + _combo.listRect.top,
	                  _origin.x + _combo.listRect.right, _origin.y + _combo.listRect.bottom);

	_combo.dropped = false;
	_combo.hover = -1;
	_surface->blitFrom(_backdrop, list, Common::Point(list.left, list.top));
	drawWindow();
}

void TunesPlayerEngine::onButtonClick(int index) {
	switch (index) {
	case kButtonGoodbye:
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
		playWaveResource(TG_TUNES_FIRST_GOODBYE_WAVE + getRandom15Bit() %TG_TUNES_NUM_GOODBYE_WAVES);
		closePlayer();
		break;

	case kButtonHelp:
		warning("TunesPlayerEngine::onButtonClick(): the help file TUNES.HLP is not supported");
		break;

	case kButtonMascot:
		playWaveResource(TG_TUNES_MASCOT_WAVE);
		break;

	case kButtonPlay: {
		bool wasPaused = _paused;

		setTransportState(kTransportPlaying);

		if (wasPaused)
			resumePausedTune();
		else
			playSelectedTune();

		break;
	}

	case kButtonStop:
		setTransportState(kTransportStopped);
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
		break;

	case kButtonPause:
		if (_playing) {
			setTransportState(kTransportPaused);
			_savedPosition = audioLibrary()->audioCommand(TG_TUNES_COMMAND_GET_POSITION, 0, 0, nullptr);
			audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
			_remaining = _deadline - getEngineMillis();
		} else {
			setTransportState(kTransportStopped);
		}

		break;

	case kButtonPrevious:
	case kButtonNext:
		changeSelection((index == kButtonNext) ? 5 : 4);
		break;

	case kButtonVolumeUp:
		if (_volumeLevel < TG_TUNES_MAX_VOLUME_LEVEL) {
			setVolumeIndicator(_volumeLevel + 1);
			setCdVolumeLevel(_volumeLevel);
		}

		break;

	case kButtonVolumeDown:
		if (_volumeLevel != 0) {
			setVolumeIndicator(_volumeLevel - 1);
			setCdVolumeLevel(_volumeLevel);
		}

		break;

	default:
		break;
	}
}

void TunesPlayerEngine::onSelectionChanged() {
	if (_playing) {
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
		playSelectedTune();
	} else if (_paused) {
		setTransportState(kTransportStopped);
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
	}
}

void TunesPlayerEngine::setTransportState(int state) {
	setButtonState(_buttons[kButtonStop], state == kTransportStopped);
	setButtonState(_buttons[kButtonPlay], state == kTransportPlaying);
	setButtonState(_buttons[kButtonPause], state == kTransportPaused);

	_paused = (state == kTransportPaused);
	_playing = (state == kTransportPlaying);
}

void TunesPlayerEngine::changeSelection(int direction) {
	int current = _combo.selected;

	if (direction != 5 && current <= 0) {
		if (_playing)
			setTransportState(kTransportStopped);

		return;
	}

	int next = (direction == 5) ? ((current < TG_TUNES_NUM_TUNES - 1) ? current + 1 : 0) : current - 1;

	setComboSelection(next);
	drawCombo();

	if (_paused) {
		setTransportState(kTransportStopped);
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
		setButtonState(_buttons[kButtonStop], true);
	} else if (_playing) {
		audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
		playSelectedTune();
	}
}

void TunesPlayerEngine::playSelectedTune() {
	const Tune &tune = kTunes[_combo.selected];
	uint32 track = tune.track;

	if (audioLibrary()->audioCommand(TG_TUNES_COMMAND_PLAY_TRACK, 0, (track << 8) | track, nullptr) == 0) {
		checkCdPresent();
		setTransportState(kTransportStopped);
		return;
	}

	_deadline = getEngineMillis() + tune.lengthMs;
	_savedPosition = track << 24;
}

void TunesPlayerEngine::resumePausedTune() {
	byte range[8];
	uint32 from = _savedPosition;
	uint32 to = (((from >> 24) + 1) & 0xFF) << 24;

	WRITE_LE_UINT32(range, from);
	WRITE_LE_UINT32(range + 4, to);

	if (audioLibrary()->audioCommand(TG_TUNES_COMMAND_PLAY_RANGE, 0, 0, range) != 0) {
		_deadline = getEngineMillis() + _remaining;
	} else {
		checkCdPresent();
		setTransportState(kTransportStopped);
	}
}

void TunesPlayerEngine::autoAdvanceTick() {
	if (getEngineMillis() >= _deadline) {
		_paused = false;
		onButtonClick(kButtonNext);
	}
}

void TunesPlayerEngine::setCdVolumeLevel(uint16 level) {
	uint32 volume = (uint32)level * TG_TUNES_VOLUME_STEP;

	audioLibrary()->audioCommand(TG_TUNES_COMMAND_SET_VOLUME, 0, volume | (volume << 16), nullptr);
}

bool TunesPlayerEngine::checkCdPresent() {
	if (audioLibrary()->audioCommand(TG_TUNES_COMMAND_GET_TRACKS, 0, 0, nullptr) == TG_TUNES_NUM_DISC_TRACKS)
		return true;

	GUIErrorMessage("Please put the TuneLand CD in your CD-ROM drive.");
	return false;
}

void TunesPlayerEngine::closePlayer() {
	_closing = true;
	audioLibrary()->audioCommand(TG_TUNES_COMMAND_STOP_CD, 0, 0, nullptr);
	audioLibrary()->audioExitLibrary();
}

} // End of namespace Topgun
