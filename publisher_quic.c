
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
    int type;                      // Tipo de paquete (DATA, ACK)
    int seq_num;                   // Número de secuencia (para control de entrega)
    char payload[MAX_PAYLOAD];     // Datos del mensaje
} quic_packet;

// Función para establecer timeout en socket (esperar máximo TIMEOUT_SEC segundos para recibir datos)
// Similar a TCP/UDP, pero QUIC es UDP-based y maneja retransmisiones con timeout
void set_socket_timeout(int sockfd, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int main() {
   
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    
    // Si no llega respuesta en 2s, retransmitimos
    set_socket_timeout(sockfd, TIMEOUT_SEC);

   
    struct sockaddr_in broker_addr;
    memset(&broker_addr, 0, sizeof(broker_addr));         
    broker_addr.sin_family = AF_INET;                     
    broker_addr.sin_port = htons(BROKER_PORT);           
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    
    char *eventos[] = {
        "Minuto 1: Inicio del partido",
        "Minuto 12: Falta en el medio campo",
        "Minuto 20: Gol de Millonarios!",
        "Minuto 35: Tarjeta amarilla para el defensa",
        "Minuto 45: Fin del primer tiempo",
        "Minuto 46: Inicio del segundo tiempo",
        "Minuto 55: Tiro de esquina",
        "Minuto 70: Sustitucion en el equipo visitante",
        "Minuto 85: Gol anulado por fuera de lugar",
        "Minuto 90+3: Fin del partido"
    };
    int num_eventos = 10;
    int current_seq = 0;  

    // "stop-and-wait" con retransmisión
    for (int i = 0; i < num_eventos; i++) {
        quic_packet pkt_out, pkt_in;  
        
        
        pkt_out.type = PACKET_DATA;
        pkt_out.seq_num = current_seq;
        strcpy(pkt_out.payload, eventos[i]);

        int ack_recibido = 0;  
        socklen_t addr_len = sizeof(broker_addr);

       
        // Retransmitimos hasta recibir ACK del broker
    
        while (!ack_recibido) {
            
            printf("[Publicador] Enviando evento (SEQ %d): %s\n", pkt_out.seq_num, pkt_out.payload);
            sendto(sockfd, &pkt_out, sizeof(pkt_out), 0, (struct sockaddr *)&broker_addr, addr_len);

          
            // Si timeout expira, recvfrom() retorna -1 y retransmitimos
            int n = recvfrom(sockfd, &pkt_in, sizeof(pkt_in), 0, (struct sockaddr *)&broker_addr, &addr_len);

            if (n < 0) {
             
                printf("[!] TIMEOUT: No llego ACK para SEQ %d. Retransmitiendo...\n", pkt_out.seq_num);
            } else if (pkt_in.type == PACKET_ACK && pkt_in.seq_num == current_seq) {
          
                printf("[+] ACK recibido para SEQ %d. Exito.\n\n", current_seq);
                ack_recibido = 1;
                current_seq++; 
            }
        }
        
        // Pausar 3 segundos antes de enviar el próximo evento
        sleep(3);
    }

    // Cerrar la conexión cuando terminamos
    close(sockfd);
    return 0;
}