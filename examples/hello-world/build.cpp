#include "qbs.h"



int main() {
    qbs::Build build("main");

    /*build.add_source_file("main.cpp");*/

    build.add_flags("-Wall", "-Wextra");

    build.set_cxx_version(qbs::CxxVersion::cpp23);

    build.add_include_path("include");
    build.add_source_dir("src");

    int ret = build.build_and_run();

    qbs::Cmd cmd("rm", "-rf", "main");

    return ret + cmd.run();
}
