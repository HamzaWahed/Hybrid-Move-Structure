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

    // fetch sdsl-lite and include sdsl header files
    exe.addLibraryPath(b.path("lib/sdsl-lite/include/sdsl"));

    // include project header files
    exe.addIncludePath(b.path("include/"));

    exe.addCSourceFile(.{ .file = b.path("src/hybrid_move_structure.cpp") });

    b.installArtifact(exe);
}
