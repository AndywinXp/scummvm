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

namespace Topgun {

const PlainGameDescriptor topgunGames[] = {
	{ "tuneland", "TuneLand Starring Howie Mandel" },
	{ "tunesplayer", "TuneLand Tunes Player" },
	{ "pythonwaste", "Monty Python's Complete Waste of Time" },
	{ "battlebeast", "Battle Beast" },
	{ "junglegames", "Timon & Pumbaa's Jungle Games" },
	{ "aceventura", "Ace Ventura: The CD-Rom Game" },
	{ "greatword", "Howie's Great Word Adventure" },
	{ "bestshot", "Bill Plympton's Take Your Best Shot" },
	{ "headshot", "Bill Plympton's Take Your Best Shot: Head Shot" },
	{ "hotshot", "Bill Plympton's Take Your Best Shot: Hot Shot" },
	{ "lineshot", "Bill Plympton's Take Your Best Shot: Line Shot" },
	{ "peterwolf", "Peter and the Wolf" },
	{ "arcadeamerica", "Arcade America" },
	{ "holygrail", "Monty Python & the Quest for the Holy Grail" },
	{ "coldblooded", "Cold Blooded" },
	{ "dominion", "Dominion: Storm Over Gift 3" },
	{ "7thlevelsamplers", "7th Level Samplers" },
	{ "burper", "Disney's Hot Shots: Burper" },
	{ "bugdrop", "Disney's Hot Shots: Bug Drop" },
	{ "hippohop", "Disney's Hot Shots: Hippo Hop" },
	{ "junglepinball", "Disney's Hot Shots: Jungle Pinball" },
	{ "topsyturvy", "Disney's The Hunchback of Notre Dame: Topsy Turvy Games" },
	{ "djalibowling", "Disney's Hot Shots: Djali Bowling" },
	{ "upsydaisy", "Disney's Hot Shots: Upsy Daisy" },
	{ "virgilreality", "The Universe According to Virgil Reality" },
	{ 0, 0 }
};

const char *const directoryGlobs[] = {
	"BEAST",
	"JUNGLE",
	"GREATWRD",
	"TUNELAND",
	"PYTHON",
	"ACE",
	"BESTACT",
	"BESTBIG",
	"BESTDEMO",
	"ARCADE",
	"GRAIL",
	"SAMPLER",
	"SAMPLERS",
	"BESTSHOT",
	"CBLOODED",
	"DOMINION",
	"BURPER",
	"BUGDROP",
	"HIPPOHOP",
	"PINBALL",
	"TOPSY",
	"BOWL",
	"UPSY",
	"VIRGIL",
	"WORD",
	"WORD_AR",
	"WORD_NR",
	nullptr
};

const ADGameDescription gameDescriptions[] = {
	{
		"tuneland",
		"",
		AD_ENTRY1s("HERALD.BIN", "9e811ce530048d37d1412f9ec64f0097", 162259),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"",
		AD_ENTRY1s("HERALD.BIN", "8fe2cc420d654c804128220bd7092ddb", 152828),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"",
		AD_ENTRY1s("HERALD.BIN", "cdce3bf8047334e15055e00d9d025068", 140086),
		Common::FR_FRA,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"Demo",
		AD_ENTRY1s("SAMPLER.BIN", "9f25f15a70effb7f4c0c1d93cfa884e8", 120268),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"1996",
		AD_ENTRY1s("TUNELAND.BIN", "06ab966f21744ad7a45868a4ab326f8d", 571278),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"1996",
		AD_ENTRY1s("TUNELAND.BIN", "9e623be057412c4e533d59e5a110af82", 571278),
		Common::RU_RUS,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tunesplayer",
		"",
		AD_ENTRY1s("TUNES.EXE", "59193f3574a3b9bd6cde56aa73415db2", 410656),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO1(GUIO_NONE)
	},

	{
		"tunesplayer",
		"",
		AD_ENTRY1s("JUKEGERM.EXE", "32b9a0275808aadeeeb9ec2bd52fd848", 307376),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO1(GUIO_NONE)
	},

	{
		"tunesplayer",
		"",
		AD_ENTRY1s("JUKEFREN.EXE", "fb1ceed8f72fe9afbf32dae2647b8680", 307472),
		Common::FR_FRA,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO1(GUIO_NONE)
	},

	{
		"pythonwaste",
		"",
		AD_ENTRY1s("HERALD.BIN", "7c57a11624726d2be3c0487716adf33d", 287772),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"pythonwaste",
		"Demo",
		AD_ENTRY1s("HERALD.BIN", "03378ba9a38c16e084b0ad5ea1b774f6", 287718),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"battlebeast",
		"",
		AD_ENTRY1s("BEAST.BIN", "f501fdf6a9a6ac7c141abf9b5c5a41e2", 4214131),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"battlebeast",
		"Demo",
		AD_ENTRY1s("BEAST.BIN", "c6581fffaf617ec2a1a173751f09d6e7", 348),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		"",
		AD_ENTRY1s("JUNGMAIN.BIN", "49e89089db55c65b0f507f2718720254", 7148491),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		"",
		AD_ENTRY1s("JUNGMAIN.BIN", "2848632c56f4775b20e6af82f99a3bfe", 7224580),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		"",
		AD_ENTRY1s("JUNGMAIN.BIN", "76aecef25e36dd95e663b8cd3aa619fd", 7187831),
		Common::FR_FRA,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		"",
		AD_ENTRY1s("JUNGMAIN.BIN", "446413cef0cea2fc4fbbaf10d838caa4", 7074945),
		Common::JA_JPN,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"",
		AD_ENTRY1s("ACE.BIN", "29ed24f8deced71e5f15bb6181b0d7da", 14339127),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"",
		AD_ENTRY1s("ACE.BIN", "a51e25697c45a785516cba1c808c9713", 85007),
		Common::IT_ITA,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_UNSUPPORTED,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"",
		AD_ENTRY1s("ACE.BIN", "099f4c3145ea790f771481c314607051", 85042),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_UNSUPPORTED,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"",
		AD_ENTRY1s("ACE.BIN", "56feca084262311daa90243f04ccba46", 85123),
		Common::ES_ESP,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_UNSUPPORTED,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"",
		AD_ENTRY1s("ACE.BIN", "80c8c1beca4b4b60b130f88b03ae5c3c", 84969),
		Common::PL_POL,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_UNSUPPORTED,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"",
		AD_ENTRY1s("ACE.BIN", "01a00ac60d5bfb4daa29d1960f52a93e", 123138),
		Common::RU_RUS,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_UNSUPPORTED,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"aceventura",
		"Demo",
		AD_ENTRY1s("ACE_DEMO.BIN", "9fe87ba911aed663175b81dd9492279b", 5816086),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"greatword",
		"",
		AD_ENTRY1s("WORD.BIN", "135d2ebd891b5c14c18c9ed7edd6724f", 24695855),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"greatword",
		"",
		AD_ENTRY1s("WORD.BIN", "f02caf0a5c261101a87c7e2e409f5957", 24767060),
		Common::NL_NLD,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"greatword",
		"",
		AD_ENTRY1s("WORD.BIN", "a361bba069091faf6dff8f7522ac3706", 29664525),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"greatword",
		"New spelling",
		AD_ENTRY1s("WORD.BIN", "e8e5ba3a8562930c7670db1a168c49aa", 29664531),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"greatword",
		"Demo",
		AD_ENTRY1s("GWA.BIN", "4739ebee440a2b20c823f8bf5453b63d", 6876981),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"bestshot",
		"",
		AD_ENTRY1s("BESTSHOT.BIN", "545e12e0f1486066cf50bdf3fd11478f", 5231400),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"bestshot",
		"",
		AD_ENTRY1s("BESTSHOT.BIN", "2769bdfbf2b82d812a69d95cb2a8ef76", 13236232),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"7thlevelsamplers",
		"Feb 1995",
		AD_ENTRY1s("DEMO.BIN", "061fe4bbb40b0328740906a1b9c6d884", 1568269),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"bestshot",
		"Demo",
		AD_ENTRY1s("BESTSHOT.BIN", "32529dee1ab03d889a14a8957d7df68c", 53836),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"headshot",
		"",
		AD_ENTRY1s("HEADSHOT.BIN", "1302447c1546da2e2a8bdb4683215fa1", 298182),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"hotshot",
		"",
		AD_ENTRY1s("HOTSHOT.BIN", "66c70765e0a9b4f80cb78b9ea24f4230", 293493),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"lineshot",
		"",
		AD_ENTRY1s("LINESHOT.BIN", "9320599edafe8236038ea8dd1c9a8524", 685729),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"peterwolf",
		"",
		AD_ENTRY1s("PETER.BIN", "ae2a2da529000b9a6c3873342dfad390", 346270),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"arcadeamerica",
		"",
		AD_ENTRY1s("ARCADE.BIN", "0d6d93e1c7b5e253170a88549ff95a33", 6804081),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"arcadeamerica",
		"",
		AD_ENTRY1s("ARCADE.BIN", "839a5e4e2ebb36dc85f6257d99ce833d", 6863598),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"arcadeamerica",
		"Demo",
		AD_ENTRY1s("ARCADE.BIN", "8699e16ee42b0342e040892a4993bf02", 5609683),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"7thlevelsamplers",
		"Dec 1995",
		AD_ENTRY1s("DEMO.BIN", "907c8b29c7be7364d0af6d903a70606d", 4742267),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"7thlevelsamplers",
		"Dec 1995",
		AD_ENTRY1s("DEMO.BIN", "59f822a647181ec917063ffeef05d2b4", 4730258),
		Common::DE_DEU,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"holygrail",
		"",
		AD_ENTRY1s("GRAIL.BIN", "5886e1a19d66360bb950aca332a6d2b7", 753342),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"holygrail",
		"",
		AD_ENTRY1s("GRAIL.BIN", "c2b8eef48ef9ae61893335987839caa5", 753419),
		Common::JA_JPN,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"holygrail",
		"Demo",
		AD_ENTRY1s("GRLDEMO.BIN", "9e1cca0a32b6c9a5177824c2a75e20ad", 6950049),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"coldblooded",
		"Demo",
		AD_ENTRY1s("COLDDEMO.BIN", "abd85780e149a2c31868701e1fe6beef", 6299597),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"dominion",
		"Demo",
		AD_ENTRY1s("SCRNDEMO.BIN", "1487e42aabfdbb2dce41c48f7476d94c", 12468095),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"burper",
		"",
		AD_ENTRY1s("JUNGBURP.BIN", "9f6e29eb00617f0445a01997f0cf8fad", 5345562),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"bugdrop",
		"",
		AD_ENTRY1s("JUNGBUGD.BIN", "b47a9255d17505f01289f90465a70672", 5248267),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"hippohop",
		"",
		AD_ENTRY1s("JUNGHIPP.BIN", "5afad76c261485a4414eb7878af42e5f", 10972954),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglepinball",
		"",
		AD_ENTRY1s("JUNGPINB.BIN", "6524cd2516983541fe47dabbb9620729", 5293559),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"topsyturvy",
		"",
		AD_ENTRY1s("TOPSCHIS.BIN", "73d002d02dc6011f5a01696e26afc650", 10565900),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"junglegames",
		"Demo",
		AD_ENTRY1s("JUNGDEM1.BIN", "6745afc5592db2eff495711bec4e0abe", 403030),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"djalibowling",
		"",
		AD_ENTRY1s("TOPSBOWL.BIN", "504b4c1280a165fe809b9cbbdd5e0617", 12299904),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"upsydaisy",
		"",
		AD_ENTRY1s("TOPSUPSY.BIN", "f411a1a828ac137123d1a5207857c76e", 10311226),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"upsydaisy",
		"Game Shots",
		AD_ENTRY1s("TOPSUPSY.BIN", "7a913241b686cfcdc1c8503dedc5eb7a", 10311264),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"virgilreality",
		"",
		AD_ENTRY2s("VIRGIL.BIN", "28bfa22ab531d2f70140305fcc79d6c2", 46354,
			"VIRLAB.BIN", "e9c0fe8cf15ce5068fdfb6ea2750f804", 2228599),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"virgilreality",
		"Demo",
		AD_ENTRY2s("LIBRARY.BIN", "03c4289eac432fb694e0c3a715b85cfd", 4707471,
			"SAMPLER3.BIN", "1643920225eab3d987c787a5f3302c70", 186982),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	{
		"tuneland",
		"Demo 1996",
		AD_ENTRY1s("TUNELAND.BIN", "89b5cc085aa3dbad366c7af3d488e297", 22209766),
		Common::EN_ANY,
		Common::kPlatformWindows,
		ADGF_UNSTABLE | ADGF_DEMO,
		GUIO2(GAMEOPTION_SCREEN_800X600, GAMEOPTION_HIGH_BIT_DEPTH)
	},

	AD_TABLE_END_MARKER
};

} // End of namespace Topgun
