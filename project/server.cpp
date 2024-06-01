#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>

#define PAYLOAD_SIZE 1024
#define HEADER_SIZE 12
#define PACKET_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
#define WINDOW_SIZE 20
#define MSS 1024

using namespace std;
volatile sig_atomic_t status = 1;


// Struct for packet
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

void dumpMessage(char client_data[], int numBytesRecv) {
    cout << "In the dumpMessage function" << endl;
    string message(client_data, numBytesRecv);
    cout << "Received message: " << message << endl;
    cout.flush();
}

int sendData(int sockfd, string message, struct sockaddr_in clientAddress) {
    char client_buf[1024];
    strcpy(client_buf, message.c_str());
    int did_send = sendto(sockfd, client_buf, strlen(client_buf), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
    if (did_send < 0) {
        perror("sendto");
        return errno;
    }
    return 1;
}

void processData(int sockfd, struct sockaddr_in clientAddress, char client_data[], int numBytesRecv) {
    dumpMessage(client_data, numBytesRecv);
    cout << "Finished dumping message" << endl;
    cout << "Sending Response" << endl;
    sendData(sockfd, "hello back\n", clientAddress);
}

void retransmit() {
    return;
}

void sig(int signal) {
    status = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        cout << "Not enough commands " << endl;
        return -1;
    }

    int securityFlag = stoi(argv[1]);
    string privKeyFile = argv[3];
    string certFile = argv[4];

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
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int PORT = stoi(argv[2]);
    serverAddress.sin_port = htons(PORT);

    const int trueFlag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) == -1) {
        perror("setsockopt");
        return errno;
    }

    if (bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind");
        cleanup(sockfd);
        return errno;
    }

    int BUF_SIZE = 1024;
    char client_buf[BUF_SIZE];
    struct sockaddr_in clientAddress;
    socklen_t clientSize = sizeof(clientAddress);

    signal(SIGINT, sig);

    while (status == 1) {
        // memset(client_buf, 0, BUF_SIZE);
        int numBytesRecv = recvfrom(sockfd, client_buf, BUF_SIZE, 0, (struct sockaddr*)&clientAddress, &clientSize);

        if (numBytesRecv > 0) {
            processData(sockfd, clientAddress, client_buf, numBytesRecv);
            retransmit();
        } else if (numBytesRecv < 0 && errno != EAGAIN) {
            perror("recvfrom");
        }
    }

    cleanup(sockfd);
    cout << "Server terminated gracefully" << endl;
    return 0;
}
