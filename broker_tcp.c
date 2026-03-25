#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h> 

#define PORT 8080
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

// Estructura para llevar el registro de quién está conectado
typedef struct {
    int socket_fd;   // Descriptor de socket
    char topic[50]; // topics
    int active;     // 1 si está en uso, 0 si está libre
} Client;


int main() {
    int server_fd, new_socket;  // Descriptor del socket del servidor y del nuevo cliente
    struct sockaddr_in address; // Estructura para la dirección del socket
    socklen_t addrlen = sizeof(address); // Variable para el tamaño de la dirección, se usa socklen:t para evitar problemas de tamaño
    char buffer[BUFFER_SIZE];  // Buffer para recibir y enviar mensajes
    
    // Arreglo para guardar el estado de todos los clientes conectados
    Client clients[MAX_CLIENTS] = {0}; 

    // 1. Crear el socket principal del broker
    server_fd = socket(AF_INET, SOCK_STREAM, 0);  //AF_INET para dirección IPv4, SOCK_STREAM para TCP

    // 2. Configurar la dirección y hacer bind() al PORT
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Aceptar conexiones de cualquier IP
    address.sin_port = htons(PORT); // Convertir el puerto a formato de red
    bind(server_fd, (struct sockaddr *)&address, addrlen);

    // 3. Poner el socket en modo escucha
    listen(server_fd, 10);  // 10 es el back-log, el número de conexiones pendientes que el sistema puede manejar
    printf("Broker TCP escuchando en el puerto %d...\n", PORT);

    // Set de descriptores de archivo para select()
    fd_set readfds;

    // Bucle principal del servidor
    while(1) {
        // Limpiar el set de sockets y añadir el server_fd
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd; // Guardar el descriptor de socket máximo

        // Añadir los sockets de los clientes activos al set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket_fd;
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // 4. Esperar actividad en ALGÚN socket (esto bloquea hasta que pase algo)
        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // 5. Detección de nuevas conexiones
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            // Guardar 'new_socket' en el primer espacio libre del arreglo 'clients'
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active == 0) {
                    clients[i].socket_fd = new_socket;
                    clients[i].active = 1;
                    break;
                }
            }
            printf("Nueva conexión aceptada.\n");
        }

        // 6. Detección de mensajes entrantes en los sockets de los clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket_fd;

            if (FD_ISSET(sd, &readfds)) {
                // Leer el mensaje con recv()
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);

                // Si valread == 0, el cliente se desconectó. Hay que cerrar el socket y liberar clients[i]
                if (valread == 0) {
                    printf("Cliente desconectado.\n");
                    close(sd);
                    clients[i].socket_fd = 0;
                    clients[i].active = 0;
                }else if (valread < 0) {
                    printf("Error de red");  // Si ocurre un error en la red, también se cierra el socket y se libera el espacio
                    close(sd);
                    clients[i].socket_fd = 0;
                    clients[i].active = 0;
                
                
                }else {
                    if (valread >= BUFFER_SIZE) valread = BUFFER_SIZE - 1;
                    buffer[valread] = '\0';

                    char original_msg[BUFFER_SIZE];
                    strcpy(original_msg, buffer);

                    char *tipo = strtok(buffer, "|");
                    char *topic = strtok(NULL, "|");
                    if (tipo == NULL || topic == NULL) continue;

                    if (strcmp(tipo, "SUB") == 0) {  // Si devuelve > 0, procesas el mensaje (SUB o PUB) y, si es PUB, se redistribuye a suscriptores del topic.
                        strncpy(clients[i].topic, topic, sizeof(clients[i].topic) - 1);
                        clients[i].topic[sizeof(clients[i].topic) - 1] = '\0';
                    } else if (strcmp(tipo, "PUB") == 0) {
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].active && clients[j].socket_fd > 0 && strcmp(clients[j].topic, topic) == 0) {
                                send(clients[j].socket_fd, original_msg, strlen(original_msg), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
