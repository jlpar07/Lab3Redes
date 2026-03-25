

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BROKER_SUB_PORT 9002
#define LOCAL_PORT 0
#define BUFFER_SIZE 1024
#define MAX_TOPIC_LEN 64

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Uso: %s <broker_ip> <tema1> [<tema2> ...]\n", argv[0]);
    fprintf(stderr, "  Ejemplo: %s 127.0.0.1 EquipoA_vs_EquipoB\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *broker_ip = argv[1];
  int num_topics = argc - 2;
  char **topics = &argv[2];

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in local_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port = htons(LOCAL_PORT);

  if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
    perror("bind");
    close(sock);
    exit(EXIT_FAILURE);
  }

  socklen_t addr_len = sizeof(local_addr);
  getsockname(sock, (struct sockaddr *)&local_addr, &addr_len);
  printf("[Subscriber] Escuchando en puerto local %d\n",
         ntohs(local_addr.sin_port));

  struct sockaddr_in broker_addr;
  memset(&broker_addr, 0, sizeof(broker_addr));
  broker_addr.sin_family = AF_INET;
  broker_addr.sin_port = htons(BROKER_SUB_PORT);

  if (inet_pton(AF_INET, broker_ip, &broker_addr.sin_addr) != 1) {
    fprintf(stderr, "Dirección IP inválida: %s\n", broker_ip);
    close(sock);
    exit(EXIT_FAILURE);
  }

  for (int t = 0; t < num_topics; t++) {
    char sub_msg[BUFFER_SIZE];
    snprintf(sub_msg, sizeof(sub_msg), "SUB|%s", topics[t]);

    ssize_t sent = sendto(sock, sub_msg, strlen(sub_msg), 0,
                          (struct sockaddr *)&broker_addr, sizeof(broker_addr));
    if (sent < 0) {
      perror("sendto (SUB)");
    } else {
      printf("[Subscriber] Suscrito al tema '%s'\n", topics[t]);
    }
  }

  printf("[Subscriber] Esperando actualizaciones...\n\n");

  char buffer[BUFFER_SIZE];
  struct sockaddr_in sender_addr;
  int msg_count = 0;

  while (1) {
    socklen_t slen = sizeof(sender_addr);
    ssize_t bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr *)&sender_addr, &slen);
    if (bytes <= 0) {
      perror("recvfrom");
      continue;
    }

    buffer[bytes] = '\0';
    msg_count++;

    char prefix[8], topic[MAX_TOPIC_LEN], message[BUFFER_SIZE];
    if (sscanf(buffer, "%7[^|]|%63[^|]|%1023[^\n]", prefix, topic, message) ==
            3 &&
        strcmp(prefix, "MSG") == 0) {

      printf("┌─────────────────────────────────────────┐\n");
      printf("│ [Actualización #%d]                      \n", msg_count);
      printf("│ Partido : %s\n", topic);
      printf("│ Evento  : %s\n", message);
      printf("└─────────────────────────────────────────┘\n\n");

    } else {

      printf("[Subscriber] Mensaje crudo recibido: \"%s\"\n", buffer);
    }
  }

  close(sock);
  return 0;
}
