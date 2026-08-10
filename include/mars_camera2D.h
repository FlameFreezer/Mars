#pragma once

#include <glm/glm.hpp>

#include "error.h"
#include "jsonparser.h"

namespace mars {
	class Camera2D {
	public:
		friend class Camera2DBuilder;
		// Getters
        // Returns the world-space position of the top-right corner of the camera.
		glm::vec2 getPosition() const noexcept;
		glm::vec2 getScale() const noexcept;
		glm::vec2 getTargetPosition() const noexcept;
        // Returns the world-space position of the center of the camera's viewport.
		glm::vec2 getCenter() const noexcept;
        float getSpeed() const noexcept;
        // Setters
        // Sets the world-space position of the top-right corner of the camera.
		void setPosition(glm::vec2 pos) noexcept;
		void setScale(glm::vec2 scale) noexcept;
        // Sets the world-space target position for the top-right corner of the camera. When update 
        // is called, the camera will try to move such that its top-right corner ends up at this position.
		void setTargetPosition(glm::vec2 pos) noexcept;
        // Sets the world-space target position for the center of the camera's viewport. When
        // update is called, the camera will try to move such that the center of its viewport is at
        // this position.
        void setTargetCenter(glm::vec2 where) noexcept;
        void setSpeed(float speed) noexcept;
        // Relocate the camera such that the center of its viewport is at this world-space 
        // position.
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
