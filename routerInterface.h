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


//typedef struct
//{
    //string apMacAddr;
    //string deviceMacAddr;
    //string hostName;
    //string ipAddr;
    //string os;
    //string vendor;
//}AddNewDevicePost_T;


class RouterInterface
{

public:
    static void SingletonInit();

    bool SendQueryRequest(std::string apMacAddr);
    bool SendNewDevicePost(std::string devMacAddr);

    void Run();
    bool HandleQueryResponse(MemoryStruct getResponse);

    bool UpdateIptables(std::string macAddr, std::string opCode);

    static RouterInterface * pfSingleton;

private:
    RouterInterface();
    ~RouterInterface();
    CURL  *pfCurl;
    bool bfShutdown;
};

#endif
