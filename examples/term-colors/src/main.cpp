#include <c2b.h>
#include <iostream>

int main() {
    c2b::Logger logger = c2b::Loggers::stdout;

    logger.log_info(c2b::TermColors::RED + "This is red text" + c2b::TermColors::RESET);
    logger.log_info(c2b::TermColors::YELLOW + "This is yellow text" + c2b::TermColors::RESET);

    return 0;
}
