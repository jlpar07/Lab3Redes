
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

// Estructura del paquete QUIC
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
   
    // SOCK_DGRAM indica UDP (sin conexión previa, como en broker_udp.c)
   
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  
    // El broker escucha en el puerto BROKER_PORT (8080)

    struct sockaddr_in broker_addr, client_addr, subscriber_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_addr.s_addr = INADDR_ANY;          
    broker_addr.sin_port = htons(BROKER_PORT);

    bind(sockfd, (struct sockaddr *)&broker_addr, sizeof(broker_addr));

    
    int has_subscriber = 0;              // ¿Hay un suscriptor registrado?
    int expected_pub_seq = 0;            // SEQ esperado del publicador
    int broker_to_sub_seq = 0;           // SEQ que usa el broker para enviar al suscriptor

    printf("[Broker] Iniciado. Esperando conexiones en puerto %d...\n", BROKER_PORT);


    // El broker recibe mensajes de publicadores y suscriptores

    while (1) {
        quic_packet pkt_in, pkt_ack;
        socklen_t addr_len = sizeof(client_addr);
        
        // El broker espera indefinidamente por mensajes de publicadores o suscriptores
        set_socket_timeout(sockfd, 0);
        recvfrom(sockfd, &pkt_in, sizeof(pkt_in), 0, (struct sockaddr *)&client_addr, &addr_len);

        // Un suscriptor se registra en el broker
        if (pkt_in.type == PACKET_SUB) {
            subscriber_addr = client_addr;  
            has_subscriber = 1;              
            printf("[Broker] Nuevo suscriptor registrado.\n");
        } 
       
        // Un publicador envía datos: el broker debe retransmitir al suscriptor
        else if (pkt_in.type == PACKET_DATA) {
            printf("[Broker] Recibido del Publicador (SEQ %d): %s\n", pkt_in.seq_num, pkt_in.payload);

            // Enviar ACK al publicador (igual que hace broker_udp.c pero con ACK explícito)
            pkt_ack.type = PACKET_ACK;
            pkt_ack.seq_num = pkt_in.seq_num;
            sendto(sockfd, &pkt_ack, sizeof(pkt_ack), 0, (struct sockaddr *)&client_addr, addr_len);

            // Validar que el paquete es el esperado 
            if (pkt_in.seq_num == expected_pub_seq) {
                expected_pub_seq++;

               
                // Si hay un suscriptor registrado, reenviar el mensaje
                if (has_subscriber) {
                    // Cambiar el SEQ al del broker hacia el suscriptor
                
                    pkt_in.seq_num = broker_to_sub_seq;
                    
                    int sub_ack_recibido = 0; 
                    set_socket_timeout(sockfd, TIMEOUT_SEC);  

                  
                    // Retransmitimos hasta recibir ACK del suscriptor
                    while (!sub_ack_recibido) {
                        sendto(sockfd, &pkt_in, sizeof(pkt_in), 0, (struct sockaddr *)&subscriber_addr, sizeof(subscriber_addr));
                        printf("  -> Reenviando al Suscriptor (SEQ %d)\n", pkt_in.seq_num);

                        quic_packet sub_ack;
                        int n = recvfrom(sockfd, &sub_ack, sizeof(sub_ack), 0, NULL, NULL);

                        if (n < 0) {
                          
                            // Retransmitimos automáticamente (mecanismo QUIC)
                            printf("  [!] TIMEOUT: Retransmitiendo...\n");
                        } else if (sub_ack.type == PACKET_ACK && sub_ack.seq_num == broker_to_sub_seq) {
                          
                            printf("  [+] ACK del Suscriptor recibido (SEQ %d).\n\n", sub_ack.seq_num);
                            sub_ack_recibido = 1;
                            broker_to_sub_seq++;  
                        }
                    }
                }
            }
        }
    }
    return 0;
}