const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    
    const c_mod = b.createModule(.{
        .root_source_file = b.path("src/c.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true
    });
    c_mod.addIncludePath(b.path("include/"));
    if(target.result.os.tag == .windows) {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const vulkanSDKPath: []u8 = try getVulkanSDKPathWindows(arena.allocator());
        c_mod.addLibraryPath(.{.cwd_relative = try joinPaths(arena.allocator(), vulkanSDKPath, "\\Lib")});
        c_mod.addIncludePath(.{ .cwd_relative = try joinPaths(arena.allocator(), vulkanSDKPath, "\\Include")});
    }
    c_mod.linkSystemLibrary("vulkan-1", .{});

    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });
    const sdl_lib = sdl_dep.artifact("SDL3");
    const sdl_test_lib = sdl_dep.artifact("SDL3_test");
    _ = sdl_test_lib;
    c_mod.linkLibrary(sdl_lib);

    const utils_mod = b.createModule(.{
        .root_source_file = b.path("src/utils.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true
    });
    utils_mod.addImport("c", c_mod);

    const test_mod = b.createModule(.{
        .root_source_file = b.path("tests/tests.zig"),
        .target = target,
        .optimize = .Debug,
        .link_libc = true
    });
    test_mod.addImport("c", c_mod);
    test_mod.addImport("Utils", utils_mod);

    const mars_mod = b.addModule("Mars", .{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize
    });
    mars_mod.addImport("c", c_mod);
    mars_mod.addImport("Utils", utils_mod);

    const sourceOptions = b.addOptions();
    sourceOptions.addOption(bool, "isDebugBuild", optimize == .Debug);
    mars_mod.addOptions("buildOpts", sourceOptions);

    const shader_compile_step = b.addSystemCommand(&.{
        "slangc",
        "shader.slang",
        "-target", "spirv",
        "-profile", "spirv_1_4",
        "-emit-spirv-directly",
        "-fvk-use-entrypoint-name", 
        "-entry", "vertMain",
        "-entry", "fragMain",
        "-o", "slang.spv"
    });
    shader_compile_step.addFileInput(b.path("shaders/shader.slang"));
    shader_compile_step.setCwd(b.path("shaders"));

    const mars_lib = b.addLibrary(.{
        .name = "Mars",
        .linkage = .static,
        .root_module = mars_mod,
        .version = .{
            .major = 0,
            .minor = 1,
            .patch = 0
        }
    });
    b.installArtifact(mars_lib);

    const exe_mod = b.createModule(.{
        .root_source_file = b.path("app/main.zig"),
        .target = target,
        .optimize = optimize
    });
    exe_mod.addImport("Mars", mars_mod);

    const exe = b.addExecutable(.{
        .name = "MarsApp",
        .root_module = exe_mod
    });

    b.installArtifact(exe);

    const run_command = b.addRunArtifact(exe);
    run_command.step.dependOn(b.getInstallStep());
    run_command.step.dependOn(&shader_compile_step.step);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_command.step);

    const marsTests = b.addTest(.{
        .root_module = test_mod
    });
    const test_command = b.addRunArtifact(marsTests);
    const test_step = b.step("test", "Run the tests");
    test_step.dependOn(&test_command.step);
}

fn getVulkanSDKPathWindows(allocator: std.mem.Allocator) ![]u8 {
    var sdkpathWCHAR: [1024:0]u16 = undefined;
    var sdkpathCHAR: []u8 = undefined;
    const envnameCHAR = "VK_SDK_PATH";
    var envnameWCHAR: [12:0]u16 = undefined;
    for(0..12) |i| envnameWCHAR[i] = envnameCHAR[i];
    const ccount = try std.os.windows.GetEnvironmentVariableW(envnameWCHAR[0..], sdkpathWCHAR[0..], sdkpathWCHAR.len);
    sdkpathCHAR = try allocator.alloc(u8, ccount);
    for(0..ccount) |i| sdkpathCHAR[i] = @intCast(sdkpathWCHAR[i]);
    return sdkpathCHAR[0..ccount];
}

fn joinPaths(allocator: std.mem.Allocator, absolute: []const u8, subpath: []const u8) ![]const u8 {
    var buffer: []u8 = try allocator.alloc(u8, absolute.len + subpath.len); 
    for(0..absolute.len) |i| buffer[i] = absolute[i];
    for(0..subpath.len) |i| buffer[absolute.len + i] = subpath[i];
    return buffer;
}