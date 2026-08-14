#include <camera/mars_camera2D.h>
#include <ecs/mars_ecs.h>
#include <mars_global.h>
#include <mars_math.h>

namespace mars {
	glm::vec2 Camera2D::getPosition() const noexcept {
		return mPosition;
	}
	glm::vec2 Camera2D::getScale() const noexcept {
		return mScale;
	}
	glm::vec2 Camera2D::getTargetPosition() const noexcept {
		return mTargetPosition;
	}
	glm::vec2 Camera2D::getCenter() const noexcept {
		return mPosition + 0.5f * mScale;
	}
    float Camera2D::getSpeed() const noexcept {
        return mSpeed;
    }
    void Camera2D::setPosition(glm::vec2 pos) noexcept {
        mPosition = pos;
    }
    void Camera2D::setScale(glm::vec2 scale) noexcept {
        mScale = scale;
    }
	void Camera2D::setTargetPosition(glm::vec2 pos) noexcept {
		mTargetPosition = pos;
	}
    void Camera2D::setSpeed(float speed) noexcept {
        mSpeed = speed;
    }
    void Camera2D::setTargetCenter(glm::vec2 where) noexcept {
        mTargetPosition = where - 0.5f * mScale;
    }
	void Camera2D::lookAt(glm::vec2 where) noexcept {
		mPosition = where - 0.5f * mScale;
	}
	void Camera2D::update(float delta) noexcept {
        if (mPosition == mTargetPosition) return;

        const glm::vec2 velocityImpact = glm::normalize(mTargetPosition - mPosition) * mSpeed * delta;
        if (glm::distance(mTargetPosition, mPosition) < glm::length(velocityImpact)) {
            mPosition = mTargetPosition;
        }
        else {
            mPosition += velocityImpact;
        }
	}

	Camera2DBuilder& Camera2DBuilder::setPosition(glm::vec2 pos) noexcept {
		mPosition = pos;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setScale(glm::vec2 scale) noexcept {
		mScale = scale;
		return *this;
	}
	Camera2DBuilder& Camera2DBuilder::setTargetPosition(glm::vec2 targetPos) noexcept {
		mTargetPosition = targetPos;
		return *this;
	}
    Camera2DBuilder& Camera2DBuilder::setSpeed(float speed) noexcept {
        mSpeed = speed;
        return *this;
    }
	Camera2D Camera2DBuilder::build() const noexcept {
		Camera2D cam{};
		cam.mPosition = mPosition;
		cam.mScale = mScale;
		cam.mTargetPosition = mTargetPosition;
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
	float metersPerPixel = 1.0f;

	if (json.contains("isPixels")) {
		TRY_INIT(bool, isPixels, JSON::valueTo<bool>(json.at("isPixels")));
		if (isPixels) {
			metersPerPixel = 1.0f / mars::Global::get().pixelsPerMeter();
		}
	}
	if (json.contains("position")) {
		TRY_INIT(glm::vec2, pos, JSON::valueTo<glm::vec2>(json.at("position")));
		cameraBuilder.setPosition(pos * metersPerPixel);
	}
	if (json.contains("scale")) {
		TRY_INIT(glm::vec2, scale, JSON::valueTo<glm::vec2>(json.at("scale")));
		cameraBuilder.setScale(scale * metersPerPixel);
	}
	if (json.contains("targetPosition")) {
		TRY_INIT(glm::vec2, targetPosition, JSON::valueTo<glm::vec2>(json.at("targetPosition")));
		cameraBuilder.setTargetPosition(targetPosition * metersPerPixel);
	}
    if (json.contains("speed")) {
        TRY_INIT(float, speed, JSON::valueTo<float>(json.at("speed")));
        cameraBuilder.setSpeed(speed * metersPerPixel);
    }
	return cameraBuilder.build();
}

