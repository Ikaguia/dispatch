#include <raylib-cpp.hpp>

#include <Common.hpp>

// Basic ASCII (SPACE to ~)
int fontChars[] = {
	// ASCII 32–126
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
	 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
	 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
	 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
	 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
	 92, 93, 94, 95, 96, 97, 98, 99,100,101,
	102,103,104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,120,121,
	122,123,124,125,126,

	// Latin-1 Supplement (accents)
	0x00C0, // À
	0x00C1, // Á
	0x00C2, // Â
	0x00C3, // Ã
	0x00C7, // Ç
	0x00C9, // É
	0x00CA, // Ê
	0x00CD, // Í
	0x00D3, // Ó
	0x00DA, // Ú

	0x00E0, // à
	0x00E1, // á
	0x00E2, // â
	0x00E3, // ã
	0x00E7, // ç
	0x00E9, // é
	0x00EA, // ê
	0x00ED, // í
	0x00F3, // ó
	0x00FA, // ú

	0 // <-- terminator
};

namespace Dispatch::UI {
	raylib::Font defaultFont{};
	raylib::Font emojiFont{"resources/fonts/NotoEmoji-Regular.ttf", 32, (int[]){ 0x2694, 0x2713, 0x2714, 0x1F3AF, 0x1F3C3, 0x1F4AC, 0x1F5F8, 0x1F6D1, 0x1F6E1, 0x1F9E0, 0 }, 10};
	raylib::Font symbolsFont{"resources/fonts/NotoSansSymbols2-Regular.ttf", 32, (int[]){ 0x2605, 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 5};
	raylib::Font fontTitle{"resources/fonts/NotoSans-Bold.ttf", 32, fontChars, sizeof(fontChars)/sizeof(fontChars[0]) - 1};
	raylib::Font fontText{"resources/fonts/NotoSans-Regular.ttf", 22, fontChars, sizeof(fontChars)/sizeof(fontChars[0]) - 1};
	raylib::Color bgLgt{244, 225, 203};
	raylib::Color bgMed{198, 175, 145};
	raylib::Color bgDrk{114, 100, 86};
	raylib::Color textColor = ColorLerp(BROWN, BLACK, 0.8f);
	raylib::Color shadow = ColorAlpha(BLACK, 0.4);
}
