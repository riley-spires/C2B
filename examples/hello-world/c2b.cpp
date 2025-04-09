#include "../../c2b.h"


int main(int argc, char **argv) {
    c2b::Build::rebuild_self(argc, argv, __FILE__);

    c2b::Build build("main");

    build.set_std(c2b::Stds::CXX23);
    build.enable_warnings();

    build.append_include_dir("include");
    build.append_source_dir("src");

    int ret = build.build_and_run("Quoopie");


    return ret;
}
