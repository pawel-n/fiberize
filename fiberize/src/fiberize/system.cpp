#include <fiberize/system.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/context.hpp>

#include <thread>
#include <chrono>

#include <boost/uuid/uuid_generators.hpp>

namespace fiberize {

System::System() : System(std::thread::hardware_concurrency()) {}

System::System(uint32_t macrothreads) {
    shuttingDown = false;
    running = 0;
    roundRobinCounter = 0;

    /**
     * Generate the uuid.
     *
     * If valgrind support is enabled we cannot use std::random_device, because valgrind 3.11.0
     * doesn't recognize the rdrand instruction used in the implementation of random_device.
     */
#ifdef FIBERIZE_VALGRIND
    std::default_random_engine seedGenerator(std::chrono::system_clock::now().time_since_epoch().count());
#else
    std::random_device seedGenerator;
#endif
    std::uniform_int_distribution<uint64_t> seedDist;
    boost::random::mt19937 pseudorandom(seedDist(seedGenerator));
    boost::uuids::random_generator uuidGenerator(pseudorandom);
    uuid_ = uuidGenerator();

    allFibersFinished_ = newEvent<Unit>();
    mainControlBlock = createUnmanagedBlock();
    mainControlBlock->grab();
    mainContext_ = new Context(mainControlBlock, this);

    // Spawn the executors.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        executors.emplace_back(new detail::Executor(this, seedDist(seedGenerator), i));
    }

    for (uint32_t i = 0; i < macrothreads; ++i) {
        executors[i]->start();
    }
}

System::~System() {
    for (detail::Executor* executor : executors) {
        executor->stop();
    }
    for (detail::Executor* executor : executors) {
        delete executor;
    }
    delete mainContext_;
    mainControlBlock->drop();
}

FiberRef System::mainFiber() {
    return FiberRef(std::make_shared<detail::LocalFiberRef>(this, mainControlBlock));
}

Context* System::mainContext() {
    return mainContext_;
}

void System::shutdown() {
    shuttingDown = true;
}

Event<Unit> System::allFibersFinished() {
    return allFibersFinished_;
}

void System::schedule(detail::ControlBlock* controlBlock) {
    // TODO: optimize memory order
    uint64_t i = std::atomic_fetch_add(&roundRobinCounter, 1lu);
    executors[i % executors.size()]->schedule(controlBlock);
}

void System::fiberFinished() {
    if (std::atomic_fetch_sub_explicit(&running, 1lu, std::memory_order_release) == 1) {
         std::atomic_thread_fence(std::memory_order_acquire);
         // TODO: subscription
         mainFiber().emit(allFibersFinished_);
    }
}

boost::uuids::uuid System::uuid() const {
    return uuid_;
}
    
} // namespace fiberize
