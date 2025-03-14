#include <iostream>
#include "hello.h"


void helloWorld(int argc, char **argv) {
    std::string word = "World";
    if (argc > 1) {
        word = argv[1];
    }

    std::cout << "Hello, " << word << "!" << std::endl;
}
