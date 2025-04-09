#include <iostream>

#include "../../c2b.h"



int main() {
    auto os = c2b::Utils::get_os();
    auto arch = c2b::Utils::get_arch();

    std::cout << "OS: ";

    switch (os) {
        case c2b::OS::WIN:
            std::cout << "Windows" << std::endl;
            break;
        case c2b::OS::LINUX:
            std::cout << "Linux" << std::endl;
            break;
        case c2b::OS::MAC:
            std::cout << "Mac" << std::endl;
            break;
        case c2b::OS::UNKNOWN:
            std::cout << "Unknown. Please submit an issue with details about your device" << std::endl;
            break;
        default:
            assert(0 && "How did you get here?");
    }

    std::cout << "Architecture: ";

    switch (arch) {
        case c2b::Arch::ARM32:
            std::cout << "ARM 32 bit" << std::endl;
            break;
        case c2b::Arch::ARM64:
            std::cout << "ARM64" << std::endl;
            break;
        case c2b::Arch::X64:
            std::cout << "x64" << std::endl;
            break;
        case c2b::Arch::X86:
            std::cout << "x86" << std::endl;
            break;
        case c2b::Arch::UKNOWN:
            std::cout << "Unknown. Please submit an issue with details about your device" << std::endl;
            break;
        default:
            assert(0 && "How did you get here?");
    }

    return 0;
}
