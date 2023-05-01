#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7 //select minimal legacy support, needed for inet_pton, inet_ntop
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
#else
#include <sys/socket.h> //for sockaddr, socket, socket
#include <sys/types.h> //for size_t
#include <netdb.h> //for getaddrinfo
#include <netinet/in.h> //for sockaddr_in
#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
#include <errno.h> //for errno
#include <stdio.h> //for fprintf, perror
#include <unistd.h> //for close
#include <stdlib.h> //for exit
#include <string.h> //for memset
#endif
#define PORT 12345 // define port to use

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    int n, i, max_val, max_val_recv;

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("Error opening socket");
    }

    // clear out memory for server address
    memset(&server_addr, 0, sizeof(server_addr));

    // configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // bind socket to server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("Error binding socket");
    }

    // receive "GO" from client
    memset(buffer, 0, sizeof(buffer));
    if ((n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        error("Error receiving from client");
    }
    printf("Received from client: %s\n", buffer);

    // send 42 random numbers to client
    srand(time(NULL)); // seed random number generator
    for (i = 0, max_val = 0; i < 42; i++) {
        int rand_num = rand();
        if (rand_num > max_val) max_val = rand_num;
        int net_order = htonl(rand_num); // convert to network byte order
        if (sendto(sockfd, &net_order, sizeof(net_order), 0, (struct sockaddr *)&client_addr, client_len) < 0) {
            error("Error sending to client");
        }
    }

    // receive largest value from client
    max_val_recv = 0;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if ((n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            error("Error receiving from client");
        }
        int recv_num = ntohl(*(int *)buffer); // convert from network byte order
        if (recv_num > max_val_recv) {
            max_val_recv = recv_num;
            // send back max value
            if (sendto(sockfd, &max_val_recv, sizeof(max_val_recv), 0, (struct sockaddr *)&client_addr, client_len) < 0) {
                error("Error sending to client");
            }
        }
        // check for timeout
        sleep(1);
        if (sendto(sockfd, &max_val_recv, sizeof(max_val_recv), 0, (struct sockaddr *)&client_addr, client_len) < 0) {
            error("Error sending to client");
        }
        if (recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&client_addr, &client_len) < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // no data