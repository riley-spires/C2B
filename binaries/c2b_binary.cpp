#include <fstream>
#include <iostream>
#include "c2b.h"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: c2b <project_name>" << std::endl;
        return 1;
    }

    std::string project_name = argv[1];

    std::cout << "Generating basic project for " << project_name << "..." << std::endl;

    c2b::Utils::make_dir_if_not_exists(project_name);

    std::ofstream file(project_name + "/c2b.cpp");

    file << "#include <c2b.h>" << std::endl << std::endl;

    file << "int main(int argc, char* argv[]) {" << std::endl;

    file << "\tc2b::Build::build_self(argc, argv, __FILE__);" << std::endl;
    file << "\tc2b::Build build(\"main\")" << std::endl;

    file << "\treturn build.build_and_run();" << std::endl;
    file << "}" << std::endl;

    file.close();

    c2b::Cmd cmd;

    cmd.append("g++", project_name + "/c2b.cpp", "-o", project_name + "/c2b");

    return cmd.run();
}
