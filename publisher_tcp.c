#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BROKER_IP "127.0.0.1" // IP del broker
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};  //el buffer se usa como espacio temporal para construir cada mensaje antes de enviarlo por socket.

    // 1. Crear el socket del cliente (AF_INET para IPv4, SOCK_STREAM para TCP)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error en la creación del socket \n");
        return -1;
    }

    // 2. Configurar la dirección y puerto del Broker al que nos vamos a conectar
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    
    if (inet_pton(AF_INET, BROKER_IP, &serv_addr.sin_addr) <= 0) {  //asignar la IP del broker a la estructura del socket
        printf("\n Dirección inválida / No soportada \n");
        return -1;
    }

    // 3. Conectarse al Broker (Esto inicia el Handshake TCP)
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Conexión fallida con el Broker \n");
        return -1;
    }
    printf("Conectado al Broker exitosamente.\n");

    // 4. tema que va a transmitir este publicador
    char *topic = "Colombia_vs_Brasil";

    // 5. Bucle principal para enviar las actualizaciones
    for (int i = 1; i <= 10; i++) {
        // Limpiar el buffer antes de armar un nuevo mensaje
        memset(buffer, 0, BUFFER_SIZE);

        // ARMAR EL MENSAJE (Protocolo de aplicación)
        // Se va a usar el formato: TIPO|PARTIDO|MENSAJE
        // "PUB" indica al broker que es un publicador.
        sprintf(buffer, "PUB|%s|Minuto %d: ¡Ataque peligroso en el partido!", topic, i * 5);  //lo que parsea luego el broker con strtok(), el cual lo separa con |

        // 6. Enviar el mensaje al Broker a través del socket
        send(sock, buffer, strlen(buffer), 0);   //strlen(buffer) son los bytes a enviar, el broker luego lo recibe con recv() y lo procesa con strtok() para separar el tipo (PUB), el topic (Colombia_vs_Brasil) y el mensaje (Minuto X: ¡Ataque peligroso en el partido!)
        printf("Mensaje enviado: %s\n", buffer);

        // Pausar un par de segundos para simular el paso del tiempo
        sleep(2); 
    }

    // 8. Cerrar el socket y terminar el programa
    close(sock);
    printf("Transmisión finalizada. Socket cerrado.\n");

    return 0;
}