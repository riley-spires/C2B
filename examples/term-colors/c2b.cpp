#include <c2b.h>


int main(int argc, char** argv) {
    c2b::Build::rebuild_self(argc, argv, __FILE__);
    c2b::Build build("main");

    build.append_source_dir("src");
    build.append_link_file("c2b");

    return build.build_and_run();
}
