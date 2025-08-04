#include "ak.hpp"

DefineTask MainTask() noexcept {
    std::print("Hello World!\n");
    co_return;
}

int main() {
    KernelConfig config = {
        .mem = nullptr,
        .memSize = 0,
        .ioEntryCount = 256
    };
    return RunMain(&config, MainTask);
}