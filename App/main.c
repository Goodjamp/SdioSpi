#include "SystemClock.h"
#include "SdSpiExample.h"

#include "DebugServices.h"

void main(void)
{
    /* Configure the system clock */
    systemClockInit();

    debugServicesInit(NULL);
    sdSpiExampleRun();

    while(1){
    }
}