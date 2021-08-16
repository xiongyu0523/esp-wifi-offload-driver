/**
 * \file            main.c
 * \brief           Main file
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.0-dev
 */
#include "lwesp/lwesp.h"
#include "netconn_client.h"
#include "tx_api.h"

typedef struct {
    const char* ssid;
    const char* pass;
} ap_entry_t;

#define USER_STACK_SIZE  4096
static TX_THREAD user_thread;
static UCHAR user_stack[USER_STACK_SIZE];

#define NETCONN_STACK_SIZE  4096
static TX_THREAD netconn_thread;
static UCHAR netconn_stack[NETCONN_STACK_SIZE];

static void user_thread_entry(ULONG arg);
static lwespr_t lwesp_callback_func(lwesp_evt_t* evt);
static lwespr_t connect_to_preferred_access_point(uint8_t unlimited);
static void utils_print_ip(const char* str_b, const lwesp_ip_t* ip, const char* str_a);

extern int  board_setup(void);

/**
 * \brief           Program entry point
 */
int
main(void) {

    board_setup();
    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}


/* Define what the initial system looks like.  */
void    tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;
    printf("Application running on STM32L496G-Discovery!\r\n");

    /* Create sample helper thread.  */
    tx_thread_create(&user_thread, "User Thread", 
                     user_thread_entry, 0,
                     user_stack, USER_STACK_SIZE,
                     0, 0,
                     TX_NO_TIME_SLICE, TX_AUTO_START);
}

/**
 * \brief           Initialization thread
 * \param[in]       arg: Thread argument
 */
static void
user_thread_entry(ULONG arg) {
    /* Initialize ESP with default callback function */
    printf("Initializing LwESP\r\n");
    if (lwesp_init(lwesp_callback_func, 1) != lwespOK) {
        printf("Cannot initialize LwESP!\r\n");
    } else {
        printf("LwESP initialized!\r\n");
    }
    /*
     * Continuously try to connect to WIFI network
     * but only in case device is not already connected
     */
    while (1) {
        if (!lwesp_sta_is_joined()) {
            /*
             * Connect to access point.
             *
             * Try unlimited time until access point accepts up.
             * Check for station_manager.c to define preferred access points ESP should connect to
             */
            connect_to_preferred_access_point(1);
        }
        
        tx_thread_sleep(1000);
    }
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
lwesp_callback_func(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            lwesp_sw_version_t v_min, v_curr;

            lwesp_get_min_at_fw_version(&v_min);
            lwesp_get_current_at_fw_version(&v_curr);

            printf("Current ESP8266 AT version is not supported by library!\r\n");
            printf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
            printf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case LWESP_EVT_INIT_FINISH: {
            printf("Library initialized!\r\n");
            break;
        }
        case LWESP_EVT_RESET_DETECTED: {
            printf("Device reset detected!\r\n");
            break;
        }
        case LWESP_EVT_WIFI_IP_ACQUIRED: {        /* We have IP address and we are fully ready to work */
            if (lwesp_sta_is_joined()) {          /* Check if joined on any network */
              
                tx_thread_create(&netconn_thread, "Netconn Thread", 
                                 netconn_client_thread, 0,
                                 netconn_stack, NETCONN_STACK_SIZE,
                                 0, 0,
                                 TX_NO_TIME_SLICE, TX_AUTO_START);
            }
            break;
        }
        default: break;
    }
    return lwespOK;
}

void
utils_print_ip(const char* str_b, const lwesp_ip_t* ip, const char* str_a) {
    if (str_b != NULL) {
        printf("%s", str_b);
    }

    if (0) {
#if LWESP_CFG_IPV6
    } else if (ip->type == LWESP_IPTYPE_V6) {
        printf("%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X\r\n",
            (unsigned)ip->addr.ip6.addr[0], (unsigned)ip->addr.ip6.addr[1], (unsigned)ip->addr.ip6.addr[2],
            (unsigned)ip->addr.ip6.addr[3], (unsigned)ip->addr.ip6.addr[4], (unsigned)ip->addr.ip6.addr[5],
            (unsigned)ip->addr.ip6.addr[6], (unsigned)ip->addr.ip6.addr[7]);
#endif /* LWESP_CFG_IPV6 */
    } else {
        printf("%d.%d.%d.%d\r\n",
            (int)ip->addr.ip4.addr[0], (int)ip->addr.ip4.addr[1],
            (int)ip->addr.ip4.addr[2], (int)ip->addr.ip4.addr[3]);
    }
    if (str_a != NULL) {
        printf("%s", str_a);
    }
}


ap_entry_t
ap_list[] = {
    { "Neo's WIIF", "8220542Xy" },
    { "TilenM_ST", "its private" },
    { "Majerle WIFI", "majerle_internet_private" },
    { "Majerle AMIS", "majerle_internet_private" },
};

/**
 * \brief           List of access points found by ESP device
 */
static
lwesp_ap_t aps[100];

/**
 * \brief           Number of valid access points in \ref aps array
 */
static
size_t apf;

/**
 * \brief           Connect to preferred access point
 *
 * \note            List of access points should be set by user in \ref ap_list structure
 * \param[in]       unlimited: When set to 1, function will block until SSID is found and connected
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
connect_to_preferred_access_point(uint8_t unlimited) {
    lwespr_t eres;
    uint8_t tried;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {
        if (lwesp_sta_has_ip()) {
            return lwespOK;
        }

        /* Scan for access points visible to ESP device */
        printf("Scanning access points...\r\n");
        if ((eres = lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1)) == lwespOK) {
            tried = 0;
            /* Print all access points found by ESP */
            for (size_t i = 0; i < apf; i++) {
                printf("AP found: %s, CH: %d, RSSI: %d\r\n", aps[i].ssid, aps[i].ch, aps[i].rssi);
            }

            /* Process array of preferred access points with array of found points */
            for (size_t j = 0; j < LWESP_ARRAYSIZE(ap_list); j++) {
                for (size_t i = 0; i < apf; i++) {
                    if (!strcmp(aps[i].ssid, ap_list[j].ssid)) {
                        tried = 1;
                        printf("Connecting to \"%s\" network...\r\n", ap_list[j].ssid);
                        /* Try to join to access point */
                        if ((eres = lwesp_sta_join(ap_list[j].ssid, ap_list[j].pass, NULL, NULL, NULL, 1)) == lwespOK) {
                            lwesp_ip_t ip;
                            uint8_t is_dhcp;

                            printf("Connected to %s network!\r\n", ap_list[j].ssid);

                            lwesp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
                            utils_print_ip("Station IP address: ", &ip, "\r\n");
                            printf("; Is DHCP: %d\r\n", (int)is_dhcp);
                            return lwespOK;
                        } else {
                            printf("Connection error: %d\r\n", (int)eres);
                        }
                    }
                }
            }
            if (!tried) {
                printf("No access points available with preferred SSID!\r\nPlease check station_manager.c file and edit preferred SSID access points!\r\n");
            }
        } else if (eres == lwespERRNODEVICE) {
            printf("Device is not present!\r\n");
            break;
        } else {
            printf("Error on WIFI scan procedure!\r\n");
        }
        if (!unlimited) {
            break;
        }
    } while (1);
    return lwespERR;
}



