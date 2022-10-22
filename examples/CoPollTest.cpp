#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/timerfd.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "CoPoll.h"
#include "CoResolver.h"
#include "CoCloudClient.h"

static Compromise::Task TestPoll(struct FastPoll* context)
{
  int handle;
  uint8_t count;
  uint64_t value;
  struct itimerspec interval;

  const char* const spinner[] =
  { "⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷" };

  interval.it_interval.tv_sec  = 0;
  interval.it_interval.tv_nsec = 250000000;
  interval.it_value.tv_sec     = interval.it_interval.tv_sec;
  interval.it_value.tv_nsec    = interval.it_interval.tv_nsec;

  handle = timerfd_create(CLOCK_MONOTONIC, 0);
  timerfd_settime(handle, 0, &interval, NULL);

  CoPoll poll(context);
  poll.add(handle, EVENT_READ);

  while (true)
  {
    auto event = co_await poll;
    if (event.flags & EVENT_READ)
    {
      read(event.handle, &value, sizeof(uint64_t));

      printf("\r[%s] ... ", spinner[count % 8]);
      fflush(stdout);

      count ++;
    }
  }

  close(handle);
}

static Compromise::Task TestClient(CloudClient* context)
{
  auto result = co_await CoCloud::Client(context, "https://ya.ru/robots.txt");

  if (result.code >= 0) printf("HTTP Code %d: %s\n", result.code, result.data);
}

static void PrintHostAddress(CoResolver::Event event)
{
  if (event.status == ARES_SUCCESS)
  {
    char buffer[INET6_ADDRSTRLEN + 1];
    inet_ntop(event.entry->h_addrtype, event.entry->h_addr_list[0], buffer, INET6_ADDRSTRLEN + 1);
    printf("Address: %s\n", buffer);
  }
}

static Compromise::Task TestResolver(struct ResolverState* context)
{
  CoResolver::Query query1(context, "127.0.0.1");
  CoResolver::Query query2(context, "rotate.aprs.net", AF_INET);

  PrintHostAddress(co_await query1);

  for (int count = 0; count < 10; count ++) PrintHostAddress(co_await query2);
}

struct TestResult : Compromise::Data
{
  int number;
};

static Compromise::Task TestYield()
{
  std::shared_ptr<TestResult> data = std::make_shared<TestResult>();

  for (data->number = 0; data->number < 10; data->number ++)  co_yield data;
}

static Compromise::Task TestInvoke()
{
  auto routine = TestYield();

  while (!routine.done())
  {
    auto data = std::dynamic_pointer_cast<TestResult>(co_await routine);
    if (data) printf("TestInvoke: %d\n", data->number);
  }
}

int main()
{
  int signal;
  sigset_t set;
  CloudClient* client;
  struct FastPoll* poll;
  struct ResolverState* resolver;

  poll     = CreateFastPoll(NULL, NULL);
  client   = CreateCloudClient(poll, 500);
  resolver = CreateResolver(poll);

  auto a = TestPoll(poll);
  auto b = TestClient(client);
  auto c = TestResolver(resolver);
  auto d = TestYield();
  auto e = TestInvoke();

  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigprocmask(SIG_BLOCK, &set, NULL);

  while ((WaitForEvents(poll, 500) > 0) ||
       (sigpending(&set) < 0) ||
       (sigismember(&set, SIGINT)  == 0) &&
       (sigismember(&set, SIGTERM) == 0))
  {
    if (!d.done())
    {
      auto result = std::dynamic_pointer_cast<TestResult>(d.value());
      printf("TestYield: %d\n", result->number);
      d.resume();
    }
  }

  ReleaseResolver(resolver);
  ReleaseCloudClient(client);
  ReleaseFastPoll(poll);

  return 0;
}