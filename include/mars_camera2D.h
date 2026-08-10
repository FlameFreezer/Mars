#pragma once

#include <glm/glm.hpp>

#include "error.h"
#include "jsonparser.h"

namespace mars {
	class Camera2D {
	public:
		friend class Camera2DBuilder;
		//Getters
		glm::vec2 getPosition() const noexcept;
		glm::vec2 getScale() const noexcept;
		glm::vec2 getTargetPosition() const noexcept;
		glm::vec2 getFollowLead() const noexcept;
		glm::vec2 getCenter() const noexcept;
        float getSpeed() const noexcept;
		void setPosition(glm::vec2 pos) noexcept;
		void setScale(glm::vec2 scale) noexcept;
		void setTargetPosition(glm::vec2 pos) noexcept;
        void setTargetCenter(glm::vec2 where) noexcept;
        void setSpeed(float speed) noexcept;
		void lookAt(glm::vec2 where) noexcept;
		void update(float delta) noexcept;
	private:
		glm::vec2 mPosition{};
		glm::vec2 mScale{};
		glm::vec2 mTargetPosition{};
        float mSpeed{};
	};

	class Camera2DBuilder {
	public:
		Camera2DBuilder& setPosition(glm::vec2 pos) noexcept;
		Camera2DBuilder& setScale(glm::vec2 scale) noexcept;
		Camera2DBuilder& setTargetPosition(glm::vec2 targetPos) noexcept;
        Camera2DBuilder& setSpeed(float speed) noexcept;
		Camera2D build() const noexcept;
	private:
		glm::vec2 mPosition{};
		glm::vec2 mScale{};
		glm::vec2 mTargetPosition{};
        float mSpeed{};
	};
}

template<>
Error<mars::Camera2D> JSON::valueTo(const JSON::Value& value) noexcept;
