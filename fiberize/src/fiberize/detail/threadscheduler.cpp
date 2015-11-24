#include <fiberize/detail/threadscheduler.hpp>
#include <fiberize/detail/fiberscheduler.hpp>
#include <fiberize/fibersystem.hpp>

#include <thread>

namespace fiberize {
namespace detail {

ThreadScheduler::ThreadScheduler(FiberSystem* system, uint64_t seed, ThreadControlBlock* controlBlock)
    : Scheduler(system, seed), controlBlock_(controlBlock)
    {}

ThreadScheduler::~ThreadScheduler() {
    if (controlBlock_ != nullptr)
        controlBlock_->drop();
}

void ThreadScheduler::enableFiber(FiberControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock) {
    assert(controlBlock->status == Suspended);
    const auto& schedulers = system()->schedulers();
    std::uniform_int_distribution<std::size_t> dist(0, schedulers.size() - 1);
    std::size_t i = dist(random());
    schedulers[i]->enableFiber(controlBlock, std::move(lock));
}

void ThreadScheduler::suspend(boost::unique_lock<ControlBlockMutex>&& lock) {
    controlBlock_->status = Suspended;

    /**
     * Exchange the control block lock to the condition variable lock.
     */
    boost::unique_lock<boost::mutex> enabledLock(controlBlock_->enabledMutex);
    lock.unlock();

    /**
     * Wait until someone enables us.
     */
    controlBlock_->enabled.wait(enabledLock);

    /**
     * Change the status to running without locking.
     */
    assert(controlBlock_->status == Scheduled);
    controlBlock_->status = Running;
}

void ThreadScheduler::yield(boost::unique_lock<ControlBlockMutex>&& lock) {
    lock.unlock();
    std::this_thread::yield();
}

void ThreadScheduler::terminate() {
    controlBlock_->status = Dead;
    controlBlock_->drop();
    controlBlock_ = nullptr;
    int retval = 0;

    // NOTE: AFAIK there is no equivalent to this function in boost::thread or std.
    //       This is not an issue for now, because we only target linux.
    pthread_exit(&retval);
}

bool ThreadScheduler::tryToStealTask(FiberControlBlock*&) {
    return false;
}

ControlBlock* ThreadScheduler::currentControlBlock() {
    return controlBlock_;
}

} // namespace detail
} // namespace fiberize