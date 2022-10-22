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
  struct Data;

  enum Status  { Resume, Suspend };

  typedef std::shared_ptr<Data> Value;
  typedef std::coroutine_handle<Promise> Handle;

  struct Data           { virtual ~Data()    = default; };
  struct Task : Handle  { using promise_type = Promise; };

  // In this implementation it is allowed only to pass value back to caller only thru co_yield.

  struct Promise
  {
    Promise()                                          { state = Resume;                         };
    Status status(Status value)                        { return std::exchange(state, value);     };

    Handle get_return_object()                         { return { Handle::from_promise(*this) }; };
    std::suspend_never initial_suspend()     noexcept  { return {  };                            };
    std::suspend_always final_suspend()      noexcept  { return {  };                            };
    std::suspend_always yield_value(Value value)       { data = std::move(value);  return {  };  };
    void unhandled_exception()                         { std::terminate();                       };
    void return_void()                                 { data.reset();                           };

    Status state;
    Value data;
  };

  template<class Actor, typename Type> struct Awaiter
  {
    Awaiter(Actor& actor)              noexcept  : actor(actor) {             };

    constexpr bool await_ready() const noexcept  { return false;              };
    bool await_suspend(Handle handle)  noexcept  { return actor.wait(handle); };
    Type await_resume()                noexcept  { return actor.value();      };

    Actor& actor;
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

      bool wait(Handle& handle)     { routine = std::move(handle);  return !update(data);                                                   };
      void wake(const Type& event)  { Handle handle;  if (routine) { data = std::move(event);  std::exchange(handle, routine);  handle(); } };

      Awaiter<Emitter, const Type&> operator co_await() noexcept  { return Awaiter<Emitter, const Type&>(*this); };

    private:

      Handle routine;
      Type data;

    protected:

      // In case of synchronous processing, update() can update the data and return true to avoid coroutine from suspension.
      // Otherwise update() has to return false and following callback must call wake() to resume coroutine.

      virtual bool update(Type&) { return false; };
  };

  class Future
  {
    public:

      explicit Future(Promise* promise)  { routine = Handle::from_promise(*promise); };
      Future(Handle&& handle)            { std::exchange(routine, handle);           };
      ~Future()                          { if (routine) routine.destroy();           };

      Future(Future&&)                 = delete;
      Future(const Future&)            = delete;
      Future& operator=(Future&&)      = delete;
      Future& operator=(const Future&) = delete;

      bool done()       { return !routine || routine.done();                                    };
      void resume()     { if (routine) { routine.promise().status(Resume);  routine.resume(); } };
      Value& value()    { return routine ? routine.promise().data : dummy;                      };
      Handle& handle()  { return routine;                                                       };

      bool wait(Handle& handle)  { if (routine && routine.promise().status(Suspend)) routine.resume(); return false; };

      Awaiter<Future, Value&> operator co_await() noexcept  { return Awaiter<Future, Value&>(*this); };

    private:

      Handle routine;
      Value dummy;
  };
};

#endif
