#include "../../c2b.h"
#include <fstream>


int main(int argc, char** argv) {
    c2b::Build::rebuild_self(argc, argv, __FILE__);

    std::ofstream log("log.txt");

    c2b::Logger logger(log);

    std::cout << "Hello, World!" << std::endl;
    logger.log_info("Outputted 'Hello, World!'");

    log.close();

    return 0;
}
