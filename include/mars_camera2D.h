#pragma once

#include <format>

#include <glm/glm.hpp>

#include "mars_types.h"
#include "error.h"
#include "jsonparser.h"

namespace mars {
	class Camera2D {
	public:
		friend class Camera2DBuilder;
		enum class FollowMode : u8 {
			DONT_FOLLOW,
			FOLLOW_ENTITY,
			FOLLOW_POSITION
		};
		enum class FollowState : u8 {
			IDLE,
			FOLLOWING,
			RETURNING,
			NULL_STATE
		};
		//Getters
		glm::vec2 getPosition() const noexcept;
		glm::vec2 getScale() const noexcept;
		glm::vec2 getDeadzone() const noexcept;
		glm::vec2 getTargetPosition() const noexcept;
		glm::vec2 getFollowLead() const noexcept;
		ID getTargetID() const noexcept;
		FollowMode getFollowMode() const noexcept;
		FollowState getFollowState() const noexcept;
		float getFollowSpeed() const noexcept;
		//Setters
		Error<noreturn> setTargetID(ID id, bool doFollowEntity = true) noexcept;
		Error<noreturn> setFollowMode(FollowMode mode) noexcept;
		void setPosition(glm::vec2 pos) noexcept;
		void setScale(glm::vec2 scale) noexcept;
		void setDeadzone(glm::vec2 deadzone) noexcept;
		void setTargetPosition(glm::vec2 pos) noexcept;
		void setFollowSpeed(float speed) noexcept;
		void setFollowLead(glm::vec2 lead) noexcept;
		void signalStateChange(FollowState next) noexcept;
		// Changes the position of the camera such that the argument is at the center of its viewport. Also sets the follow mode to FOLLOW_POSITION.
		void moveAndLookAt(glm::vec2 where) noexcept;
		// Sets the target position such that the argument is at the center of its viewport, but does not move the camera. Also sets the follow mode to FOLLOW_POSITION.
		void lookAt(glm::vec2 where) noexcept;
		Error<noreturn> update(float delta) noexcept;
	private:
		Error<FollowState> checkStateTransitions() const noexcept;
		glm::vec2 mPosition{};
		glm::vec2 mScale{};
		glm::vec2 mDeadzone{};
		glm::vec2 mTargetPosition{};
		glm::vec2 mFollowLead{};
		ID mTargetID{ nullID };
		float mFollowSpeed{};
		FollowMode mFollowMode{ FollowMode::DONT_FOLLOW };
		FollowState mFollowState{ FollowState::IDLE };
		FollowState mNextState{ FollowState::NULL_STATE };
	};

	class Camera2DBuilder {
	public:
		Camera2DBuilder& setPosition(glm::vec2 pos) noexcept;
		Camera2DBuilder& setScale(glm::vec2 scale) noexcept;
		Camera2DBuilder& setDeadzone(glm::vec2 deadzone) noexcept;
		Camera2DBuilder& setTargetPosition(glm::vec2 targetPos) noexcept;
		Camera2DBuilder& setFollowLead(glm::vec2 lead) noexcept;
		Camera2DBuilder& setFollowSpeed(float speed) noexcept;
		Camera2D build() const noexcept;
	private:
		glm::vec2 mPosition{};
		glm::vec2 mScale{};
		glm::vec2 mDeadzone{};
		glm::vec2 mTargetPosition{};
		glm::vec2 mFollowLead{};
		float mFollowSpeed{};
	};
}

template<>
Error<mars::Camera2D> JSON::valueTo(const JSON::Value& value) noexcept;
