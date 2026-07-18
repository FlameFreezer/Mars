#include <mars_global.h>

std::mutex mutex;

namespace mars {
	Global& Global::get() noexcept {
		std::unique_lock<std::mutex> lock{ mutex };
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