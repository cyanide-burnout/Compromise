#ifndef COCLOUDCLIENT_H
#define COCLOUDCLIENT_H

#include "CloudClient.h"
#include "Compromise.h"

#include <exception>

namespace CoCloud
{
  struct Event
  {
    int code;
    CURL* easy;
    char* data;
    size_t length;
  };

  struct Exception : public std::exception
  {
    const char* what() const noexcept;
  };

  class Client : public Compromise::Emitter<Event>
  {
    public:

      Client(CloudClient* client);
      Client(CloudClient* client, CURL* easy);
      Client(CloudClient* client, const char* location, struct curl_slist* headers = nullptr, const char* token = nullptr, const char* data = nullptr, size_t length = 0);

      bool query(CURL* easy);
      bool query(const char* location, struct curl_slist* headers = nullptr, const char* token = nullptr, const char* data = nullptr, size_t length = 0);

    private:

      CloudClient* client;

      static void invoke(int code, CURL* easy, char* data, size_t length, void* parameter1, void* parameter2);
  };
};

#endif
