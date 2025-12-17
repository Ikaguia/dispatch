#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
	#include <iostream>
	void AttachConsole() {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		std::cout << "Console attached!" << std::endl;
	}
#else
	void AttachConsole() {}
#endif
