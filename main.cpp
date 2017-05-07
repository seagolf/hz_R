#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>


#define DELIM ","
#define SUB_DELIM "-"    
#define IPTABLES_FILE "/tmp/iptables.txt"
using namespace std;


typedef struct {
  char *memory;
  size_t size;
}MemoryStruct;
 
static size_t
IptablesOperateCallback(void *contents, size_t size, size_t nmemb, void *userp)
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


bool sendQueryRequest( CURL *curl)
{

    //get https response string

    CURLcode res;

    MemoryStruct response;

    response.memory = (char*) malloc(1);  /* will be grown as needed by the realloc above */ 
    response.size = 0;    /* no data at this point */ 

    string postthis("apMacAddr=AB:CD:EF:FE:DC:BA");

    if(curl == NULL)
    {
        cout << " Error curl instance has not been created" << endl;
        return false;
    }

    //url
    curl_easy_setopt(curl, CURLOPT_URL, \
            "http://60.205.212.99/squirrel/swagger-ui.html#!/DeviceController/apIntervalUsingPOST"); 
    
    //post data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,postthis);
    //if we don't provide POSTFIELDSIZE, libcurl will strlen() by itself 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postthis.length());
         
    
    //register callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, IptablesOperateCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
    
    //some servers don't like requests that are made without a user-agent field, so we provide one  
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl);

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
            /* ask for the content-type */ 
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if((CURLE_OK == res) && ct)
            {
                printf("We received Content-Type: %s\n", ct);
            }
            else
            {
                cout << "response: " << res << endl;
            }
        }

    
             
                                             
        printf("%s\n",response.memory);
        char *iptablesEntry;

        //get each iptables entry, format:  AB:CD:EF:00:00:00-1
        iptablesEntry = strtok(response.memory, ",");
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

            string cmd ="sudo iptables  -C FORWARD -m mac --mac-source ";
            cmd.append(macAddr);
            cmd.append(" -j ");
            cmd.append(operation);

            cout << "cmd: " << cmd << endl;


            if (0> system (cmd.c_str()))
            {
                cout << "iptables failed" << endl;
            }

            iptablesEntry = strtok(NULL, ",");


        }


        if (0> system ("sudo iptables -L"))
        {
            cout << "iptables failed" << endl;
        }
    }

}


int main()
{

    //create a file to store iptables rules
    
    if (!ifstream(IPTABLES_FILE, ifstream::in))
    {
        ofstream iptableConfigFile(IPTABLES_FILE);
        if (! iptableConfigFile)
        {
            cout << "File could not be created" << endl;
            return false;
        }
         cout << "Iptables Configuratoin File is created" << endl;
    }
    else
    {
        //restore previous iptables
        string restoreCmd (" sudo iptables-restore ");
        restoreCmd.append(IPTABLES_FILE);
        system( restoreCmd.c_str());
        
    }
    
    //init curl handle instance
    CURL * curl = NULL;

    curl_global_init(CURL_GLOBAL_ALL);

    //init the curl session 
    curl = curl_easy_init();

    while (1)
    {
    
        if (!sendQueryRequest(curl))
        {
            cout << "sendQueryRequest failed" <<endl;
        }

        sleep(5);
    }
#if 0
    else
    {

        char response[] = "AB:CD:EF:00:00:00-1,00:00:00:AB:CD:EF-0";
        char *iptablesEntry;

        //get each iptables entry, format:  AB:CD:EF:00:00:00-1
        iptablesEntry = strtok(response, ",");
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

            string cmd ="sudo iptables  -C FORWARD -m mac --mac-source ";
            cmd.append(macAddr);
            cmd.append(" -j ");
            cmd.append(operation);

            cout << "cmd: " << cmd << endl;


            if (0> system (cmd.c_str()))
            {
                cout << "iptables failed" << endl;
            }

            iptablesEntry = strtok(NULL, ",");


        }
    }
#endif



}


