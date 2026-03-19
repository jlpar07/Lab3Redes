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

static int crearudpsocket(int puerto);
static void manejo_publicador_mensaje(int pubsock, int subsock);
static void manejo_subscriptor_mensaje(int subsock);
static void registrarsubscriptor(const char *tema, struct sockaddr_in *addr);
static void mensajedistribuido(int subsock, const char *tema,
                               const char *mensaje);
