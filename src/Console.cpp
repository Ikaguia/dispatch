#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace Dispatch::Debug {

	class CrashProofLogger {
		// Internal helper to mirror output to two streams simultaneously
		class TeeBuffer : public std::streambuf {
			std::streambuf *sb1, *sb2;
		public:
			TeeBuffer(std::streambuf* s1, std::streambuf* s2) : sb1(s1), sb2(s2) {}
			virtual int overflow(int c) override {
				if (c == EOF) return !EOF;
				return (sb1->sputc(c) == EOF || sb2->sputc(c) == EOF) ? EOF : c;
			}
			virtual int sync() override {
				return (sb1->pubsync() == 0 && sb2->pubsync() == 0) ? 0 : -1;
			}
		};

		// Static members to keep the log file and buffers alive until program exit
		static inline std::ofstream logFile;
		static inline TeeBuffer *teeOut = nullptr, *teeErr = nullptr;

	public:
		static void Init(const std::string& filename = "latest_log.txt") {
#ifdef _WIN32
			// 1. Attach/Allocate Windows Console
			if (AllocConsole()) {
				// Use wide-character safe freopen_s for modern Windows apps
				FILE* dummy;
				freopen_s(&dummy, "CONOUT$", "w", stdout);
				freopen_s(&dummy, "CONOUT$", "w", stderr);
			}
#endif
			// 2. Open log file with auto-flush (unitbuf)
			// This ensures data is written to disk before the next line of code executes
			logFile.open(filename.c_str(), std::ios::out);

			if (logFile.is_open()) {
				logFile <<  std::ios::app << std::ios::unitbuf;

				// Use std::cout and std::cerr, NOT std::stdout or std::stderr
				teeOut = new TeeBuffer(std::cout.rdbuf(), logFile.rdbuf());
				teeErr = new TeeBuffer(std::cerr.rdbuf(), logFile.rdbuf());

				// Apply the redirection
				std::cout.rdbuf(teeOut);
				std::cerr.rdbuf(teeErr);
			}

			std::cout << "\n--- Logger Initialized ---" << std::endl;
		}
	};
}

#include <exception>
#include <iostream>

void myTerminateHandler() {
    // This is your last chance to log something before the process dies
    std::cerr << "FATAL: std::terminate called! (Unhandled exception or noexcept violation)" << std::endl;
    
    // If you have a stack trace library like cpptrace:
    // cpptrace::generate_trace().print();

    std::abort(); // Required to actually end the process
}

#include <csignal>
#include <iostream>

void signalHandler(int signal) {
    std::cerr << "\nCRITICAL SIGNAL RECEIVED: ";
    switch (signal) {
        case SIGSEGV: std::cerr << "Segmentation Fault (Invalid Memory Access)"; break;
        case SIGABRT: std::cerr << "Abort Signal (Internal Error)"; break;
        case SIGFPE:  std::cerr << "Floating Point Exception (Division by Zero)"; break;
        case SIGILL:  std::cerr << "Illegal Instruction"; break;
        default:      std::cerr << "Unknown Signal (" << signal << ")"; break;
    }
    std::cerr << std::endl;

    // Log the stack trace here if possible
    // Note: Technically, calling many functions inside a signal handler is "unsafe,"
    // but during a crash, it's often worth the risk to get diagnostic data.

    std::exit(signal); 
}

void AttachConsole() {
	Dispatch::Debug::CrashProofLogger::Init("log.txt");
	std::set_terminate(myTerminateHandler);
	std::signal(SIGSEGV, signalHandler);
	std::signal(SIGABRT, signalHandler);
	std::signal(SIGFPE, signalHandler);
}