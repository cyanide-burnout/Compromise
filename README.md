# Compromise
Simple C++ coroutine helper library

Artem Prilutskiy, 2022

*Please note*, examples are incomplete fut functional.

## Main components

### Compromise::Task

That is main type that represents a coroutine. It derives std::coroutine_handle<Promise> and not a Future.
I prefered to not generate an instance of Future to make more flexibilities with managing future as well as readiness of core.
You have to create an instance of Future separately.

```C++
Compromise::Task TestYield()
{
  std::shared_ptr<TestResult> data = std::make_shared<TestResult>();

  for (data->number = 0; data->number < 10; data->number ++)
    co_yield data;
}

```

There is no suspend on the start of coroutine. In case when you need to suspend coroutine immedietly after the start you can always use co_yield with empty Compromise::Data().
Now is about co_return. To make coroutines more generic, there is no support of return values, but you can always use co_yield to pass result before exiting.


## Compromise::Data and Compromise::Value

Compromise::Data type for your own types to pass the data from (and to) coroutine using operator co_yield. Compromise::Value is simple smart pointer on it.

```C++
struct TestResult : Compromise::Data
{
  int number;
};

```

This approach allows to pass different data types between caller and callable in coroutine at the same time by using dynamic casting and type checking.

## Compromise::Future

Compromise::Future is a future class implementation no manage coroutine.

* Can be created on the stack as well as on the heap
* Controls live-cycle of Compromise::Task
* Provides awaitable interface to the caller, which allows to call one coroutine from another one

```C++
Compromise::Task TestInvokeFromCoroutine()
{
  Compromise::Future routine(TestYield());

  while (!routine.done())
  {
    auto data = std::dynamic_pointer_cast<TestResult>(co_await routine);
    if (data) printf("Result: %d\n", data->number);
  }
}

void TestInvokeFromFunction()
{
  Compromise::Future routine(TestYield());

  while (!routine.done())
  {
    auto data = std::dynamic_pointer_cast<TestResult>(routine.value());
    printf("Result: %d\n", data->number);
    routine.resume();
  }
}

```

## Compromise::Emitter

Compromise::Emitter is a wrapper to transform callback-style code into an awaitable, where Type is a type of object to return in co_yield.

```C++
Compromise::Task TestClient(CloudClient* context)
{
  auto result = co_await CoCloud::Client(context, "https://ya.ru/robots.txt");

  if (result.code >= 0) printf("HTTP Code %d: %s\n", result.code, result.data);
}

Compromise::Task TestResolver(struct ResolverState* context)
{
  CoResolver::Query query1(context, "127.0.0.1");
  CoResolver::Query query2(context, "rotate.aprs.net", AF_INET);

  PrintHostAddress(co_await query1);

  for (int count = 0; count < 10; count ++) PrintHostAddress(co_await query2);
}
```

Here is an example of wrapper code:

```C++
struct ResolverEvent
{
  int status;
  int timeouts;
  struct hostent* entry;
};

class Query : public Compromise::Emitter<ResolverEvent>
{
  public:

    Query(struct ResolverState* state, const char* name, int family = AF_UNSPEC) : state(state), name(name), family(family)
    {
      ClearHostEntryIterator(&iterator);
    }

  private:

    int family;
    const char* name;
    struct ResolverState* state;
    struct HostEntryIterator iterator;

    bool update(Event& data)
    {
      ResolveHostAddress(state, name, family, invoke, this, &iterator);
      return false;
    }

    static void invoke(void* argument, int status, int timeouts, struct hostent* entry)
    {
      CoResolver::Query* self = static_cast<CoResolver::Query*>(argument);
      ValidateHostEntry(&status, entry, &self->iterator);
      self->wake({ status, timeouts, entry });
    }
};


struct PollEvent
{
  int handle;
  uint32_t flags;
  uint64_t options;
};

class CoPoll : public Compromise::Emitter<PollEvent>
{
  public:

    CoPoll(struct FastPoll* poll) : poll(poll) {  }
    ~CoPoll()                                  { RemoveAllEventHandlers(poll, invoke, this);            }

    void add(int handle, uint64_t flags)       { AddEventHandler(poll, handle, flags, invoke, this);    }
    void modify(int handle, uint64_t flags)    { ManageEventHandler(poll, handle, flags, invoke, this); }
    void remove(int handle)                    { RemoveEventHandler(poll, handle);                      }

  private:

    struct FastPoll* poll;

    static void invoke(int handle, uint32_t flags, void* data, uint64_t options)
    {
      static_cast<CoPoll*>(data)->wake({ handle, flags, options });
    }
};

```

