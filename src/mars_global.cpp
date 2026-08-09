#include <mars_global.h>

namespace mars {
    std::mutex Global::mutex {};

	Global& Global::get() noexcept {
		std::unique_lock<std::mutex> lock{ Global::mutex };
		static Global instance{};
		return instance;
	}
	float Global::pixelsPerMeter() const noexcept {
		return mPixelsPerMeter;
	}
	void Global::setPixelsPerMeter(float pixelsPerMeter) noexcept {
		std::unique_lock<std::mutex> lock{ mutex };
		mPixelsPerMeter = pixelsPerMeter;
	}
}