#pragma once

#include <limits>
#include <memory>
#include <vector>

#include "assetmanager/mars_texture.h"
#include "ecs/mars_component_system.h"
#include "ecs/mars_entity.h"

namespace mars {
    struct AnimationFrame {
        std::shared_ptr<Texture> texture;
        u32 spriteIndex;
    };
    struct Animation {
        HeapArray<AnimationFrame> frames;
        float framesPerSecond;
        bool isLooped;
    };
    class AnimationPlayer {
    public:
        AnimationPlayer(mars::ComponentSystem<mars::Draw>& sysDraw) : sysDraw(sysDraw) {}
        void update(float delta) noexcept;
        void addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped, HeapArray<AnimationFrame>&& frames) noexcept;
        // This overload assumes that the animation should consume every frame of the texture
        void addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped) noexcept;
        void play(u32 animationIndex, u32 startingFrame = 0) noexcept;
        void pause() noexcept;
        void resume() noexcept;
        void setEntity(Entity entity) noexcept;
        bool isPaused() const noexcept;
        u32 currentAnimation() const noexcept;
    private:
        mars::ComponentSystem<mars::Draw>& sysDraw;
        std::vector<Animation> mAnimations;
        //Animation is mAnimations[mAnimationIndex]
        u32 mAnimationIndex = std::numeric_limits<u32>::max();
        //Frame is mAnimations[mAnimationIndex].frames[mFrameIndex]
        u32 mFrameIndex = 0;
        ID mEntityID = nullID;
        float mTimeAccumulated = 0.0f;
        bool mIsPaused = true;
    };
}