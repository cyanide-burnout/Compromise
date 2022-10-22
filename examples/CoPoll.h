#ifndef COPOLL_H
#define COPOLL_H

#include "FastPoll.h"
#include "Compromise.h"

struct PollEvent
{
  int handle;
  uint32_t flags;
  uint64_t options;
};

class CoPoll : public Compromise::Emitter<PollEvent>
{
  public:

    CoPoll(struct FastPoll* poll);
    ~CoPoll();

    void add(int handle, uint64_t flags);
    void modify(int handle, uint64_t flags);
    void remove(int handle);

  private:

    struct FastPoll* poll;

    static void invoke(int handle, uint32_t flags, void* data, uint64_t options);
};

#endif
