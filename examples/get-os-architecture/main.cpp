#include <iostream>

#include "../../qbs.h"



int main() {
    auto os = qbs::Utils::get_os();
    auto arch = qbs::Utils::get_arch();

    std::cout << "OS: ";

    switch (os) {
        case qbs::OS::WIN:
            std::cout << "Windows" << std::endl;
            break;
        case qbs::OS::LINUX:
            std::cout << "Linux" << std::endl;
            break;
        case qbs::OS::MAC:
            std::cout << "Mac" << std::endl;
            break;
        case qbs::OS::UNKNOWN:
            std::cout << "Unknown. Please submit an issue with details about your device" << std::endl;
            break;
        default:
            assert(0 && "How did you get here?");
    }

    std::cout << "Architecture: ";

    switch (arch) {
        case qbs::Arch::ARM32:
            std::cout << "ARM 32 bit" << std::endl;
            break;
        case qbs::Arch::ARM64:
            std::cout << "ARM64" << std::endl;
            break;
        case qbs::Arch::X64:
            std::cout << "x64" << std::endl;
            break;
        case qbs::Arch::X86:
            std::cout << "x86" << std::endl;
            break;
        case qbs::Arch::UKNOWN:
            std::cout << "Unknown. Please submit an issue with details about your device" << std::endl;
            break;
        default:
            assert(0 && "How did you get here?");
    }

    return 0;
}
