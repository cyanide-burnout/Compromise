# Compromise

Simple C++ coroutine helper library, suitable to integrate with main loop.

Artem Prilutskiy, 2022

*Please note*, examples are incomplete but functional :)

## Fratures

* Can be integrated to main loop
* Allows call coroutines from coroutines asynchronously (asymmetric coroutines)
* Allows proceed coroutines status in callbacks (see Hook)
* Provides base interface to create asynchronous wrappers to be called from coroutines (Compromise::Emitter)
* *Not thread safe*

## Main components

### Compromise::Data / Compromise::Value / Compromise::Empty

Compromise::Data type for your own types to pass the data from (and to) coroutine using operator co_yield. Compromise::Value is simple smart pointer on it.

```C++
struct TestResult : Compromise::Data
{
  int number;
};

```

This approach allows to pass different data types between caller and callable in coroutine at the same time by using dynamic casting and type checking.
Compromise::Empty is an alias to Compromise::Data to pass empty data.

### Compromise::Task / Compromise::Future

Compromise::Future is main type that represents a coroutine, Compromise::Task is just an alias to simplify readability.

```C++
Compromise::Task TestYield()
{
  std::shared_ptr<TestResult> data = std::make_shared<TestResult>();

  co_yield Compromise::Empty();

  for (data->number = 0; data->number < 10; data->number ++)
    co_yield data;
}

```

There is no suspend on the start of coroutine. In case when you need to suspend coroutine immedietly after the start you can always use co_yield with Compromise::Empty().
Now is about co_return. To make coroutines more generic, there is no support of return values, but you can always use co_yield to pass result before exiting.

Compromise::Future is a future class implementation no manage coroutine.

* Can be created on the stack as well as on the heap by using copy constructor
* Controls live-cycle of coroutine
* Provides awaitable interface to the caller, which allows to call one coroutine from another one

#### Methods

* **done()** - coroutine done or not exists
* **value()** - get last value, passed by co_yield
* **resume()** - resumes coroutine
* **handle()** - get coroutine handle
* **rethrow()** - rethrows an unhandled exception (if any)
* **release()** - releases binding between coroutine and future
* **operator bool()** - coroutine exists and not done
* **operator ()()** - resumes coroutine and returns a value, passed by co_yield (synchronous call, value may be not set in case of incomplete execution)
* **operator co_await()** - resumes coroutine and returns a value, passed by co_yield (asynchronous call)

```C++
Compromise::Task TestInvokeFromCoroutine()
{
  auto routine = TestYield();

  while (routine)
  {
    auto data = std::dynamic_pointer_cast<TestResult>(co_await routine);
    if (data) printf("Result: %d\n", data->number);
  }
}

void TestInvokeFromFunction()
{
  auto routine = TestYield();

  while (routine)
  {
    auto data = std::dynamic_pointer_cast<TestResult>(routine());
    if (data) printf("Result: %d\n", data->number);
  }
}

Compromise::Task taskOnHeap = new Compromise::Task(TestYield());

```

#### Exception forwarding and handling

Any unhandled exception in coroutine code will be forwarded to the main code.

```C++
Compromise::Task TestException()
{
  throw std::exception();
}

auto routine = TestException();

try
{
  routine.rethrow();
}
catch (const std::exception& exception)
{
  printf("Unhandled exception: %s\n", exception.what());
}


```

#### Hook

Compromise::Future allows you to hook events of coroutine in callback manner. For example for cases, when you need to wait a result without pooling a coroutine or collect  garbage.

```C++
auto routine = new Compromise::Future(TestYield());
routine->hook = [] (Compromise::Future* future, Compromise::Status status) -> bool
{
  if (status == Compromise::Yield)
  {
    // This code will be called on co_yield
    auto data = std::dynamic_pointer_cast<TestResult>(future->value());
    if (data) printf("Test hook: %d\n", data->number);
    return true;  // Don't suspend a coroutine, the result is already handled
  }

  if (status == Compromise::Return)
  {
    // This code will be called after the end of coroutine's execution
    printf("Test hook done\n");
    future->release();  // Unbind coroutine from the future to prevent collision and fault on future's destruction
    delete future;      // Since we created future on the heap we have to delete it
    return true;        // Allow coroutine to destruct itself
  }

  return false;  // Suspend a coroutine (just an example)
};

// To avoid a loose of hook, coroutine used call co_yield Compromise::Empty() to suspend its execution immediately after creation
routine->resume();
```

### Compromise::Emitter

Compromise::Emitter is a wrapper to transform callback-style code into an awaitable, where Type is a type of object to return in co_await.

#### Methods:

* **wake(value)** - resumes coroutine
* **update(value&)** - optional virtual method, called by Emitter to initiate asynchronous call or update the value immediately

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
      return false;  // Update is incomplete, suspend coroutine and wait for call to wake()
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

