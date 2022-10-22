#ifndef COMPROMISE_H
#define COMPROMISE_H

/*
 Simple C++ coroutine helper library

 https://github.com/cyanide-burnout/Compromise

 Artem Prilutskiy, 2022
*/

#include <memory>
#include <utility>
#include <coroutine>

namespace Compromise
{
  struct Promise;
  class Future;

  enum Status
  {
    Resume,
    Suspend
  };

  struct Data
  {
    virtual ~Data() = default;
  };

  typedef Data Empty;
  typedef std::shared_ptr<Data> Value;
  typedef std::coroutine_handle<Promise> Handle;

  using Task = Future;

  // In this implementation it is allowed only to pass value back to caller only thru co_yield.

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

      Awaiter<Future, Value&> operator co_await() noexcept;

    private:

      Handle routine;
  };

  template<class Type> class Emitter
  {
    public:

      Emitter()                          = default;
      Emitter(Emitter&&)                 = delete;
      Emitter(const Emitter&)            = delete;
      Emitter& operator=(Emitter&&)      = delete;
      Emitter& operator=(const Emitter&) = delete;

      const Type& value()  { return data; };

      bool wait(Handle& handle)
      {
        routine = std::move(handle);
        return !update(data);
      };

      void wake(const Type& event)
      {
        if (routine)
        {
          data = std::move(event);
          std::exchange(routine, nullptr).resume();
        }
      };

      Awaiter<Emitter, const Type&> operator co_await() noexcept  {  return { *this }; };

    private:

      Handle routine;
      Type data;

    protected:

      // In case of synchronous processing, update() can update the data and return true to avoid coroutine from suspension.
      // Otherwise update() has to return false and following callback must call wake() to resume coroutine.

      virtual bool update(Type&)
      {
        return false;
      };
  };
};

#endif
