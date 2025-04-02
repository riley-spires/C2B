#include "../../qbs.h"


int main(int argc, char **argv) {
    qbs::Build::rebuild_self(argc, argv, __FILE__);

    qbs::Build build("main");

    build.enable_export_compile_commands();

    build.set_std(qbs::Stds::CXX23);
    build.enable_warnings();

    build.append_include_dir("include");
    build.append_source_dir("src");

    int ret = build.build_and_run("Quoopie");

    qbs::Cmd cmd("rm", "-rf", "build");

    return ret + cmd.run();
}
