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

#ifndef TOPGUN_TUNES_PLAYER_H
#define TOPGUN_TUNES_PLAYER_H

#include "common/events.h"
#include "common/formats/winexe_ne.h"
#include "common/rect.h"
#include "graphics/managed_surface.h"
#include "graphics/screen.h"

#include "topgun/topgun.h"

#define TG_TUNES_CLIENT_WIDTH   364
#define TG_TUNES_CLIENT_HEIGHT  164
#define TG_TUNES_SCREEN_WIDTH   640
#define TG_TUNES_SCREEN_HEIGHT  480
#define TG_TUNES_WINDOW_WIDTH   370
#define TG_TUNES_WINDOW_HEIGHT  190
#define TG_TUNES_CLIENT_LEFT    1
#define TG_TUNES_CLIENT_TOP     21
#define TG_TUNES_NUM_TUNES      43
#define TG_TUNES_NUM_BITMAPS    7
#define TG_TUNES_FIRST_BITMAP   11
#define TG_TUNES_NUM_BUTTONS    12

namespace Topgun {

class TunesPlayerEngine : public TopgunEngine {
public:
	TunesPlayerEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~TunesPlayerEngine() override;

protected:
	Common::Error run() override;
	bool rtStartup(bool runChecks) override;
	uint16 mainLoop() override;

private:
	enum ButtonIndex {
		kButtonStop,
		kButtonPlay,
		kButtonPause,
		kButtonPrevious,
		kButtonNext,
		kButtonVolumeUp,
		kButtonVolumeDown,
		kButtonMascot,
		kButtonVolumeIndicator,
		kButtonLabel,
		kButtonGoodbye,
		kButtonHelp
	};

	enum TransportState {
		kTransportStopped = 1,
		kTransportPlaying = 2,
		kTransportPaused = 3
	};

	struct Tune {
		uint16 track;
		uint32 lengthMs;
		const char *name;
	};

	struct Button {
		Common::Rect rect;
		byte bitmap;
		byte row;
		bool latching;
		bool interactive;
		bool displayOnly;
		bool state;
		bool pressed;
		byte frame;

		Button() : bitmap(0), row(0), latching(false), interactive(true), displayOnly(false), state(false),
			pressed(false), frame(0) {}
	};

	struct ComboBox {
		Common::Rect closedRect;
		Common::Rect listRect;
		int selected;
		int top;
		int hover;
		bool dropped;

		ComboBox() : selected(0), top(0), hover(-1), dropped(false) {}
	};

	static const Tune kTunes[TG_TUNES_NUM_TUNES];

	// Resources
	bool loadBitmap(uint16 id, Graphics::ManagedSurface &surface);
	bool loadBitmaps();
	void playWaveResource(uint16 id);

	// Controls
	void createControls();
	uint32 color(byte red, byte green, byte blue) const;
	void drawDesktop();
	void drawFrame(const Common::Rect &outer);
	void drawWindow();
	void drawButton(Button &button, bool force);
	void drawCombo();
	void drawComboList();
	void presentScreen();
	void setButtonState(Button &button, bool on);
	void setButtonPressed(Button &button, bool pressed);
	void setVolumeIndicator(uint16 level);

	// Mouse
	void pollEvents();
	void onMouseDown(const Common::Point &position);
	void onMouseMove(const Common::Point &position);
	void onMouseUp(const Common::Point &position);
	bool handleComboMouseDown(const Common::Point &position);
	int comboItemAt(const Common::Point &position) const;
	int comboVisibleItems() const;
	void setComboSelection(int index);
	void closeCombo();

	// Logic
	void onButtonClick(int index);
	void onSelectionChanged();
	void setTransportState(int state);
	void changeSelection(int direction);
	void playSelectedTune();
	void resumePausedTune();
	void autoAdvanceTick();
	void setCdVolumeLevel(uint16 level);
	bool checkCdPresent();
	void closePlayer();

	Graphics::PixelFormat _format;
	Graphics::Screen *_surface = nullptr;
	Graphics::ManagedSurface _window;
	Graphics::ManagedSurface _backdrop;
	Graphics::ManagedSurface _popup;
	Common::Point _origin;
	Common::NEResources _resources;
	Graphics::ManagedSurface _bitmaps[TG_TUNES_NUM_BITMAPS];
	Button _buttons[TG_TUNES_NUM_BUTTONS];
	ComboBox _combo;
	Button *_captured = nullptr;
	bool _playing = false;
	bool _paused = false;
	bool _closing = false;
	uint16 _volumeLevel = 0;
	uint32 _deadline = 0;
	uint32 _savedPosition = 0;
	uint32 _remaining = 0;
};

} // End of namespace Topgun

#endif // TOPGUN_TUNES_PLAYER_H
