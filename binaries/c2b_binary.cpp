#include <fstream>
#include <iostream>
#include "../c2b.h"

int display_help();
int new_project(std::string project_name);
int build_project(std::string project_name, bool always_rebuild);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return display_help();
    }
    c2b::Logger logger = c2b::Loggers::stdout;

    std::string subcommand = argv[1];

    if (subcommand == "new") {
        if (argc < 3) {
            return display_help();
        }

        std::string project_name = argv[2];

        return new_project(project_name);
    } else if (subcommand == "build") {
        std::string project_file_path = "c2b.cpp";
        bool always_rebuild = false;
        if (argc >= 3) {
            std::string arg2 = argv[2];

            if (arg2 == "-B") {
                always_rebuild = true;
            } else {
                project_file_path = arg2;
            }
        }  

        return build_project(project_file_path, always_rebuild);
    } else if (subcommand == "help") {
        return display_help();
    } else {
        logger.log_error("Invalid subcommand: " + subcommand);
        return display_help();
    }

    return 0;
}

int display_help() {
    c2b::Logger logger = c2b::Loggers::stdout;

    logger.log_info("Usage: c2b_binary <new|build|help> {options}");
    logger.log_info("     new: Creates a new project");
    logger.log_info("          Usage: c2b_binary new <project_name> {flags}");
    logger.log_info("     build: Builds the project in the current directory");
    logger.log_info("          Usage: c2b_binary build [project_file_path = c2b.cpp]");
    logger.log_info("          Flags:");
    logger.log_info("              -B: Always rebuild the project");
    logger.log_info("     help: Displays this help message");
    return 1;
}

int new_project(std::string project_name) {
    c2b::Logger logger = c2b::Loggers::stdout;
    logger.log_info("Generating " + project_name + "...");

    c2b::Utils::make_dir_if_not_exists(project_name + "/build");
    c2b::Utils::make_dir_if_not_exists(project_name + "/src");

    std::ofstream file(project_name + "/c2b.cpp");

    file << "#include <c2b.h>" << std::endl << std::endl;

    file << "int main(int argc, char* argv[]) {" << std::endl;

    file << "    c2b::Build::rebuild_self(argc, argv, __FILE__);" << std::endl;
    file << "    c2b::Build build(\"main\");" << std::endl << std::endl;

    file << "    build.append_source_dir(\"src\");" << std::endl << std::endl;

    file << "    return build.build_and_run();" << std::endl;
    file << "}" << std::endl;

    file.close();

    file.open(project_name + "/src/main.cpp");

    file << "#include <iostream>" << std::endl << std::endl;

    file << "int main() {" << std::endl;
    file << "    std::cout << \"Hello World!\" << std::endl;" << std::endl;
    file << "    return 0;" << std::endl;
    file << "}" << std::endl;

    file.close();


    c2b::Cmd cmd;

    cmd.append("g++", project_name + "/c2b.cpp", "-o", project_name + "/build" + "/build-project");

    return cmd.run();
}

int build_project(std::string project_file_path, bool always_rebuild) {
    c2b::Cmd cmd;
    if (always_rebuild) {
        cmd.append("rm", "-rf", "build");
        cmd.run();
        cmd.clear();
    }

    c2b::Utils::make_dir_if_not_exists("build");


    if (always_rebuild || !c2b::Utils::file_exists("build/build-project")) {
        cmd.append("g++", "c2b.cpp", "-o", "build/build-project");
        cmd.run();
        cmd.clear();
    }

    cmd.append("./build/build-project");
    return cmd.run_redirect_output();
}
