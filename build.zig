const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const mars_mod = b.createModule(.{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize
    });
    const c_mod = b.createModule(.{
        .root_source_file = b.path("src/c.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true
    });
    c_mod.addIncludePath(b.path("include/GLFW"));
    c_mod.addIncludePath(b.path("include/VULKAN"));
    c_mod.linkSystemLibrary("glfw", .{});
    c_mod.linkSystemLibrary("Vulkan", .{});
    mars_mod.addImport("c", c_mod);

    const sourceOptions = b.addOptions();
    sourceOptions.addOption(bool, "isDebugBuild", optimize == std.builtin.OptimizeMode.Debug);
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

    const exe_mod = b.createModule(.{
        .root_source_file = b.path("app/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    exe_mod.addImport("Mars", mars_mod);

    const exe = b.addExecutable(.{
        .name = "zig-vulkan",
        .root_module = exe_mod
    });

    b.installArtifact(exe);

    const run_command = b.addRunArtifact(exe);
    run_command.step.dependOn(b.getInstallStep());
    run_command.step.dependOn(&shader_compile_step.step);

    if(b.args) |args| {
        run_command.addArgs(args);
    }

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_command.step);
}
