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
	glm::vec2 Camera2D::getFollowLead() const noexcept {
		return mFollowLead;
	}
	ID Camera2D::getTargetID() const noexcept {
		return mTargetID;
	}
	Camera2D::FollowMode Camera2D::getFollowMode() const noexcept {
		return mFollowMode;
	}
	Camera2D::FollowState Camera2D::getFollowState() const noexcept {
		return mFollowState;
	}
	float Camera2D::getFollowSpeed() const noexcept {
		return mFollowSpeed;
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
		mFollowMode = Camera2D::FollowMode::DONT_FOLLOW;
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
	void Camera2D::setFollowSpeed(float speed) noexcept {
		mFollowSpeed = speed;
	}
	void Camera2D::setFollowLead(glm::vec2 lead) noexcept {
		mFollowLead = lead;
	}
	void Camera2D::signalStateChange(FollowState next) noexcept {
		mNextState = next;
	}
	void Camera2D::moveAndLookAt(glm::vec2 where) noexcept {
		mTargetPosition = where - (0.5f * mScale);
		mPosition = mTargetPosition;
		mFollowMode = Camera2D::FollowMode::FOLLOW_POSITION;
	}
	void Camera2D::lookAt(glm::vec2 where) noexcept {
		mTargetPosition = where - (0.5f * mScale);
		mFollowMode = Camera2D::FollowMode::FOLLOW_POSITION;
	}
	Error<Camera2D::FollowState> Camera2D::checkStateTransitions() const noexcept {
		switch (mFollowState) {
		case FollowState::IDLE:
			if (mFollowMode == FollowMode::FOLLOW_ENTITY) {
				const ComponentSystem<Draw>& sysDraw = ECS::get().system<Component::draw>();
				if (!sysDraw.has(mTargetID)) {
					FATAL(std::format("Target ID {} invalid for Camera2D", mTargetID));
				}
				const glm::vec2 targetCenter = sysDraw.position(mTargetID) + (0.5f * sysDraw.scale(mTargetID));
				if (glm::length(targetCenter - mPosition + (0.5f * mScale)) > glm::length(mDeadzone)) {
					return FollowState::FOLLOWING;
				}
			}
			break;
		case FollowState::RETURNING: {
			if (mFollowMode == FollowMode::FOLLOW_ENTITY) {
				const ComponentSystem<Draw>& sysDraw = ECS::get().system<Component::draw>();
				if (!sysDraw.has(mTargetID)) {
					FATAL(std::format("Target ID {} invalid for Camera2D", mTargetID));
				}
				const glm::vec2 targetCenter = sysDraw.position(mTargetID) + (0.5f * sysDraw.scale(mTargetID));
				if (mPosition + (0.5f * mScale) == targetCenter) {
					return FollowState::IDLE;
				}
			}
			if (mPosition + (0.5f * mScale) == mTargetPosition) {
				return FollowState::IDLE;
			}
			break;
		}
		case FollowState::FOLLOWING: break;
		}
		return FollowState::NULL_STATE;
	}
	Error<noreturn> Camera2D::update(float delta) noexcept {
		// If a state change is buffered, perform it
		if (mNextState != Camera2D::FollowState::NULL_STATE) {
			mFollowState = mNextState;
			mNextState = Camera2D::FollowState::NULL_STATE;
		}
		// Check state transitions
		TRY_INIT(FollowState, next, checkStateTransitions());
		if (next != FollowState::NULL_STATE) {
			mFollowState = next;
		}
		// Do the main update
		switch (mFollowState) {
		case FollowState::FOLLOWING: {
			glm::vec2 target{ mTargetPosition };
			if (mFollowMode == FollowMode::FOLLOW_ENTITY) {
				const ComponentSystem<Draw>& sysDraw = ECS::get().system<Component::draw>();
				if (!sysDraw.has(mTargetID)) {
					FATAL(std::format("Target ID {} invalid for Camera2D", mTargetID));
				}
				//Look at the center of the target object
				target = sysDraw.position(mTargetID) + (0.5f * sysDraw.scale(mTargetID)) - (0.5f * mScale);
			}
			const glm::vec2 distance{ target - mPosition + mFollowLead};
			glm::vec2 movement = glm::normalize(distance) * std::min(glm::length(distance), mFollowSpeed * delta);
			mPosition += movement;
			break;
		}
		case FollowState::RETURNING: {
			glm::vec2 target{ mTargetPosition };
			if (mFollowMode == FollowMode::FOLLOW_ENTITY) {
				const ComponentSystem<Draw>& sysDraw = ECS::get().system<Component::draw>();
				if (!sysDraw.has(mTargetID)) {
					FATAL(std::format("Target ID {} invalid for Camera2D", mTargetID));
				}
				//Look at the center of the target object
				target = sysDraw.position(mTargetID) + (0.5f * sysDraw.scale(mTargetID)) - (0.5f * mScale);
			}
			const glm::vec2 distance{ target - mPosition};
			glm::vec2 movement = glm::normalize(distance) * std::min(glm::length(distance), mFollowSpeed * delta);
			mPosition += movement;
			break;
		}
		case FollowState::IDLE: break;
		}
		return SUCCESS;
	}

	Camera2DBuilder& Camera2DBuilder::setPosition(glm::vec2 pos) noexcept {
		mPosition = pos;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setScale(glm::vec2 scale) noexcept {
		mScale = scale;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setDeadzone(glm::vec2 deadzone) noexcept {
		mDeadzone = deadzone;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setTargetPosition(glm::vec2 targetPos) noexcept {
		mTargetPosition = targetPos;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setFollowLead(glm::vec2 lead) noexcept {
		mFollowLead = lead;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setFollowSpeed(float speed) noexcept {
		mFollowSpeed = speed;
		return *this;
	}
	Camera2D Camera2DBuilder::build() const noexcept {
		Camera2D cam{};
		cam.mPosition = mPosition;
		cam.mScale = mScale;
		cam.mDeadzone = mDeadzone;
		cam.mTargetPosition = mTargetPosition;
		cam.mFollowLead = mFollowLead;
		cam.mFollowSpeed = mFollowSpeed;
		return cam;
	}
}

template<>
Error<mars::Camera2D> JSON::valueTo(const JSON::Value& value) noexcept {
	if (value.getType() != JSON::Type::jobject) {
		FATAL(std::format("When constructing mars::Camera2D, expected {}, got {}", JSON::typeToString(JSON::Type::jobject), JSON::typeToString(value.getType())));
	}
	mars::Camera2DBuilder cameraBuilder{};
	const JSON::Object& json{ value.getObject().value()};

	if (json.contains("position")) {
		TRY_INIT(glm::vec2, pos, JSON::valueTo<glm::vec2>(json.at("position")));
		cameraBuilder.setPosition(pos);
	}
	if (json.contains("scale")) {
		TRY_INIT(glm::vec2, scale, JSON::valueTo<glm::vec2>(json.at("scale")));
		cameraBuilder.setScale(scale);
	}
	if (json.contains("deadzone")) {
		TRY_INIT(glm::vec2, deadzone, JSON::valueTo<glm::vec2>(json.at("deadzone")));
		cameraBuilder.setDeadzone(deadzone);
	}
	if (json.contains("targetPosition")) {
		TRY_INIT(glm::vec2, targetPosition, JSON::valueTo<glm::vec2>(json.at("targetPosition")));
		cameraBuilder.setTargetPosition(targetPosition);
	}
	if (json.contains("followLead")) {
		TRY_INIT(glm::vec2, followLead, JSON::valueTo<glm::vec2>(json.at("followLead")));
		cameraBuilder.setFollowLead(followLead);
	}
	if (json.contains("followSpeed")) {
		TRY_INIT(float, followSpeed, JSON::valueTo<float>(json.at("followSpeed")));
		cameraBuilder.setFollowSpeed(followSpeed);
	}
	return cameraBuilder.build();
}

