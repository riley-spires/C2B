#include <unistd.h>
#include "../c2b.h"

void display_help(c2b::Logger logger);
int install(c2b::Logger logger);
int uninstall(c2b::Logger logger);

int main(int argc, char** argv) {
    c2b::Logger logger = c2b::Loggers::stdout;
    if (argc != 2) {
        display_help(logger);
        return 1;
    }
    const std::string subcommand = argv[1];

    if (subcommand == "help") {
        display_help(logger);
        return 0;
    }

    if (getuid() != 0) {
        logger.log_fatal("You must be root to (un)install C2B!");
    }

    if (subcommand == "install") {
        return install(logger);
    } else if (subcommand == "uninstall") {
        uninstall(logger);
    } else {
        logger.log_error("Invalid subcommand!");
        display_help(logger);
        return 1;
    }
}

void display_help(c2b::Logger logger) {
    logger.log_info("Usage: c2b_installer <install|uninstall|help>");
    logger.log_info("     install: Installs C2B");
    logger.log_info("     uninstall: Uninstalls C2B");
    logger.log_info("     help: Displays this help message");

}

int install(c2b::Logger logger) {
    const std::string header_install_path = "/usr/local/include";
    const std::string binary_install_path = "/usr/local/bin";

    logger.log_info("Installing C2B...");

    c2b::Cmd cmd;

    cmd.append("cp", "../c2b.h", header_install_path + "/c2b.h");
    if (cmd.run() != 0) {
        logger.log_fatal("Failed to install C2B header!");
    }
    cmd.clear();
    cmd.append("g++", "c2b_binary.cpp", "-o", "c2b");
    if (cmd.run() != 0) {
        logger.log_fatal("Failed to compile C2B binary!");
    }
    cmd.clear();
    cmd.append("cp", "c2b", binary_install_path + "/c2b");
    if (cmd.run() != 0) {
        logger.log_fatal("Failed to install C2B binary!");
    }

    logger.log_info("C2B installed successfully!");

    return 0;
}

int uninstall(c2b::Logger logger) {
    logger.log_info("Uninstalling C2B...");
    c2b::Cmd cmd;
    cmd.append("rm", "-rf", "/usr/local/include/c2b.h");
    cmd.run();
    cmd.clear();
    cmd.append("rm", "-f", "/usr/local/bin/c2b");
    cmd.run();

    logger.log_info("C2B uninstalled successfully!");
    return 0;
}
