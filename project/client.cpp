#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <csignal>
#include <fcntl.h>

using namespace std;
volatile sig_atomic_t status = 1;

#define PAYLOAD_SIZE 1024
#define HEADER_SIZE 12
#define PACKET_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
#define WINDOW_SIZE 20
#define MSS 1024


typedef struct __attribute__((__packed__)) {
    unsigned int seq;
    unsigned int ack;
    unsigned short size;
    char padding[2];
    char data[MSS];
} packet;


void cleanup(int sockfd) {
    close(sockfd);
}

int sendData(int sockfd, string message, struct sockaddr_in serverAddress) {
    char client_buf[1024];
    strcpy(client_buf, message.c_str());
    int did_send = sendto(sockfd, client_buf, strlen(client_buf), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (did_send < 0) {
        perror("sendto");
        return errno;
    }
    return 1;
}

void processData() {
    cout << "will process data" << endl;
}

void retransmit() {
    return;
}

void timeout() {
    exit(3);
}

void sig(int signal) {
    status = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        cout << "not enough arguments for client" << endl;
        return -1;
    }

    int securityFlag = stoi(argv[1]);
    string host = argv[2];
    string pubKey = argv[4];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(errno);
    }

    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    int PORT = stoi(argv[3]);
    serverAddress.sin_port = htons(PORT);

    const int trueFlag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) == -1) {
        perror("setsockopt");
        return errno;
    }

    int dataSent = sendData(sockfd, "Hello world!", serverAddress);
    if (dataSent < 0) {
        perror("sendData");
    }

    int BUF_SIZE = 1024;
    char server_buf[BUF_SIZE];
    socklen_t serverSize = sizeof(serverAddress);

    signal(SIGINT, sig);

    while (status == 1) {
        memset(server_buf, 0, BUF_SIZE);
        int bytes_recvd = recvfrom(sockfd, server_buf, BUF_SIZE, 0, (struct sockaddr*)&serverAddress, &serverSize);

        if (bytes_recvd > 0) {
            write(1, server_buf, bytes_recvd);
            processData();
            retransmit();
        } else if (bytes_recvd < 0 && errno != EAGAIN) {
            perror("recvfrom");
        }

        sleep(1); // Wait before checking again to avoid busy waiting
    }

    cleanup(sockfd);
    cout << "Client terminated gracefully" << endl;
    return 0;
}
