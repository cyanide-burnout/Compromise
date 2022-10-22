#include "CoResolver.h"

CoResolver::Query::Query(struct ResolverState* state, const char* name, int family) : state(state), name(name), family(family)
{
  ClearHostEntryIterator(&iterator);
}

bool CoResolver::Query::update(CoResolver::Event& data)
{
  ResolveHostAddress(state, name, family, invoke, this, &iterator);
  return false;
}

void CoResolver::Query::invoke(void* argument, int status, int timeouts, struct hostent* entry)
{
  CoResolver::Query* self = static_cast<CoResolver::Query*>(argument);
  ValidateHostEntry(&status, entry, &self->iterator);
  self->wake({ status, timeouts, entry });
}
