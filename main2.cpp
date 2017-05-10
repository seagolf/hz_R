#include <string>
#include <unistd.h>
#include <fstream>
#include <stdlib.h>
#include <iostream>

#include "routerInterface.h"

using namespace std;


void restoreExistedRuels()
{

    string restoreCmd (" sudo iptables-restore ");
    restoreCmd.append(IPTABLESCONFIGFILE);
    system( restoreCmd.c_str());
}


RouterInterface * theRouter()
{

    if(RouterInterface::pfSingleton == 0)
    {
        return NULL;
    }
    return RouterInterface::pfSingleton;
}

int main()
{
    //restore existed iptables rules
    ifstream existedRules (IPTABLESCONFIGFILE, ifstream::in);
    if(existedRules.good())
    {
        restoreExistedRuels();
    }
    
    RouterInterface::SingletonInit();

    if (NULL == theRouter())
    {
        cout << "FATAL RouterInterface has not been initialized" << endl;
        return -1;

    }

    //tmp for test 
#if 0
    while (1)
    {
        theRouter()->SendNewDevicePost("hello");

        sleep(3);
    }
#endif
    
    theRouter()->Run();

    return 0;

}
