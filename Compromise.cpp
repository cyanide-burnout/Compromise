#include "Compromise.h"

#include <utility>
#include <type_traits>

using namespace Compromise;

Promise::Promise() : status(Resume)
{

};

Future Promise::get_return_object()
{
  return Future(this);
};

std::suspend_never Promise::initial_suspend() noexcept
{
  return { };
};

std::suspend_always Promise::final_suspend() noexcept
{
  return { };
};

std::suspend_always Promise::yield_value(Value value)
{
  data = std::move(value);
  return { };
};

void Promise::unhandled_exception()
{
  std::terminate();
};

void Promise::return_void()
{
  data.reset();
};

Future::Future(Promise* promise)
{
  routine = Handle::from_promise(*promise);
};

Future::Future(Handle&& handle)
{
  std::swap(routine, handle);
};

Future::Future(Future&& future)
{
  std::swap(routine, future.routine);
}

Future::~Future()
{
  if (routine) routine.destroy();
};

bool Future::done()
{
  return !routine || routine.done();
};

void Future::resume()
{
  routine.promise().status = Resume;
  routine.resume();
};

Value& Future::value()
{
  return routine.promise().data;
};

Handle& Future::handle()
{
  return routine;
};

bool Future::wait(Handle&)
{
  if (std::exchange(routine.promise().status, Suspend)) routine();
  return false;
};

Awaiter<Future, Value&> Future::operator co_await() noexcept
{
  return { *this };
};
