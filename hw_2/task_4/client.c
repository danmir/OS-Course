#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

void error(char *message) {
    fprintf(stderr, "%s.\n", message);
    exit(1);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in serv_addr;
    struct hostent *server;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        error("Не удалось создать сокет");
    };

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(7891);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Не удалось присоединиться");
    }

    char symb;
    while (read(sock, &symb, 1) > 0) {
        printf("%c", symb);
    };
    close(sock);

    return 0;

};