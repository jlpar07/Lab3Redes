
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>


#define MAX_PAYLOAD 1024          
#define TIMEOUT_SEC 2             
#define BROKER_PORT 8080          
#define PACKET_DATA 1             
#define PACKET_ACK 2              
#define PACKET_SUB 3             

// Estructura del paquete QUIC (protocolo a medida con confirmación de entrega)
typedef struct {
    int type;                     
    int seq_num;                  
    char payload[MAX_PAYLOAD];    
} quic_packet;

// Función para establecer timeout en socket (esperar máximo TIMEOUT_SEC segundos para recibir datos)
void set_socket_timeout(int sockfd, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int main() {
    
    // SOCK_DGRAM indica UDP (sin conexión previa, como en subscriber_udp.c)
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;                      
    broker_addr.sin_port = htons(BROKER_PORT);            
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

 
    quic_packet pkt_sub;
    pkt_sub.type = PACKET_SUB;  // Indicar que es una suscripción
    
    
    sendto(sockfd, &pkt_sub, sizeof(pkt_sub), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr));
    printf("[Suscriptor] Solicitud de suscripcion enviada.\n");

   
    // Similar a subscriber_tcp.c (recv loop) pero con ACK por cada mensaje (mecanismo QUIC)
    
    int expected_seq = 0;  // Esperamos que el primer mensaje tenga SEQ = 0

    while (1) {
        quic_packet pkt_in, pkt_ack;  // Paquete a recibir y paquete ACK a enviar
        socklen_t addr_len = sizeof(broker_addr);

        // Recibir paquete del broker
        recvfrom(sockfd, &pkt_in, sizeof(pkt_in), 0, (struct sockaddr *)&broker_addr, &addr_len);

        
        if (pkt_in.type == PACKET_DATA) {
            
            if (pkt_in.seq_num == expected_seq) {
                printf("[Suscriptor] Evento recibido en orden (SEQ %d): %s\n", pkt_in.seq_num, pkt_in.payload);
                expected_seq++;  
            } else {
                
                printf("[!] Paquete desordenado o duplicado (SEQ %d, esperaba %d).\n", pkt_in.seq_num, expected_seq);
            }

           
            // Confirmamos al broker que recibimos el paquete (sin importar si fue duplicado)
            pkt_ack.type = PACKET_ACK;
            pkt_ack.seq_num = pkt_in.seq_num;  // Confirmar el SEQ recibido
            sendto(sockfd, &pkt_ack, sizeof(pkt_ack), 0, (struct sockaddr *)&broker_addr, addr_len);
            printf("[Suscriptor] ACK enviado para SEQ %d\n", pkt_ack.seq_num);
        }
    }

    close(sockfd);
    return 0;
}