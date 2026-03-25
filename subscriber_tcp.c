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

    // Convertir la dirección IP de texto a formato binario
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

    // 4. Enviar un primer mensaje de suscripción al Broker para el tema de interés
    char *topic = "Colombia_vs_Brasil";

    // Se utiliza el siguiente formato: TIPO|PARTIDO
    // "SUB" le indica al broker que es un suscriptor
    sprintf(buffer, "SUB|%s", topic);
    send(sock, buffer, strlen(buffer), 0);  
    printf("Mensaje de suscripción enviado: %s\n", buffer);

    // 5. Bucle para recibir mensajes del Broker (actualizaciones del partido)
    while (1) {
        // Limpieza del buffer antes de recibir algo nuevo
        memset(buffer, 0, BUFFER_SIZE);

        // Recibir un mensaje del Broker
        int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);

        if (valread > 0) {
            // Se recibe un mensaje correctamente
            buffer[valread] = '\0'; // Aseguramos que sea un texto válido en C
            printf("Actualización en vivo: %s\n", buffer);
        } 
        else if (valread == 0) {
            // El broker cerró la conexión (ej. se acabó el partido)
            printf("\nEl Broker ha cerrado la conexión.\n");
            break; 
        } 
        else {
            // Ocurrió un error en la red (valread < 0)
            perror("\nError al recibir datos del Broker");
            break; 
        }
    }
    

    // 6. Cerrar el socket y terminar el programa
    close(sock);
    printf("Transmisión finalizada. Socket cerrado.\n");

    return 0;
}