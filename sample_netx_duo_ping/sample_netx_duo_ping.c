/* This is a small ping demo of the high-performance NetX Duo TCP/IP stack.  */
#include "nx_api.h"
#include "wifi.h"
#include "nxd_dns.h"
#include "nx_secure_tls_api.h"
#include "stm32l4xx_hal.h"  


/* Define the ThreadX and NetX object control blocks...  */

NX_PACKET_POOL    pool_0;
NX_IP             ip_0;  


#ifndef SAMPLE_PACKET_COUNT
#define SAMPLE_PACKET_COUNT             (20)
#endif /* SAMPLE_PACKET_COUNT  */

#ifndef SAMPLE_PACKET_SIZE
#define SAMPLE_PACKET_SIZE              (1200)  /* Set the default value to 1200 since WIFI payload size (ES_WIFI_PAYLOAD_SIZE) is 1200.  */
#endif /* SAMPLE_PACKET_SIZE  */

#define SAMPLE_POOL_SIZE                ((SAMPLE_PACKET_SIZE + sizeof(NX_PACKET)) * SAMPLE_PACKET_COUNT)
#define SAMPLE_IP_STACK_SIZE            (2048)


/* Define the stack/cache for ThreadX.  */
static UCHAR sample_pool_stack[SAMPLE_POOL_SIZE];
static ULONG sample_ip_stack[SAMPLE_IP_STACK_SIZE / sizeof(ULONG)];


VOID board_setup(void);
VOID nx_driver_stm32l4(NX_IP_DRIVER *driver_req_ptr);

int main(int argc, char ** argv)
{
    /* Setup the hardware. */
    board_setup();


    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}


/* Define what the initial system looks like.  */

void    tx_application_define(void *first_unused_memory)
{

UINT  status;
    
     
    /* Initialize the NetX system.  */
    nx_system_initialize();
      
    /* Create a packet pool.  */
    status = nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", SAMPLE_PACKET_SIZE,
                                   sample_pool_stack , SAMPLE_POOL_SIZE);
    
    /* Check for pool creation error.  */
    if (status)
    {
        printf("PACKET POOL CREATE FAIL.\r\n");
        return;
    }
        
    /* Create an IP instance.  */
     status = nx_ip_create(&ip_0, "NetX IP Instance 0", 0, 0,
                           &pool_0, nx_driver_stm32l4, (UCHAR*)sample_ip_stack, sizeof(sample_ip_stack), 1);

    /* Check for IP create errors.  */
    if (status)
    {
	  	printf("IP CREATE FAIL.\r\n");
        return;
    }
}
