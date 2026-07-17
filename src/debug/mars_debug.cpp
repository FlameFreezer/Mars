#include <mars_debug.h>

#include <print>
#include <cstdio>
#include <string_view>

namespace mars {
	namespace debug {
		void inputLog(std::string_view message, std::FILE* ostrm) noexcept {
			if constexpr (doInputDebug) {
				std::println(ostrm, "From Input: {}", message);
			}
		}
	}
}