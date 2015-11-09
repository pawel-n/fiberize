#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct Printer : public Fiber<Unit> {
    Printer(int n): n(n) {}
    
    virtual Unit run() {
        std::cout << "Hello from fiber #" << n << std::endl;
        return {};
    }

    int n;
};

int main() {
    System system;
    
    for (int i = 0; i < 1000000; ++i) {
        system.run<Printer>(i);
    }

    system.allFibersFinished().await(system.mainContext());
    return 0;
}
