# Laboratorio de Redes: Sistema de Publicación/Suscripción (TCP y UDP)

## 👥 Autores
Carla González - 202411176
Valeria Martinez -  202410228
Jose Luis Parra - 202410271

## 📌 Descripción del Proyecto
Este proyecto implementa un sistema distribuido basado en el patrón Arquitectónico de Publicación/Suscripción (Pub/Sub) utilizando sockets en C. 
El sistema cuenta con un **Broker** central que enruta los mensajes, **Publicadores** que envían actualizaciones de partidos de fútbol y **Suscriptores** que se conectan para recibir dichas actualizaciones en tiempo real.

Se incluyen dos versiones completas del sistema:
1. **Versión TCP:** Comunicación orientada a la conexión (garantiza la entrega).
2. **Versión UDP:** Comunicación mediante datagramas (sin conexión).

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
gcc subscriber_udp.c -o sub_udp
gcc publisher_udp.c -o pub_udp

## 🚀 Instrucciones de Ejecución
Para que el sistema funcione correctamente, los procesos deben iniciarse en un orden específico en **terminales separadas**.

### Ejecución TCP:
1. **Terminal 1 (Broker):** `./broker_tcp`
   *(Inicia el servidor en el puerto 8080 y espera conexiones).*
2. **Terminal 2 (Suscriptor):** `./sub_tcp`
   *(Se conecta al broker y envía el mensaje de suscripción `SUB|topic`).*
3. **Terminal 3 (Publicador):** `./pub_tcp`
   *(Se conecta e inicia la transmisión de 10 mensajes con el formato `PUB|topic|mensaje`).*

### Ejecución UDP:
1. **Terminal 1 (Broker):** `./broker_udp`
2. **Terminal 2 (Suscriptor):** `./sub_udp`
3. **Terminal 3 (Publicador):** `./pub_udp`

## 📊 Capturas de Red (Wireshark)
Junto a este código fuente se adjuntan los archivos de captura `.pcapng` solicitados en la guía:
- `tcp_pubsub.pcapng`: Muestra el Handshake de 3 vías de TCP, la transferencia de mensajes de la aplicación y el cierre de la conexión.
- `udp_pubsub.pcapng`: Muestra el intercambio de datagramas UDP independientes sin establecimiento de conexión.
*(Nota: Las capturas se realizaron sobre la interfaz de red local `lo` - 127.0.0.1).*

## 📚 Librerías y Dependencias Externas utilizadas para la implementación de TCP

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

