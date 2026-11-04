#include "animation/mars_animation_player.h"

namespace mars {
    void AnimationPlayer::update(float delta) noexcept {
        // Don't play if paused
        if (mIsPaused) return;
        // Don't play if associated entity is invalid
        if (!sysDraw.has(mEntityID) or mEntityID == nullID) return;
        // Don't play if animation index is invalid
        if (mAnimationIndex >= mAnimations.size()) return;
        // Get the selected animation
        Animation& anim = mAnimations[mAnimationIndex];
        // Don't play if we've reached the end of the animation
        if (mFrameIndex >= anim.frames.size()) return;

        mTimeAccumulated += delta;
        const float secondsPerFrame = 1.0f / anim.framesPerSecond;
        if (mTimeAccumulated >= secondsPerFrame) {
            mTimeAccumulated -= secondsPerFrame;
            // Move on to the next frame
            mFrameIndex++;

            // Keep the frame index within bounds
            if (mFrameIndex >= anim.frames.size()) {
                if (anim.isLooped) mFrameIndex %= anim.frames.size();
                else mFrameIndex = anim.frames.size() - 1;
            }

            sysDraw[mEntityID].spriteIndex = anim.frames[mFrameIndex].spriteIndex;
            sysDraw[mEntityID].texture = mAnimations[mAnimationIndex].frames[mFrameIndex].texture;
        }
    }

    void AnimationPlayer::play(u32 animationIndex, u32 startingFrame) noexcept {
        mIsPaused = false;
        mAnimationIndex = animationIndex;
        mTimeAccumulated = 0.0f;
        mFrameIndex = startingFrame;
        sysDraw[mEntityID].spriteIndex = mAnimations[mAnimationIndex].frames[mFrameIndex].spriteIndex;
        sysDraw[mEntityID].texture = mAnimations[mAnimationIndex].frames[mFrameIndex].texture;
    }

    void AnimationPlayer::pause() noexcept {
        mIsPaused = true;
    }

    void AnimationPlayer::resume() noexcept {
        mIsPaused = false;
    }

    bool AnimationPlayer::isPaused() const noexcept {
        return mIsPaused;
    }

    void AnimationPlayer::setEntity(Entity entity) noexcept {
        mEntityID = entity.id();
    }

    void AnimationPlayer::addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped, HeapArray<AnimationFrame>&& frames) noexcept {
        Animation anim;
        anim.frames = std::move(frames);
        for(AnimationFrame& frame : anim.frames) {
            frame.texture = texture;
        }
        anim.framesPerSecond = framesPerSecond;
        anim.isLooped = isLooped;
        mAnimations.push_back(std::move(anim));
    }

    void AnimationPlayer::addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped) noexcept {
        Animation anim;
        anim.framesPerSecond = framesPerSecond;
        anim.isLooped = isLooped;
        anim.frames.resize(texture->sprites().size());
        for(size_t i = 0; i < anim.frames.size(); i++) {
            anim.frames[i].spriteIndex = i;
            anim.frames[i].texture = texture;
        }
        mAnimations.push_back(std::move(anim));
    }

    u32 AnimationPlayer::currentAnimation() const noexcept {
        return mAnimationIndex;
    }
}