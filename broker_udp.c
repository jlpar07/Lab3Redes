#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PUERTOPUBLICADOR 9001
#define PUERTOSUBSCRIPTOR 9002
#define MAXISUBSCRIPTORES 50
#define MAXITEMA 64
#define TAMANOBUFFER 1024

typedef struct {
  struct sockaddr_in addr;
  char tema[MAXITEMA];
  int activo;
} Subscriptor;
static Subscriptor subscribers[MAXISUBSCRIPTORES];
static int sub_count = 0;

static int crearudpsocket(int puerto);
static void manejo_publicador_mensaje(int pubsock, int subsock);
static void manejo_subscriptor_mensaje(int subsock);
static void registrarsubscriptor(const char *tema, struct sockaddr_in *addr);
static void mensajedistribuido(int sub_sock, const char *topic,
                               const char *message);

int main(void) {
  int pub_sock = crearudpsocket(PUERTOPUBLICADOR);
  printf("broker escuchando publicadores en udp puerto %d\n", PUERTOPUBLICADOR);
  int sub_sock = crearudpsocket(PUERTOSUBSCRIPTOR);
  printf("broker escuchando subsdcriptores en udp puerto %d\n",
         PUERTOSUBSCRIPTOR);
  printf("broker en espera de mensajes...");

  while (1) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pub_sock, &read_fds);
    FD_SET(sub_sock, &read_fds);
    int max_fd = (pub_sock > sub_sock) ? pub_sock : sub_sock;
    int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    if (ready < 0) {
      perror("broker select");
      break;
    }

    if (FD_ISSET(pub_sock, &read_fds)) {
      manejo_publicador_mensaje(pub_sock, sub_sock);
    }

    if (FD_ISSET(sub_sock, &read_fds)) {
      manejo_subscriptor_mensaje(sub_sock);
    }
  }
  close(pub_sock);
  close(sub_sock);
  return 0;
}

static int crearudpsocket(int puerto) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  int opt = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(puerto);
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(sock);
    exit(EXIT_FAILURE);
  }

  return sock;
}

static void manejo_publicador_mensaje(int pub_sock, int sub_sock) {
  char buffer[TAMANOBUFFER];
  struct sockaddr_in sender_addr;
  socklen_t addr_len = sizeof(sender_addr);
  ssize_t bytes = recvfrom(pub_sock, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr *)&sender_addr, &addr_len);
  if (bytes <= 0)
    return;

  buffer[bytes] = '\0';
  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sender_addr.sin_addr, ip_str, sizeof(ip_str));
  printf("broker recibido de publisher %s:%d → \"%s\"\n", ip_str,
         ntohs(sender_addr.sin_port), buffer);

  char prefix[8], topic[MAXITEMA], message[TAMANOBUFFER];
  if (sscanf(buffer, "%7[^|]|%63[^|]|%1023[^\n]", prefix, topic, message) !=
      3) {
    printf("broker formato inválido, ignorando.\n");
    return;
  }
  if (strcmp(prefix, "PUB") != 0) {
    printf("broker prefijo desconocido '%s', ignorando.\n", prefix);
    return;
  }

  mensajedistribuido(sub_sock, topic, message);
}

static void manejo_subscriptor_mensaje(int sub_sock) {
  char buffer[TAMANOBUFFER];
  struct sockaddr_in sender_addr;
  socklen_t addr_len = sizeof(sender_addr);
  ssize_t bytes = recvfrom(sub_sock, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr *)&sender_addr, &addr_len);
  if (bytes <= 0)
    return;
  buffer[bytes] = '\0';
  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sender_addr.sin_addr, ip_str, sizeof(ip_str));
  printf("broker solicitud de suscriptor %s:%d → \"%s\"\n", ip_str,
         ntohs(sender_addr.sin_port), buffer);
  char prefix[8], topic[MAXITEMA];
  if (sscanf(buffer, "%7[^|]|%63s", prefix, topic) != 2) {
    printf("broker formato inválido de suscripción, ignorando.\n");
    return;
  }
  if (strcmp(prefix, "SUB") != 0) {
    printf("broker prefijo desconocido '%s', ignorando.\n", prefix);
    return;
  }

  registrarsubscriptor(topic, &sender_addr);
}

static void registrarsubscriptor(const char *topic, struct sockaddr_in *addr) {

  for (int i = 0; i < sub_count; i++) {
    if (subscribers[i].activo &&
        subscribers[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
        subscribers[i].addr.sin_port == addr->sin_port &&
        strcmp(subscribers[i].tema, topic) == 0) {
      printf("[Broker] Suscriptor ya registrado en tema '%s'.\n", topic);
      return;
    }
  }

  if (sub_count >= MAXISUBSCRIPTORES) {
    printf("[Broker] Tabla llena, no se puede registrar más suscriptores.\n");
    return;
  }

  subscribers[sub_count].addr = *addr;
  strncpy(subscribers[sub_count].tema, topic, MAXITEMA - 1);
  subscribers[sub_count].tema[MAXITEMA - 1] = '\0';
  subscribers[sub_count].activo = 1;
  sub_count++;

  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
  printf("[Broker] Nuevo suscriptor %s:%d registrado en tema '%s'. "
         "Total: %d\n",
         ip_str, ntohs(addr->sin_port), topic, sub_count);
}

static void distribute_message(int sub_sock, const char *topic,
                               const char *message) {
  char out_buf[TAMANOBUFFER];
  snprintf(out_buf, sizeof(out_buf), "MSG|%s|%s", topic, message);

  int sent_count = 0;

  for (int i = 0; i < sub_count; i++) {
    if (!subscribers[i].activo)
      continue;
    if (strcmp(subscribers[i].tema, topic) != 0)
      continue;
    ssize_t sent = sendto(sub_sock, out_buf, strlen(out_buf), 0,
                          (struct sockaddr *)&subscribers[i].addr,
                          sizeof(subscribers[i].addr));
    if (sent < 0) {
      perror("[Broker] sendto");
    } else {
      sent_count++;
    }
  }

  printf("[Broker] Mensaje distribuido a %d suscriptor(es) del tema '%s'.\n",
         sent_count, topic);
}
