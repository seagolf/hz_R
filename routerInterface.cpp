#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>


#include "routerInterface.h"

using namespace std;

RouterInterface *RouterInterface::pfSingleton = 0;

RouterInterface::RouterInterface()
    :bfShutdown(false)
{

    curl_global_init(CURL_GLOBAL_ALL);

    //init the curl session 
    pfCurl = curl_easy_init();

}


RouterInterface::~RouterInterface()
{
    curl_easy_cleanup(pfCurl);
}


void RouterInterface::SingletonInit()
{
    //create the sigleton instance
    if(RouterInterface::pfSingleton == 0)
    {
        RouterInterface::pfSingleton = new RouterInterface();
    }

    else
    {
        cout << "WARNING: RouterInterface pfSingleton already created!" <<endl;

    }
    return;
}


void RouterInterface::Run()
{

    while (!bfShutdown)
    {

        string apMacAddr("FC\%3AF5\%3A28\%3AD4\%3A81\%3AAA");

        //send http GET  and handle reponse message
        if (!SendQueryRequest(apMacAddr))
        {
            cout << "sendQueryRequest failed" <<endl;
        }


        sleep(5);
    }

}
static
size_t IptablesOperateCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  MemoryStruct *mem = (MemoryStruct *)userp;
 
  mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
      /* out of memory! */ 
      printf("not enough memory (realloc returned NULL)\n");
      return 0;
    }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}


bool RouterInterface::SendQueryRequest(string apMacAddr)
{

    if(pfCurl == NULL)
    {
        cout << " Error curl instance has not been created" << endl;
        return false;
    }

    CURLcode res;
    string url("http://60.205.212.99/squirrel/v1/devices/ap_interval?apMacAddr=FC%3AF5%3A28%3AD4%3A81%3AAA");
    //url.append(apMacAddr);

    MemoryStruct getResponse;
    getResponse.memory = (char*) malloc(1);  // will be grown as needed by the realloc above 
    getResponse.size = 0;    // no data at this point 

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: */*");

    curl_easy_setopt(pfCurl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(pfCurl, CURLOPT_URL, url.c_str());


    //register callback
    curl_easy_setopt(pfCurl, CURLOPT_WRITEFUNCTION, IptablesOperateCallback);
    curl_easy_setopt(pfCurl, CURLOPT_WRITEDATA, (void *) &getResponse);

    //some servers don't like requests that are made without a user-agent field, so we provide one  
    curl_easy_setopt(pfCurl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(pfCurl);

    // check for errors
    if(res != CURLE_OK)
    {
        cout<<"ERROR: curl_easy_perform failed" << curl_easy_strerror(res) << endl;
    }
    else
    {
        //Now, our chunk.memory points to a memory block that is chunk.size
        //bytes big and contains the remote file.

        if(CURLE_OK == res) 
        {
            char *ct;
             //ask for the content-type  
            res = curl_easy_getinfo(pfCurl, CURLINFO_CONTENT_TYPE, &ct);

            if((CURLE_OK == res) && ct)
            {
                printf("We received Content-Type: %s\n", ct);
            }
        }

        printf("%lu bytes received\n",(long) getResponse.size);

        HandleIptables (getResponse);
    }

}

bool RouterInterface::HandleIptables(MemoryStruct getResponse)
{

    //clean all iptables for avoid duplicated rules
    system("sudo iptables -F");
    char *iptablesEntry;

    //get each iptables entry, format:  AB:CD:EF:00:00:00-1
    iptablesEntry = strtok(getResponse.memory, ",");
    while(iptablesEntry != NULL)
    {
        string tableString(iptablesEntry);

        cout << "iptables Entry:" << tableString << endl; 

        std::size_t dashPosition = tableString.find("-");
        string macAddr = tableString.substr(0, dashPosition);

        cout<< "---> mac: " << macAddr << endl;

        string opCodeStr = tableString.substr(dashPosition+1);
        string operation = (opCodeStr == "1") ? "ACCEPT":"DROP" ;
        cout<< "---> action:" << operation << endl;


        //update iptables configuration file

        string cmd ("sudo iptables -A FORWARD -m mac --mac-source ");
        cmd.append(macAddr);
        cmd.append(" -j ");
        cmd.append(operation);

        cout <<" cmd : " << cmd << endl;

        system(cmd.c_str());

        //updateIptables(macAddr, operation);

        iptablesEntry = strtok(NULL, ",");


    }

    //restore iptables configruation;
    string restoreCmd (" sudo iptables-save > ");
    restoreCmd.append(IPTABLESCONFIGFILE);
    system( restoreCmd.c_str());

    cout << "restorecmd :" << restoreCmd << endl;

    if (0> system ("sudo iptables -L"))
    {
        cout << "iptables failed" << endl;
    }
}

bool RouterInterface::SendNewDevicePost(std::string devMacAddr)
{
    

    CURLcode res;
    string url("http://60.205.212.99/squirrel/v1/devices/add_device_to_user");
    //url.append(apMacAddr);



    curl_easy_setopt(pfCurl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pfCurl, CURLOPT_POSTFIELDS, "{ \
            \"apMacAddr\": \"FC:F5:28:D4:81:AA\", \
            \"deviceMacAddr\" : \"00:00:00:DD:EE:FF\", \
            \"hostname\" : \"hzspec\", \
            \"ip\" : \"192.168.1.2\", \
            \"os\" : \"mac\", \
            \"vendor\" : \"apple\" \
            }");


    //register callback

    //some servers don't like requests that are made without a user-agent field, so we provide one  
    curl_easy_setopt(pfCurl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(pfCurl);

    // check for errors
    if(res != CURLE_OK)
    {
        cout<<"ERROR: curl_easy_perform failed" << curl_easy_strerror(res) << endl;
    }
    else
    {
        //Now, our chunk.memory points to a memory block that is chunk.size
        //bytes big and contains the remote file.

        if(CURLE_OK == res) 
        {
            char *ct;
             //ask for the content-type  
            res = curl_easy_getinfo(pfCurl, CURLINFO_CONTENT_TYPE, &ct);

            if((CURLE_OK == res) && ct)
            {
                printf("We received Content-Type: %s\n", ct);
            }
        }
    }
}
