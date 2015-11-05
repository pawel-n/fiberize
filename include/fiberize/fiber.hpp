#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/context.hpp>
#include <fiberize/event.hpp>
#include <fiberize/system.hpp>

namespace fiberize {
    
template <typename A>
struct Fiber: public detail::FiberBase {
    typedef A result_type;
    
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    virtual void _execute() {
        auto controlBlock = detail::Executor::current()->currentControlBlock();
        self_ = detail::Executor::current()->system()->currentFiber();
        try {
            auto finished = Event<A>::fromPath(controlBlock->finishedEventPath);
            controlBlock->parent.emit(finished, run());
        } catch (...) {
            auto crashed = Event<Unit>::fromPath(controlBlock->crashedEventPath);
            controlBlock->parent.emit(crashed);
        }
    }
    
protected:
    /**
     * Yields control to the event loop.
     */
    void yield() const {
        return Context::current()->yield();
    }
    
    /**
     * Processes all pending events.
     */
    void process() const {
        return Context::current()->process();
    }
    
    /**
     * Returns the reference to the current fiber.
     */
    FiberRef self() const {
        return self_;
    }
    
private:
    FiberRef self_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
