#pragma once

#include <initializer_list>
#include <memory>
#include <vector>
#include "assetmanager/mars_texture.h"
#include "jsonparser.h"

namespace mars {
    struct Animation {
        std::shared_ptr<Texture> texture;
        float framesPerSecond;
        HeapArray<u32> spriteIndices;
        bool isLooped;
    };
    class AnimationPlayer {
    public:
        AnimationPlayer(ID entityID) noexcept;
        void update(float delta) noexcept;
        void addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped, std::initializer_list<u32> spriteIndices) noexcept;
        // This overload assumes that the animation should consume every frame of the texture
        void addAnimation(std::shared_ptr<Texture> texture, float framesPerSecond, bool isLooped) noexcept;
        Error<noreturn> loadAnimations(const JSON::Value& json) noexcept;
        void play(u32 animationIndex, u32 startingFrame = 0) noexcept;
        void pause() noexcept;
        void resume() noexcept;
        bool isPaused() const noexcept;
    private:
        std::vector<Animation> mAnimations;
        //Animation is mAnimations[mAnimationIndex]
        u32 mAnimationIndex = 0;
        //Sprite index is mAnimations[mAnimationIndex].spriteIndices[mAnimationSpriteIndex]
        u32 mAnimationSpriteIndex = 0;
        ID mEntityID = nullID;
        float mTimeAccumulated = 0.0f;
        bool mIsPaused = true;
    };
}