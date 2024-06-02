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
    uint32_t seq;
    uint32_t ack;
    uint16_t size;
    uint8_t padding[2];
    uint8_t data[MSS];
} packet;



void cleanup(int sockfd) {
    close(sockfd);
}

void dumpMessage(packet* clientPacket) {
   for(int i = 0; i < clientPacket->size; i++){
    cout << clientPacket->data[i];
   }
   cout << endl;
   cout.flush();
}

void sendAck(int sockfd, struct sockaddr_in clientAddress, packet ACK) {
    int sent = sendto(sockfd, &ACK, sizeof(ACK), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}

void sendData(int sockfd, struct sockaddr_in clientAddress, char inputBuff[1024], int dataSize, int serverSeq, int lastRecAck){
    packet serverData;
    serverData.seq = serverSeq;
    serverData.ack = lastRecAck;
    serverData.size = dataSize;
    memcpy(serverData.data, inputBuff, dataSize);
    
    int sent = sendto(sockfd, &serverData, sizeof(serverData), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}

void processData(int sockfd, struct sockaddr_in clientAddress ,packet* clientPacket) {
    // First dump message
    dumpMessage(clientPacket);
    // packet ACK = {0};
    // ACK.ack = clientPacket.seq;
    // // Send ack back
    // int sent = sendto(sockfd, &ACK, sizeof(ACK), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
    
}

void retransmit() {
    return;
}

void sig(int signal) {
    status = 0;
    close(STDIN_FILENO);
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

 
    char client_buf[sizeof(packet) + MSS];
    // packet clientPacket;
    struct sockaddr_in clientAddress;
    socklen_t clientSize = sizeof(clientAddress);

    int BUF_SIZE = 1024;
    char inputBuff[BUF_SIZE];
    signal(SIGINT, sig);

    // Current packet number to send
    int serverSeq = 0;
    // Last ack number this server has sent over;
    int lastSentACK = 0;
    // Last ack number the client sent over
    int lastRecAck = 0;


    while (status == 1) {

        // check if there is any read data from stdin

        int messageBytes = read(STDIN_FILENO, inputBuff, BUF_SIZE);
        // If there is data written to stdin, format a packet
        if(messageBytes > 0){
            inputBuff[messageBytes] = '\0';
            // Send Packet with stdin data
            sendData(sockfd, clientAddress, inputBuff, messageBytes, serverSeq, lastSentACK);
        }


        // memset(client_buf, 0, BUF_SIZE);
         int numBytesRecv = recvfrom(sockfd, &client_buf, sizeof(client_buf), 0, (struct sockaddr*)&clientAddress, &clientSize);

        if (numBytesRecv > 0) {
            continue;
            // Format the buffer into UDP packet format
            packet* clientPacket = (packet*) &client_buf;
            processData(sockfd, clientAddress, clientPacket);
            retransmit();
        } else if (numBytesRecv < 0 && errno != EAGAIN) {
            perror("recvfrom");
        }
    }

    cleanup(sockfd);
    // cout << "Server terminated gracefully" << endl;
    return 0;
}
