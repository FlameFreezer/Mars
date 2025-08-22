pub usingnamespace @cImport({
    @cInclude("string.h");
    @cDefine("SDL_HINT_VIDEO_SYNC_WINDOW_OPERATIONS", "1");
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_vulkan.h");
    @cInclude("vulkan/vulkan.h");
    @cDefine("STB_IMAGE_IMPLEMENTATION", {});
    @cInclude("stb_image.h");
});
