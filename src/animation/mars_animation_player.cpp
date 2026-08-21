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
        if (mAnimationSpriteIndex >= anim.spriteIndices.size()) return;

        mTimeAccumulated += delta;
        const float secondsPerFrame = 1.0f / anim.framesPerSecond;
        if (mTimeAccumulated >= secondsPerFrame) {
            mTimeAccumulated -= secondsPerFrame;
            // Move on to the next frame
            mAnimationSpriteIndex++;
            // If the animation is looped, return us to the first frame if beyond the last one
            if (anim.isLooped) {
                mAnimationSpriteIndex %= anim.spriteIndices.size();
            }
            // Otherwise stay on the last frame
            else if (mAnimationSpriteIndex == anim.spriteIndices.size()) {
                --mAnimationSpriteIndex;
                mIsPaused = true;
            }
            sysDraw[mEntityID].spriteIndex = anim.spriteIndices[mAnimationSpriteIndex];
        }
    }

    void AnimationPlayer::play(u32 animationIndex, u32 startingFrame) noexcept {
        mIsPaused = false;
        mAnimationIndex = animationIndex;
        mTimeAccumulated = 0.0f;
        mAnimationSpriteIndex = startingFrame;
        sysDraw[mEntityID].spriteIndex = mAnimations[mAnimationSpriteIndex].spriteIndices[mAnimationSpriteIndex];
        sysDraw[mEntityID].texture = mAnimations[mAnimationSpriteIndex].texture;
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

    void AnimationPlayer::addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped, HeapArray<u32>&& spriteIndices) noexcept {
        Animation anim;
        anim.texture = texture;
        anim.framesPerSecond = framesPerSecond;
        anim.spriteIndices = std::move(spriteIndices);
        anim.isLooped = isLooped;
        mAnimations.push_back(std::move(anim));
    }

    void AnimationPlayer::addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped) noexcept {
        Animation anim;
        anim.texture = texture;
        anim.framesPerSecond = framesPerSecond;
        anim.isLooped = isLooped;
        anim.spriteIndices.resize(texture->sprites().size());
        for(size_t i = 0; i < anim.spriteIndices.size(); i++) {
            anim.spriteIndices[i] = i;
        }
        mAnimations.push_back(std::move(anim));
    }
}