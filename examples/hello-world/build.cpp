#include "../../qbs.h"



int main() {
    qbs::Build build("main");

    build.set_std(qbs::Stds::CXX23);
    build.enable_warnings();
    build.append_include_dir("include");
    build.append_source_dir("src");

    int ret = build.build_and_run("Quoopie");

    qbs::Cmd cmd("rm", "-rf", "output");

    return ret + cmd.run();
}
