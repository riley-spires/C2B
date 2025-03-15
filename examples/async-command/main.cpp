#include "../../qbs.h"

#include <format>

int main() {
    // Easily adjust how long the async cmd will run for
    const int SLEEP_TIME = 5;

    // Create cmd that runs multiple commands at once
    // NOTE: This is not the recommended way to use Cmd, but is used this way for demo purposes
    //       Proper use would be multiple Cmd objects, or reset the current one after running
    //       with cmd.set_length(0)
    qbs::Cmd cmd(std::format("sleep {};", SLEEP_TIME), "echo 'sleep done!'");

    // Run the cmd asynchronously
    std::future<int> cmdResult = cmd.run_async();

    // Do other things while cmd is running
    for (int i = 0; i < SLEEP_TIME * SLEEP_TIME * SLEEP_TIME; ++i) {
        std::cout << i << std::endl;
    }


    // Wait for the command to finish running and get it's result
    return cmdResult.get();
}
