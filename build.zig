const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "hybrid_move_exec",
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibCpp();
    exe.linkLibrary("sdsl");

    // fetch sdsl-lite and include sdsl header files
    exe.addLibraryPath(b.path("lib/sdsl-lite/build"));

    // include project header files
    exe.addIncludePath(b.path("include/"));

    exe.addCSourceFile(.{ .file = b.path("src/hybrid_move_structure.cpp") });

    b.installArtifact(exe);

    std.debug.print("Built executable\n", .{});

    const run_exe = b.addRunArtifact(exe);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);
}
