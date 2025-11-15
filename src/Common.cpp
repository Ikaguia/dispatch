#include <Common.hpp>

namespace Dispatch {
	namespace UI {
		raylib::Font defaultFont{};
		raylib::Font emojiFont{"resources/fonts/NotoEmoji-Regular.ttf", 32, (int[]){ 0x2694, 0x2713, 0x2714, 0x1F3C3, 0x1F4AC, 0x1F5F8, 0x1F6E1, 0x1F9E0, 0 }, 8};
		raylib::Font symbolsFont{"resources/fonts/NotoSansSymbols2-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};
		raylib::Font fontTitle{"resources/fonts/NotoSans-Bold.ttf", 32};
		raylib::Font fontText{"resources/fonts/NotoSans-Regular.ttf", 22};
		raylib::Color bgLgt{230,230,220,255};
		raylib::Color bgMed{200,200,190,255};
		raylib::Color bgDrk{150,150,140,255};
		raylib::Color shadow{0,0,0,60};
		raylib::Color textColor{30,30,30,255};
		// raylib::Color bgLgt{244, 225, 203};
		// raylib::Color bgMed{198, 175, 145};
		// raylib::Color bgDrk{114, 100, 86};
		// raylib::Color textColor = ColorLerp(BROWN, BLACK, 0.8f);
		// raylib::Color shadow = ColorAlpha(BLACK, 0.4);
	}
}
