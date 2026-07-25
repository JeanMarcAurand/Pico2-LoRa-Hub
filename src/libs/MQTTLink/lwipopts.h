#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Configuration réseau de base
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1

#define LWIP_TCP_KEEPALIVE          1

// DHCP et DNS (indispensable pour se connecter à ta box)
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define TCP_MSS                     1460
#define LWIP_WND_SCALE              1
#define TCP_RCV_SCALE               0
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            16
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_UDP                    1
#define LWIP_DNS                    1

#define LWIP_TCP                1
#define MEMP_NUM_TCP_PCB        5    // Autorise jusqu'à 5 connexions TCP simultanées
#define MEMP_NUM_SYS_TIMEOUT    15   // Nécessaire pour les timeouts MQTT
#define MEMP_NUM_MQTT_CLIENT    1
#define MEMP_NUM_MQTT_TOPIC     1
#define MEMP_NUM_MQTT_REQUEST   1

// Configuration pour MQTT
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0
#define LWIP_STATS                  0
#define LWIP_CHKSUM_COPY_ALGORITHM  1
#define MQTT_VAR_HEADER_BUFFER_LEN 128
#define MQTT_REQ_MAX_IN_FLIGHT 8 // Permet d'avoir jusqu'à 4 messages en attente de confirmation
// Taille du buffer d'émission TCP (doit être assez grand pour contenir tes messages)
#define MCP_TCP_SND_BUF            (8 * TCP_MSS)

// Paramètres de mémoire (ajustables si besoin)
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    16384
// Nombre de buffers de paquets. 
// Augmenter à 32 aide à absorber la latence Wi-Fi sans perdre de paquets.
#define PBUF_POOL_SIZE 32

#endif