#include "Compromise.h"

using namespace Compromise;

bool Suspender::await_ready()
{
  return
     future && future->hook && future->hook(future, (Status)status) ||
    !future && status;
}

void Suspender::await_suspend(Handle) noexcept
{
  if (entry)
  {
    // Resume coroutine instead of returning coroutine_handle<> to prevent segmentation fault in case of null pointer
    entry.resume();
  }
}

void Suspender::await_resume() const noexcept
{

}

Promise::Promise() : status(Idle), future(nullptr), exception(nullptr)
{

}

Future Promise::get_return_object()
{
  return Future(this);
}

std::suspend_never Promise::initial_suspend() noexcept
{
  return { };
}

Suspender Promise::final_suspend() noexcept
{
  status = Return;
  return { future, nullptr, status };
}

Suspender Promise::yield_value(Value value)
{
  data = std::move(value);

  if (entry)
  {
    status = Idle;
    return { nullptr, std::exchange(entry, nullptr), false };
  }

  status = Yield;
  return { future, nullptr, status };
}

void Promise::unhandled_exception()
{
  exception = std::current_exception();
}

void Promise::return_void()
{
  data.reset();
}

Future::Future(Promise* promise)
{
  routine = Handle::from_promise(*promise);
  promise->future = this;
}

Future::Future(Handle&& handle)
{
  std::swap(routine, handle);
  routine.promise().future = this;
}

Future::Future(Future&& future)
{
  std::swap(routine, future.routine);
  routine.promise().future = this;
}

Future::~Future()
{
  if (routine)
  {
    // Handle has to be destroyed to prevent a leakage
    routine.destroy();
  }
}

bool Future::done()
{
  return !routine || routine.done();
}

void Future::resume()
{
  routine.promise().status = Idle;
  routine.promise().data.reset();
  routine.resume();
}

void Future::rethrow()
{
  if (routine && routine.promise().exception)
  {
    // There is no need to return a status due to a throw
    std::rethrow_exception(std::exchange(routine.promise().exception, nullptr));
  }
}

void Future::release()
{
  if (routine)
  {
    routine.promise().future = nullptr;
    routine                  = nullptr;
  }
}

Value& Future::value() &
{
  return routine.promise().data;
}

Handle& Future::handle() &
{
  return routine;
}

bool Future::wait(Handle& handle)
{
  while (routine.promise().status != Return)
  {
    switch (routine.promise().status)
    {
      case Idle:
        routine.promise().data.reset();
        routine.resume();
        continue;

      case Await:
        routine.promise().entry = handle;
        return true;

      case Yield:
        routine.promise().status = Idle;
        return false;
    }
  }

  return false;
};

Future::operator bool()
{
  return routine && !routine.done();
}

Value& Future::operator ()()
{
  if (std::exchange(routine.promise().status, Idle) == Idle)
  {
    routine.promise().data.reset();
    routine.resume();
  }

  return routine.promise().data;
};

Awaiter<Future, Value&> Future::operator co_await() noexcept
{
  return { *this };
};
