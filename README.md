# Laboratorio de Redes: Sistema de Publicación/Suscripción (TCP, UDP y QUIC)

## 👥 Autores
- Carla González - 202411176
- Valeria Martinez - 202410228
- Jose Luis Parra - 202410271

## 📌 Descripción del Proyecto
Este proyecto implementa un sistema distribuido basado en el patrón Arquitectónico de Publicación/Suscripción (Pub/Sub) utilizando sockets en C. 
El sistema cuenta con un **Broker** central que enruta los mensajes, **Publicadores** que envían actualizaciones de partidos de fútbol y **Suscriptores** que se conectan para recibir dichas actualizaciones en tiempo real.

Se incluyen tres versiones completas del sistema:
1. **Versión TCP:** Comunicación orientada a la conexión (garantiza la entrega).
2. **Versión UDP:** Comunicación mediante datagramas (sin conexión).
3. **Versión QUIC:** Protocolo personalizado sobre UDP con confirmación de entrega (stop-and-wait) y manejo de retransmisiones por timeout.

## 🛠️ Requisitos del Sistema
- Entorno Linux (Ubuntu, Debian, WSL, etc.)
- Compilador GCC instalado (`sudo apt install build-essential`)
- Wireshark (para la captura de tráfico en la interfaz `Loopback`)

## ⚙️ Compilación
Para compilar los archivos fuente, abra una terminal en el directorio del proyecto y ejecute los siguientes comandos:

**Para la versión TCP:**
gcc broker_tcp.c -o broker_tcp
gcc subscriber_tcp.c -o sub_tcp
gcc publisher_tcp.c -o pub_tcp

**Para la versión UDP:**
gcc broker_udp.c -o broker_udp
gcc subscriber_udp.c -o sub_ud

**Para la versión QUIC:**
gcc broker_quic.c -o broker_quic
gcc subscriber_quic.c -o sub_quic
gcc publisher_quic.c -o pub_quicp
gcc publisher_udp.c -o pub_udp

## 🚀 Instrucciones de Ejecución
Para que el sistema funcione correctamente, los procesos deben iniciarse en un orden específico en **terminales separadas**.

### Ejecución TCP:
1. **Terminal 1 (Broker):** `./broker_tcp`
   *(Inicia el servidor en el puerto 8080 y espera conexiones).*
2. **Terminal 2 (Suscriptor):** `./sub_tcp`
   *(Se conecta al broker y envía el mensaje de suscripción `SUB|topic`).*
3. **Terminal 3 (Publicador):** `./pub_tcp`
   *(Se conecta e inicia la transmisión de 

### Ejecución QUIC (Protocolo Personalizado):
1. **Terminal 1 (Broker):** `./broker_quic`
   *(Inicia el servidor en el puerto 8080 esperando suscriptores y publicadores).*
2. **Terminal 2 (Suscriptor):** `./sub_quic`
   *(Se conecta al broker y envía solicitud de suscripción (PACKET_SUB), espera mensajes y envía ACK por cada paquete recibido).*
3. **Terminal 3 (Publicador):** `./pub_quic`
   *(Se conecta e inicia la transmisión de 10 eventos, esperando confirmación (ACK) de cada paquete antes de pasar al siguiente).*

**Nota sobre QUIC:** A diferencia de UDP simple, QUIC implementa un mecanismo de confirmación de entrega **stop-and-wait**: el publicador retransmite hasta recibir ACK del broker, y el broker retransmite al suscriptor hasta recibir ACK del mismo. Las retransmisiones se gatillan por timeout (2 segundos).10 mensajes con el formato `PUB|topic|mensaje`).*

### Ejecución UDP:
1. **Terminal 1 (Broker):** `./broker_udp`
2. **Terminal 2 (Suscriptor):** `./sub_udp`
3. **Terminal 3 (Publicador):** `./pub_udp`

## 📊 Capturas de Red (Wireshark)
Junto a este código fuente se adjuntan los archivos de captura `.pcapng` solicitados en la guía:
- `tcp_pubsub.pcapng`: Muestra el Handshake de 3 vías de TCP, la transferencia de mensajes de la aplicación y el cierre de la conexión.
- `udp_pubsub.pcapng`: Muestra el intercambio de datagramas UDP independientes sin establecimiento de conexión.
(Nota: Las capturas se realizaron sobre la interfaz de red local lo - 127.0.0.1).

## 📚 Librerías y Dependencias Externas utilizadas

Para garantizar la máxima compatibilidad y rendimiento, la implementación de este proyecto se realizó de forma **nativa**, por lo que **no se utilizaron librerías de terceros, frameworks, ni dependencias externas** (tales como ZeroMQ, RabbitMQ o MQTT brokers prefabricados). 

Todo el desarrollo se apoyó estrictamente en la **Biblioteca Estándar de C (libc)** y en la **API de Sockets de POSIX** (Portable Operating System Interface) propia de los sistemas Unix/Linux.

A continuación se detalla el uso de cada archivo de cabecera (`header`) importado en el código fuente:

### 1. API de Redes y Sockets (POSIX)
* **`<sys/socket.h>`**: Es el núcleo de la comunicación. Provee las estructuras y funciones fundamentales para crear los puntos de conexión.
  * *Funciones utilizadas:* `socket()` (para inicializar descriptores), `bind()` (para asignar el puerto 8080 al broker), `listen()` y `accept()` (para el manejo de conexiones TCP), y las primitivas de transmisión `send()` / `recv()` (TCP) y `sendto()` / `recvfrom()` (UDP).
* **`<arpa/inet.h>` y `<netinet/in.h>`**: Fundamentales para el manejo de direcciones de Internet y la conversión de datos entre el formato del host (tu computadora) y el formato de la red (Network Byte Order).
  * *Estructuras utilizadas:* `struct sockaddr_in` (para almacenar familias de direcciones, IPs y puertos).
  * *Funciones utilizadas:* `htons()` (Host TO Network Short, para alinear los bytes del puerto) y `inet_pton()` (Pointer TO Network, para convertir la IP de texto `"127.0.0.1"` a formato binario de red).
* **`<sys/select.h>`**: Crucial para el diseño del Broker TCP. Permite la **multiplexación de entrada/salida**, haciendo que el servidor sea concurrente sin necesidad de usar múltiples hilos de procesamiento (*multithreading*).
  * *Funciones y Macros utilizadas:* `select()` (para vigilar múltiples sockets simultáneamente), `FD_ZERO()`, `FD_SET()`, y `FD_ISSET()` (para administrar el conjunto de descriptores de archivo activos).

### 2. Interacción con el Sistema Operativo
* **`<unistd.h>`**: Provee acceso a la API del sistema operativo para manipular archivos y tiempos.
  * *Funciones utilizadas:* `close()` (para liberar correctamente los descriptores de socket cuando un cliente se desconecta y evitar fugas de memoria) y `sleep()` (utilizada en el Publicador para simular intervalos de tiempo real entre los mensajes del partido).

### 3. Biblioteca Estándar de C (Manejo de Memoria y Cadenas)
* **`<string.h>`**: Indispensable para el enrutamiento de los mensajes y el protocolo de aplicación inventado (`SUB|topic` y `PUB|topic|mensaje`).
  * *Funciones utilizadas:* `strtok()` (para dividir los mensajes entrantes usando el delimitador `|`), `strcmp()` (para validar los tipos de mensajes y hacer coincidir los temas del Pub/Sub), `memset()` (para limpiar los buffers antes de cada lectura) y `strcpy()` / `strncpy()` (para hacer copias seguras de los mensajes).
* **`<stdio.h>`**: Utilizada para la interacción por consola (Standard Input/Output) y la construcción dinámica de strings.
  * *Funciones utilizadas:* `printf()` (para imprimir los logs del sistema), `perror()` (para imprimir mensajes descriptivos de error proveídos por el sistema operativo cuando una función de red falla), y `sprintf()` (para empaquetar variables dentro de un string antes de enviarlo por el socket).
* **`<stdlib.h>`**: Para utilidades generales del sistema y finalización de procesos, como `exit()` en caso de fallos críticos en la creación de los sockets.

## 📚 Librerías y Dependencias Externas utilizadas para la implementación de QUIC

La versión QUIC implementa un **protocolo personalizado sobre UDP** que simula características de confiabilidad similares a TCP, pero manteniendo la característica sin conexión de UDP. Utiliza los mismos headers de POSIX, pero con un enfoque diferente:

### 1. API de Redes y Sockets (POSIX) - Específico para QUIC

* **`<sys/socket.h>`**: Igual que TCP/UDP, pero usado exclusivamente para UDP (SOCK_DGRAM).
  * *Funciones utilizadas:* `socket()` (crear socket UDP), `bind()` (en broker_quic.c para escuchar), `sendto()` / `recvfrom()` (primitivas UDP de transmisión).
  * **Diferencia clave:** No se usa `listen()` ni `accept()` (no hay conexión). Todos los mensajes son datagramas independientes.

* **`<netinet/in.h>` y `<arpa/inet.h>`**: Idénticas a TCP/UDP.
  * *Estructuras y funciones:* `struct sockaddr_in`, `htons()`, `inet_addr()` para configurar direcciones.

* **`<sys/time.h>`**: **EXCLUSIVA DE QUIC** para implementar timeouts de retransmisión.
  * *Estructura utilizada:* `struct timeval` (para especificar segundos y microsegundos).
  * *Funciones utilizadas:* `setsockopt()` con `SO_RCVTIMEO` (Receive Timeout) para que `recvfrom()` expire después de `TIMEOUT_SEC` segundos. Si vence el timeout, la función retorna -1 y se retransmite automáticamente.
  * **Importancia:** Este es el mecanismo central de confiabilidad en QUIC. Permite implementar "stop-and-wait" sin usar threads o callbacks: el publicador simplemente espera a que venza el timeout, imprime un mensaje de retransmisión y reenvía el paquete.

### 2. Estructura de Paquete Personalizado - **EXCLUSIVA DE QUIC**

No está en ninguna librería estándar, se define en el código:
```c
typedef struct {
    int type;                    // PACKET_DATA, PACKET_ACK, or PACKET_SUB
    int seq_num;                 // Número de secuencia (0, 1, 2, ...)
    char payload[MAX_PAYLOAD];   // Datos del evento (publicador) o vacío (ACK/SUB)
} quic_packet;
```

**Propósito:**
- **`type`**: Identifica el tipo de mensaje (diferente a TCP/UDP que envían strings como "SUB|topic" o "PUB|topic|mensaje").
- **`seq_num`**: Asegura que los paquetes lleguen en orden y permite detectar duplicados.
- **`payload`**: Transporta el contenido del evento.

### 3. Interacción con el Sistema Operativo

* **`<unistd.h>`**: Igual que TCP/UDP.
  * *Funciones:* `close()` (cerrar sockets), `sleep()` (simular paso del tiempo entre eventos).

### 4. Biblioteca Estándar de C

* **`<string.h>`**: Igual que TCP/UDP.
  * *Funciones:* `memset()` (inicializar estructura `broker_addr` y `subscriber_addr`), `strcpy()` (copiar payload).

* **`<stdio.h>`**: Igual que TCP/UDP.
  * *Funciones:* `printf()` para logs del sistema.

* **`<stdlib.h>`**: Similar a TCP/UDP.
  * *Funciones:* `exit()` en caso de errores críticos.




