#ifndef COMPROMISE_H
#define COMPROMISE_H

// Simple C++ coroutine helper library
// https://github.com/cyanide-burnout/Compromise
// Artem Prilutskiy, 2022

#include <memory>
#include <utility>
#include <coroutine>
#include <exception>
#include <functional>

namespace Compromise
{
  struct Promise;
  class Future;

  enum Status
  {
    Idle,
    Await,
    Yield,
    Return
  };

  struct Data
  {
    virtual ~Data() = default;
  };

  typedef std::function<bool (Future*, Status)> Hook;
  typedef std::coroutine_handle<Promise> Handle;
  typedef std::shared_ptr<Data> Value;
  typedef Value Empty;

  using Task = Future;

  // In this implementation it is allowed to pass value back to caller by co_yield only

  struct Suspender
  {
    bool await_ready();
    void await_suspend(Handle) noexcept;
    void await_resume() const noexcept;

    Future* future;
    Handle entry;
    int status;
  };

  template<class Actor, typename Type> struct Awaiter
  {
    constexpr bool await_ready() const  { return false;              };
    bool await_suspend(Handle handle)   { return actor.wait(handle); };
    Type await_resume()                 { return actor.value();      };

    Actor& actor;
  };

  struct Promise
  {
    Promise();

    Future get_return_object();
    std::suspend_never initial_suspend() noexcept;
    Suspender final_suspend() noexcept;
    Suspender yield_value(Value value);
    void unhandled_exception();
    void return_void();

    Value data;
    Handle entry;
    Status status;
    Future* future;
    std::exception_ptr exception;
  };

  class Future
  {
    public:

      using promise_type = Promise;

      explicit Future(Promise* promise);
      Future(Handle&& handle);
      Future(Future&& future);
      ~Future();

      bool done();
      void resume();
      void rethrow();
      void release();

      Value& value();
      Handle& handle();

      bool wait(Handle& handle);

      operator bool();
      Value& operator ()();
      Awaiter<Future, Value&> operator co_await() noexcept;

      Hook hook;

    private:

      Handle routine;
  };

  template<class Type> class Emitter
  {
    public:

      const Type& value()           { return data;                                                                         };
      bool wait(Handle& handle)     { routine = std::move(handle); routine.promise().status = Await; return !update(data); };
      void wake(const Type& event)  { if (routine) { data = std::move(event); std::exchange(routine, nullptr)(); }         };

      Awaiter<Emitter, const Type&> operator co_await() noexcept  {  return { *this }; };

    private:

      Handle routine;
      Type data;

    protected:

      virtual bool update(Type&)
      {
        // In case of synchronous processing, update() can update the data and return true to avoid coroutine from suspension
        // Otherwise update() has to return false and following callback must call wake() to resume coroutine
        return false;
      };
  };
};

#endif
