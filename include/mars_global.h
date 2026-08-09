#pragma once

#include <mutex>

namespace mars {
	class Global {
	public:
		static Global& get() noexcept;
		float pixelsPerMeter() const noexcept;
		void setPixelsPerMeter(float pixelsPerMeter) noexcept;
	private:
		Global() noexcept = default;
		float mPixelsPerMeter{ 1.0f };
        static std::mutex mutex;
	};

}
