#pragma once
#include <string_view>
#include <cstdio>

#ifndef MARS_DEBUG_ALL
#define MARS_DEBUG_ALL false
#endif

#ifndef MARS_DEBUG_INPUT
#define MARS_DEBUG_INPUT false
#endif

namespace mars {
	namespace debug {
		constexpr bool doInputDebug = MARS_DEBUG_INPUT | MARS_DEBUG_ALL;
		void inputLog(std::string_view message, std::FILE* ostrm = stdout) noexcept;
	}
}