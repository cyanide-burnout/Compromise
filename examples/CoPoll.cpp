#include "CoPoll.h"

CoPoll::CoPoll(struct FastPoll* poll) : poll(poll)
{

}

CoPoll::~CoPoll()
{
  RemoveAllEventHandlers(poll, invoke, this);
}

void CoPoll::add(int handle, uint64_t flags)
{
  AddEventHandler(poll, handle, flags, invoke, this);
}

void CoPoll::modify(int handle, uint64_t flags)
{
  ManageEventHandler(poll, handle, flags, invoke, this);
}

void CoPoll::remove(int handle)
{
  RemoveEventHandler(poll, handle);
}

void CoPoll::invoke(int handle, uint32_t flags, void* data, uint64_t options)
{
  static_cast<CoPoll*>(data)->wake({ handle, flags, options });
}
