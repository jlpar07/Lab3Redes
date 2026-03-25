#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BROKER_PUB_PORT 9001
#define BUFFER_SIZE 2048

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Uso: %s <broker_ip> <tema>\n", argv[0]);
    fprintf(stderr, "  Ejemplo: %s 127.0.0.1 \"EquipoA_vs_EquipoB\"\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *broker_ip = argv[1];
  const char *topic = argv[2];
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in broker_addr;
  memset(&broker_addr, 0, sizeof(broker_addr));
  broker_addr.sin_family = AF_INET;
  broker_addr.sin_port = htons(BROKER_PUB_PORT);
  if (inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr) != 1) {
    fprintf(stderr, "Dirección IP inválida: %s\n", broker_ip);
    close(sock);
    exit(EXIT_FAILURE);
  }

  printf("[Publisher] Tema: '%s' → broker %s:%d\n", topic, broker_ip,
         BROKER_PUB_PORT);
  printf("[Publisher] Escribe mensajes (Ctrl+C para salir):\n\n");

  char input[BUFFER_SIZE];
  char packet[BUFFER_SIZE];
  int msg_num = 1;

  while (1) {
    printf("[%d] > ", msg_num);
    fflush(stdout);

    /* Leer línea del usuario */
    if (fgets(input, sizeof(input), stdin) == NULL)
      break;

    /* Eliminar salto de línea */
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n')
      input[len - 1] = '\0';
    if (strlen(input) == 0)
      continue;

    /* Construir paquete con el protocolo definido */
    snprintf(packet, sizeof(packet), "PUB|%.63s|%.950s", topic, input);

    ssize_t sent = sendto(sock, packet, strlen(packet), 0,
                          (struct sockaddr *)&broker_addr, sizeof(broker_addr));
    if (sent < 0) {
      perror("sendto");
    } else {
      printf("[Publisher] Enviado (%zd bytes): \"%s\"\n", sent, packet);
    }

    msg_num++;
  }

  close(sock);
  printf("[Publisher] Desconectado.\n");
  return 0;
}
