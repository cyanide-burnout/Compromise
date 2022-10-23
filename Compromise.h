#ifndef COMPROMISE_H
#define COMPROMISE_H

// Simple C++ coroutine helper library
// https://github.com/cyanide-burnout/Compromise
// Artem Prilutskiy, 2022

#include <memory>
#include <utility>
#include <coroutine>

namespace Compromise
{
  struct Promise;
  class Future;

  enum Status
  {
    Probable,
    Incomplete
  };

  struct Data
  {
    virtual ~Data() = default;
  };

  typedef std::coroutine_handle<Promise> Handle;
  typedef std::shared_ptr<Data> Value;
  typedef Value Empty;

  using Task = Future;

  // In this implementation it is allowed to pass value back to caller by co_yield only.

  struct Promise
  {
    Promise();

    Future get_return_object();
    std::suspend_never initial_suspend() noexcept;
    std::suspend_always final_suspend() noexcept;
    std::suspend_always yield_value(Value value);
    void unhandled_exception();
    void return_void();

    Value data;
    Status status;
  };

  template<class Actor, typename Type> struct Awaiter
  {
    constexpr bool await_ready() const noexcept  { return false;              };
    bool await_suspend(Handle handle)  noexcept  { return actor.wait(handle); };
    Type await_resume()                noexcept  { return actor.value();      };

    Actor& actor;
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
      Value& value();
      Handle& handle();

      bool wait(Handle&);

      operator bool();
      Value& operator ()();
      Awaiter<Future, Value&> operator co_await() noexcept;

    private:

      Handle routine;
  };

  template<class Type> class Emitter
  {
    public:

      const Type& value()           { return data;                                                                 };
      bool wait(Handle& handle)     { routine = std::move(handle); return !update(data);                           };
      void wake(const Type& event)  { if (routine) { data = std::move(event); std::exchange(routine, nullptr)(); } };

      Awaiter<Emitter, const Type&> operator co_await() noexcept  {  return { *this }; };

    private:

      Handle routine;
      Type data;

    protected:

      virtual bool update(Type&)
      {
        // In case of synchronous processing, update() can update the data and return true to avoid coroutine from suspension.
        // Otherwise update() has to return false and following callback must call wake() to resume coroutine.
        return false;
      };
  };
};

#endif
