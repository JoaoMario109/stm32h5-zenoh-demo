/**
 * Derived from:
 *  - https://github.com/eclipse-zenoh/zenoh-pico/blob/main/examples/freertos_plus_tcp/include/FreeRTOSIPConfig.h
 *  - https://github.com/htibosch/freertos_plus_projects/blob/master/STM32H747_cube/CM7/Core/Inc/FreeRTOSIPConfig.h
 *
 * Thanks!
 */

#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

#include "stm32h5xx_hal.h"

/** Driver specific */
#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN

#define ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES (1)

#define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM      (0)
#define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM      (1)

#define ipconfigZERO_COPY_RX_DRIVER                 (1)
#define ipconfigZERO_COPY_TX_DRIVER                 (1)

#define ipconfigUSE_LINKED_RX_MESSAGES              (1)

/**
 * Several API's will block until the result is known, or the action has been
 * performed, for example FreeRTOS_send() and FreeRTOS_recv(). The timeouts can be
 * set per socket, using setsockopt(). If not set, the times below will be
 * used as defaults.
 */
#define ipconfigSOCK_DEFAULT_RECEIVE_BLOCK_TIME (5000)
#define	ipconfigSOCK_DEFAULT_SEND_BLOCK_TIME    (5000)

/**
 * Include support for DNS caching. For TCP, having a small DNS cache is very
 * useful.  When a cache is present, ipconfigDNS_REQUEST_ATTEMPTS can be kept low
 * and also DNS may use small timeouts. If a DNS reply comes in after the DNS
 * socket has been destroyed, the result will be stored into the cache.  The next
 * call to FreeRTOS_gethostbyname() will return immediately, without even creating
 * a socket.
 */
#define ipconfigUSE_DNS_CACHE                 (1)
#define ipconfigDNS_CACHE_NAME_LENGTH         (16)
#define ipconfigDNS_CACHE_ENTRIES             (4)
#define ipconfigDNS_REQUEST_ATTEMPTS          (4)
#define ipconfigDNS_CACHE_ADDRESSES_PER_ENTRY (4)


/** Features */
#define ipconfigUSE_IPv4                (1)
#define ipconfigUSE_IPv6                (1)
#define ipconfigUSE_DHCP                (1)
#define ipconfigUSE_DHCPv6              (0)
#define ipconfigUSE_RA                  (0)
#define ipconfigUSE_DNS                 (1)
#define ipconfigUSE_TCP                 (1)
#define ipconfigUSE_ARP_REMOVE_ENTRY    (0)
#define ipconfigUSE_ARP_REVERSED_LOOKUP (0)
#define ipconfigSUPPORT_SELECT_FUNCTION (0)
#define ipconfigSUPPORT_OUTGOING_PINGS  (0)
#define ipconfigUSE_NETWORK_EVENT_HOOK  (1)
#define ipconfigUSE_DHCP_HOOK           (0)

/** DHCP */
#define ipconfigDHCP_REGISTER_HOSTNAME      (1)
#define ipconfigMAXIMUM_DISCOVER_TX_PERIOD  (pdMS_TO_TICKS(1000U))

#define ipconfigIP_TASK_PRIORITY            (configMAX_PRIORITIES - 2)
#define ipconfigIP_TASK_STACK_SIZE_WORDS    (configMINIMAL_STACK_SIZE * 5)

/**
 * ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS defines the total number of network buffer that
 * are available to the IP stack. The total number of network buffers is limited
 * to ensure the total amount of RAM that can be consumed by the IP stack is capped
 * to a pre-determinable value.
 */
#define ETH_TX_BUF_SIZE                           (2048U)
#define ETH_RX_BUF_SIZE                           (2048U)
#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS    (64)

#define iptraceFAILED_TO_OBTAIN_NETWORK_BUFFER()  configASSERT(1 == 0)
#define traceISR_ENTER()                          configASSERT(1 == 0)

/**
 * A FreeRTOS queue is used to send events from application tasks to the IP
 * stack. ipconfigEVENT_QUEUE_LENGTH sets the maximum number of events that can
 * be queued for processing at any one time. The event queue must be a minimum of
 * 5 greater than the total number of network buffers.
 */
#define ipconfigEVENT_QUEUE_LENGTH (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS + 5)

/**
 * The MTU is the maximum number of bytes the payload of a network frame can
 * contain.  For normal Ethernet V2 frames the maximum MTU is 1500.  Setting a
 * lower value can save RAM, depending on the buffer management scheme used.  If
 * ipconfigCAN_FRAGMENT_OUTGOING_PACKETS is 1 then (ipconfigNETWORK_MTU - 28) must
 * be divisible by 8.
 */
#define ipconfigNETWORK_MTU (1500)

/**
 * If ipconfigFILTER_OUT_NON_ETHERNET_II_FRAMES is set to 1 then Ethernet frames
 * that are not in Ethernet II format will be dropped. This option is included for
 * potential future IP stack developments.
 */
#define ipconfigFILTER_OUT_NON_ETHERNET_II_FRAMES 1

/**
 * Advanced only: in order to access 32-bit fields in the IP packets with
 * 32-bit memory instructions, all packets will be stored 32-bit-aligned, plus
 * 16-bits.  This has to do with the contents of the IP-packets: all 32-bit fields
 * are 32-bit-aligned, plus 16-bit(!).
 */
#define ipconfigPACKET_FILLER_SIZE 2

/** Set ipconfigBUFFER_PADDING on 64-bit platforms */
#if INTPTR_MAX == INT64_MAX
  #define ipconfigBUFFER_PADDING 14U
#endif

#endif /* FREERTOS_IP_CONFIG_H */
