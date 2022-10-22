#include "CoCloudClient.h"

#define ThrowIfFailed(condition)  if (!(condition)) throw CoCloud::Exception();

const char* CoCloud::Exception::what() const noexcept
{
  return "Error calling CoCloud::Client()";
};

CoCloud::Client::Client(CloudClient* client) : client(client)
{

}

CoCloud::Client::Client(CloudClient* client, CURL* easy) : client(client)
{
  ThrowIfFailed(TransmitExtendedCloudData(client, easy, invoke, this, nullptr));
}

CoCloud::Client::Client(CloudClient* client, const char* location, struct curl_slist* headers, const char* token, const char* data, size_t length) : client(client)
{
  ThrowIfFailed(TransmitSimpleCloudData(client, location, headers, token, data, length, invoke, this, nullptr));
}

bool CoCloud::Client::query(CURL* easy)
{
  return TransmitExtendedCloudData(client, easy, invoke, this, nullptr);
}

bool CoCloud::Client::query(const char* location, struct curl_slist* headers, const char* token, const char* data, size_t length)
{
  return TransmitSimpleCloudData(client, location, headers, token, data, length, invoke, this, nullptr);
}

void CoCloud::Client::invoke(int code, CURL* easy, char* data, size_t length, void* parameter1, void* parameter2)
{
  static_cast<CoCloud::Client*>(parameter1)->wake({ code, easy, data, length });
}
