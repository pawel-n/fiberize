/**
 * IO modes.
 *
 * @see @ref io_modes
 *
 * @file mode.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_MODE_HPP
#define FIBERIZE_IO_MODE_HPP

#include <memory>

#include <fiberize/promise.hpp>

namespace fiberize {
namespace io {

/**
 * @page io_modes IO modes
 *
 * Controls how to execute IO operations.
 *
 * IO modes control whether an IO operation should be blocking, asynchronous or nonblocking.
 * There are three IO modes:
 *   - Await - Executing an IO operation in await mode will block the fiber until the operation is done,
 *             while processing messages and allowing other fibers to execute. This mode is usually the default.
 *   - Block - Exeuting an IO operation in block mode will block the fiber and the thread it is executing on.
 *             This version does not process messages and does not allow other fibers to execute on this core.
 *   - Async - Executing an IO operation in defer mode won't block the fiber and won't process messages.
 *             Instead it starts the IO operation asynchronously and reports the result with a promise.
 *
 * IO operations are implemented as template functions, which take the mode as a template parameter.
 * For example we can open a file in three different ways:
 * * Blocking:
 *   @code
 *     File file = File::open<Block>("test", O_RDONLY, 0777);
 *   @endcode
 * * Nonblocking, but synchronous:
 *   @code
 *     File file = File::open<Await>("test", O_RDONLY, 0777);
 *   @endcode
 * * Asynchronous, which gives us a promise:
 *   @code
 *     std::shared_ptr<Promise<File>> promise = File::open<Async>("test", O_RDONLY, 0777);
 *     File file = promise->await();
 *   @endcode
 *
 * Some functions provide a default mode. For example if you call
 * @code
 *   File file = File::open("test", O_RDONLY, 0777);
 * @endcode
 * it will use the Block mode.
 */

/**
 * Executing an IO operation in await mode will block the fiber until the operation is done,
 * while processing messages and allowing other fibers to execute.
 *
 * This mode is usually the default.
 */
class Await {};

/**
 * Exeuting an IO operation in block mode will block the fiber and the thread it is executing on.
 * This version does not process messages and does not allow other fibers to execute on this core.
 *
 * This mode should be used for inexpensive and predictable IO operations, especially filesystem
 * operations. Asynchronous FS operations are currently implemented with a worker pool. The cost
 * of sending the job to a worker and thread synchronization can outweight the benefit.
 */
class Block {};

/**
 * Executing an IO operation in defer mode won't block the fiber and won't process messages.
 * Instead it starts the IO operation asynchronously and reports the result with a promise.
 */
class Async {};

namespace detail {

template <typename Value, typename Mode>
struct ResultImpl {
};

template <typename Value>
struct ResultImpl<Value, Await> {
    typedef Value Type;
};

template <typename Value>
struct ResultImpl<Value, Block> {
    typedef Value Type;
};

template <typename Value>
struct ResultImpl<Value, Async> {
    typedef std::shared_ptr<Promise<Value>> Type;
};

} // namespace detail

/**
 * A helper used to choose the right result type based on IO mode. Await and Block modes
 * return the value, while Async returns a pointer to a promise.
 */
template <typename Value, typename Mode>
using Result = typename detail::ResultImpl<Value, Mode>::Type;

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_MODE_HPP

