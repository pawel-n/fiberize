#include <fiberize/fiberize.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace fiberize;

Event<FiberRef> init("init");
Event<Unit> ping("ping");
Event<Unit> pong("pong");

struct Ping : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();
        
        while (true) {
            std::cout << "Ping" << std::endl;
            peer.emit(ping);
            pong.await();
        }
    }
};

struct Pong : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();

        while (true) {
            ping.await();
            std::cout << "Pong" << std::endl;
            peer.emit(pong);
        }
    }
};

int main() {
    using namespace std::literals;
    System system;
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>();
    
    ping.emit(init, pong);
    pong.emit(init, ping);
    
    Context::current()->yield();
}
