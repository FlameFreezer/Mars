#include <mars_camera2D.h>
#include <mars_ecs.h>

namespace mars {
	glm::vec2 Camera2D::getPosition() const noexcept {
		return mPosition;
	}
	glm::vec2 Camera2D::getScale() const noexcept {
		return mScale;
	}
	glm::vec2 Camera2D::getDeadzone() const noexcept {
		return mDeadzone;
	}
	glm::vec2 Camera2D::getTargetPosition() const noexcept {
		return mTargetPosition;
	}
	ID Camera2D::getTargetID() const noexcept {
		return mTargetID;
	}
	Camera2D::FollowMode Camera2D::getFollowMode() const noexcept {
		return mFollowMode;
	}
	Error<noreturn> Camera2D::setTargetID(ID id, bool doFollowEntity) noexcept {
		const ComponentSystem<Draw>& sysDraw = ECS::get().system<Component::draw>();
		if (!sysDraw.has(id)) {
			FATAL(std::format("Entity with ID {} doesn't have a draw component", id));
		}
		mTargetID = id;
		if (doFollowEntity) {
			mTargetPosition = sysDraw.position(mTargetID) - mScale;
			mFollowMode = Camera2D::FollowMode::FOLLOW_ENTITY;
		}
		return SUCCESS;
	}
	Error<noreturn> Camera2D::setFollowMode(Camera2D::FollowMode mode) noexcept {
		if (mode == Camera2D::FollowMode::FOLLOW_ENTITY and mTargetID == nullID) {
			FATAL("Tried to set the follow mode of a Camera2D to FOLLOW_ENTITY, but the camera had no target entity");
		}
		mFollowMode = mode;
		return SUCCESS;
	}
	void Camera2D::setPosition(glm::vec2 pos) noexcept {
		mPosition = pos;
		mFollowMode = Camera2D::FollowMode::STATIC;
	}
	void Camera2D::setScale(glm::vec2 scale) noexcept {
		mScale = scale;
	}
	void Camera2D::setDeadzone(glm::vec2 deadzone) noexcept {
		mDeadzone = deadzone;
	}
	void Camera2D::setTargetPosition(glm::vec2 pos) noexcept {
		mTargetPosition = pos;
		mFollowMode = Camera2D::FollowMode::FOLLOW_POSITION;
	}
	void Camera2D::lookAt(glm::vec2 where) noexcept {
		mTargetPosition = where - mScale;
		mPosition = mTargetPosition;
		mFollowMode = Camera2D::FollowMode::FOLLOW_POSITION;
	}
	void Camera2D::setLookAtTarget(glm::vec2 where) noexcept {
		mTargetPosition = where - mScale;
		mFollowMode = Camera2D::FollowMode::FOLLOW_POSITION;
	}
}
