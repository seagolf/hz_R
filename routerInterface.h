#ifndef ROUTERINTERFACE_H
#define ROUTERINTERFACE_H

#include <string>
#include <curl/curl.h>


#define IPTABLESCONFIGFILE "/tmp/iptables.txt"
typedef struct 
{
  char *memory;
  size_t size;
}MemoryStruct;


class RouterInterface
{

public:
    static void SingletonInit();

    bool SendQueryRequest(std::string apMacAddr);
    bool SendNewDevicePost(std::string devMacAddr);

    void Run();
    bool HandleIptables(MemoryStruct getResponse);
    static RouterInterface * pfSingleton;

private:
    RouterInterface();
    ~RouterInterface();
    CURL  *pfCurl;
    bool bfShutdown;
};

#endif
