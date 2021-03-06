#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


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

        string apMacAddr("FC:F5:28:D4:81:AA");

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
    
    string url("http://60.205.212.99/squirrel/v1/devices/ap_interval?apMacAddr=");

    for(int i = 0; i < 18; i+=3)
    {
        string tmpString = apMacAddr.substr(i,2);
        url.append(tmpString);

        if( i < 15)
        {
            url.append("%3A");    
        }
    
    }

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
            long response_code;
            curl_easy_getinfo(pfCurl, CURLINFO_RESPONSE_CODE, &response_code);

            if( response_code != 200)
            {
                cout <<" Error! response code: " << (int) response_code << endl;
            }

            char *ct;
             //ask for the content-type  
            res = curl_easy_getinfo(pfCurl, CURLINFO_CONTENT_TYPE, &ct);

            if((CURLE_OK == res) && ct)
            {
                printf("We received Content-Type: %s\n", ct);
            }
        }

        printf("%lu bytes received\n",(long) getResponse.size);

        HandleQueryResponse (getResponse);
    }

}

bool RouterInterface::HandleQueryResponse(MemoryStruct getResponse)
{

    //clean all iptables for avoid duplicated rules
    //system("sudo iptables -F");
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


        if(! UpdateIptables(macAddr, operation))

        {
            cout << " UpdateIptables failed: " << macAddr << " opCode: " << operation << endl;
        
        }


        iptablesEntry = strtok(NULL, ",");


    }


#ifndef NDEBUG 

    if (0> system ("sudo iptables -L"))
    {
        cout << "iptables failed" << endl;
    }
#endif
}

bool RouterInterface::SendNewDevicePost(string jsonObj)
{
    

    CURLcode res;
    string url("http://60.205.212.99/squirrel/v1/devices/add_device_to_user");
    string tmpjsonObj( "{ \
            \"apMacAddr\": \"FC:F5:28:D4:81:AA\", \
            \"deviceMacAddr\" : \"00:00:00:DD:EE:FF\", \
            \"hostName\" : \"hzspec\", \
            \"ip\" : \"192.168.1.2\", \
            \"os\" : \"mac\", \
            \"vendor\" : \"apple\" \
            }");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");


    curl_easy_setopt(pfCurl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(pfCurl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pfCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(pfCurl, CURLOPT_POSTFIELDS, tmpjsonObj.c_str()); 
    //curl_easy_setopt(pfCurl, CURLOPT_VERBOSE, 1L);


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
            long response_code;
        
            curl_easy_getinfo(pfCurl, CURLINFO_RESPONSE_CODE, &response_code);

            if( response_code != 200)
            {
                cout <<" Error! response code:"  << (int) response_code << endl;
            }

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

bool RouterInterface::UpdateIptables(string macAddr, string opCode) 
{
    ifstream iptablesFile;
    string iptablesEntry;

    iptablesFile.open(IPTABLESCONFIGFILE, std::ifstream::in);

    bool bFound = false;
    bool bNeedUpdate = false;

    while (getline(iptablesFile, iptablesEntry))

    {

        //a exist device mac
        if (string::npos != iptablesEntry.find(macAddr))
        {
            size_t actionPos = iptablesEntry.find("-j ");   
            if(string::npos != actionPos)
            {
                actionPos +=3;
                string prevAction = iptablesEntry.substr(actionPos);

                if(prevAction != opCode)
                {
                    ostringstream replaceCmd;
                    replaceCmd << "sudo iptables -C FORWARD -m mac --mac-source ";
                    replaceCmd << macAddr << "-j " << opCode;

#ifndef NDEBUG
                    cout << replaceCmd.str() << endl;
#endif
                    system(replaceCmd.str().c_str());

                    bFound = true;
                    bNeedUpdate = true;
                    break;


                }
                else
                {
                
                    bFound = true;
                    bNeedUpdate = false;
                    cout <<iptablesEntry << " : entry already exists!" << endl;
                }

            }
            else
            {
            
                cout << " current rule error!" << endl;
            
            }

        }

    }

    //a new entry
    if( !bFound )
    {
        ostringstream addNewEntry;
        addNewEntry << "sudo iptables -A FORWARD -m mac --mac-source " << macAddr << " -j "  << opCode  << endl;

        system(addNewEntry.str().c_str());

        bNeedUpdate = true;

    
    }
    if (bNeedUpdate)
    {
        ostringstream saveCmd;
        saveCmd << "sudo iptables-save > " << IPTABLESCONFIGFILE ;

        system(saveCmd.str().c_str());
     }
    


    return true;

}
