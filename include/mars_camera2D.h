#pragma once

#include <format>

#include <glm/glm.hpp>

#include "mars_types.h"
#include "error.h"

namespace mars {
	class Camera2D {
	public:
		enum class FollowMode : u8 {
			STATIC,
			FOLLOW_ENTITY,
			FOLLOW_POSITION
		};
		//Getters
		glm::vec2 getPosition() const noexcept;
		glm::vec2 getScale() const noexcept;
		glm::vec2 getDeadzone() const noexcept;
		glm::vec2 getTargetPosition() const noexcept;
		ID getTargetID() const noexcept;
		FollowMode getFollowMode() const noexcept;
		//Setters
		Error<noreturn> setTargetID(ID id, bool doFollowEntity = true) noexcept;
		Error<noreturn> setFollowMode(FollowMode mode) noexcept;
		void setPosition(glm::vec2 pos) noexcept;
		void setScale(glm::vec2 scale) noexcept;
		void setDeadzone(glm::vec2 deadzone) noexcept;
		void setTargetPosition(glm::vec2 pos) noexcept;
		void lookAt(glm::vec2 where) noexcept;
		void setLookAtTarget(glm::vec2 where) noexcept;
	private:
		glm::vec2 mPosition{};
		glm::vec2 mScale{};
		glm::vec2 mDeadzone{};
		glm::vec2 mTargetPosition{};
		//Target MUST have a draw component
		ID mTargetID{ nullID };
		FollowMode mFollowMode{ FollowMode::STATIC };
	};
}
