#ifndef CORESOLVER_H
#define CORESOLVER_H

#include <netdb.h>

#include "Resolver.h"
#include "Compromise.h"

namespace CoResolver
{
  struct Event
  {
    int status;
    int timeouts;
    struct hostent* entry;
  };

  class Query : public Compromise::Emitter<Event>
  {
    public:

      Query(struct ResolverState* state, const char* name, int family = AF_UNSPEC);

    private:

      int family;
      const char* name;
      struct ResolverState* state;
      struct HostEntryIterator iterator;

      bool update(Event& data);

      static void invoke(void* argument, int status, int timeouts, struct hostent* entry);
  };
};

#endif
