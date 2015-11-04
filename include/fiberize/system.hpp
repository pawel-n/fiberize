#ifndef FIBERIZE_SYSTEM_HPP
#define FIBERIZE_SYSTEM_HPP

#include <boost/context/all.hpp>

#include <fiberize/fiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/deadletterfiberref.hpp>

namespace fiberize {

class System {
public:
    /**
     * Starts the system with a number of macrothreads based on the number of cores.
     */
    System();
    
    /**
     * Starts the system with the given number of macrothreads.
     */
    System(uint32_t macrothreads);
    
    /**
     * Starts a fiber.
     */
    template <typename FiberImpl, typename MailboxImpl = LockfreeQueueMailbox, typename ...Args>
    FiberRef run(Args&& ...args) {
        std::shared_ptr<detail::FiberRefImpl> impl;
        if (!shuttingDown) {
            FiberImpl* fiber = new FiberImpl(std::forward<Args>(args)...);
            
            // Create the control block.
            detail::ControlBlock* block = new detail::ControlBlock;
            block->stack = stackAllocator.allocate();
            block->context = boost::context::make_fcontext(block->stack.sp, block->stack.size, fiberRunner);
            block->mailbox = new MailboxImpl();
            block->fiber = fiber;
            block->finished = false;
            
            // Send it to a random executor.
            std::uniform_int_distribution<uint32_t> chooseExecutor(0, executors.size() - 1);
            executors[chooseExecutor(System::random)]->execute(block);
            
            // Create a local reference.
            impl = std::make_shared<detail::LocalFiberRef>(block->mailbox);
        } else {
            // System is shutting down, do not create new fibers.
            impl = std::make_shared<detail::DeadLetterFiberRef>();
        }
        return FiberRef(impl);
    }
    
    /**
     * Shut down the system.
     */
    void shutdown() {
        shuttingDown = true;
    };
    
    /**
     * Thread local random.
     */
    static thread_local std::default_random_engine random;
    
private:
    /**
     * Trampoline used to start a fiber.
     */
    static void fiberRunner(intptr_t);

    /**
     * Stack allocator.
     */
    boost::context::fixedsize_stack stackAllocator;
    
    /**
     * Currently running executors.
     */
    std::vector<std::unique_ptr<detail::Executor>> executors;
    
    /**
     * Whether the system is shutting down.
     */
    std::atomic<bool> shuttingDown;
    
};
    
} // namespace fiberize

#endif // FIBERIZE_SYSTEM_HPP
