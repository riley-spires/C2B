#include "../../c2b.h"



int main(int argc, char **argv) {
    // Enable rebuild self
    c2b::Build::rebuild_self(argc, argv, __FILE__);

    // First we are going to build GLFW
    c2b::Build build("GLFW");

    // Prepare raylib source code
    c2b::Utils::fetch("https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz", "5.5.tar.gz");
    if (!c2b::Utils::file_exists("raylib-5.5")) {
        c2b::Utils::decompress("5.5.tar.gz");
    }

    // Prep build settings
    build.set_output_dir("lib");
    build.set_build_type(c2b::BuildType::LIB);
    build.set_std({ .version_flag="-std=c99", .extension = ".c" });
    build.set_export_compile_commands(false);

    // Add build directories
    build.append_include_dir("raylib-5.5/src/external/glfw/include");
    build.append_source_dir("raylib-5.5/src/external/glfw/src", false);

    // Add required flags for GLFW
    build.append_flag("D_GLFW_X11", "fpermissive", "O1");

    // Build GLFW
    if (build.build() != 0) {
        c2b::Loggers::stderr.log_fatal("Failed to build GLFW");
    }

    // Clear build to build raylib next
    build.clear("raylib");

    // Prep build settings
    build.set_output_dir("lib");
    build.set_build_type(c2b::BuildType::LIB);
    build.set_compiler(c2b::Compilers::GCC);
    build.set_std({ .version_flag = "-std=c99", .extension = ".c" });
    build.set_export_compile_commands(false);

    // Add build directories
    build.append_source_dir("raylib-5.5/src", false);
    build.append_include_dir("raylib-5.5/src/external/glfw/include");

    // Add necessary flags to build raylib
    build.append_flag("DPLATFORM_DESKTOP", "D_GLFW_X11", "O1");

    // Link GLFW from build above
    build.append_link_dir("lib");
    build.append_link_file("GLFW");

    // Build raylib
    if (build.build() != 0) {
        c2b::Loggers::stderr.log_fatal("Failed to build raylib");
    }

    // Clear the build to build our final program
    build.clear("main");

    // Add build directories
    build.append_source_dir("src");
    build.append_include_dir("raylib-5.5/src");

    // Link to raylib
    build.append_link_dir("lib");
    build.append_link_file("raylib");

    // Run the final program
    return build.build_and_run();
}
