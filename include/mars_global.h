#pragma once

#include <mutex>

namespace mars {
	class Global {
	public:
		static Global& get() noexcept;
		float pixelsPerMeter() const noexcept;
		void setPixelsPerMeter(float pixelsPerMeter) noexcept;

        Global(const Global&) = delete;
        Global(Global&&) = delete;
        Global& operator=(const Global&) = delete;
        Global& operator=(Global&&) = delete;
	private:
		Global() noexcept = default;
		float mPixelsPerMeter{ 1.0f };
        static std::mutex mutex;
	};

}
