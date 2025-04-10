#include <iostream>
#include <unistd.h>
#include "../c2b.h"


int main() {
    if (getuid() != 0) {
        std::cout << "You must be root to install C2B!" << std::endl;
        return 1;
    }

    const std::string header_install_path = "/usr/local/include";
    const std::string binary_install_path = "/usr/local/bin";

    std::cout << "Installing C2B..." << std::endl;

    c2b::Cmd cmd;

    cmd.append("cp", "../c2b.h", header_install_path + "/c2b.h");
    if (cmd.run() != 0) {
        std::cout << "Failed to install C2B header!" << std::endl;
        return 1;
    }
    cmd.set_length(0);
    cmd.append("g++", "c2b_binary.cpp", "-o", "c2b");
    if (cmd.run() != 0) {
        std::cout << "Failed to compile C2B binary!" << std::endl;
        return 1;
    }
    cmd.set_length(0);
    cmd.append("cp", "c2b", binary_install_path + "/c2b");
    if (cmd.run() != 0) {
        std::cout << "Failed to install C2B binary!" << std::endl;
        return 1;
    }

    std::cout << "C2B installed successfully!" << std::endl;

    return 0;
}
