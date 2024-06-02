#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <map>

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
    for (int i = 0; i < clientPacket->size; i++) {
        cout << clientPacket->data[i];
    }
    // cout << " , packet number: " << clientPacket->seq << endl;
    cout.flush();
}

void sendAck(int sockfd, struct sockaddr_in clientAddress, uint32_t ack) {
    packet ACK = {0};
    ACK.ack = ack;
    int sent = sendto(sockfd, &ACK, sizeof(ACK), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}

void sendData(int sockfd, struct sockaddr_in clientAddress, char inputBuff[1024], int dataSize, int &serverSeq, int lastSentAck) {
    packet serverData = {0};
    serverData.seq = serverSeq;
    serverData.ack = lastSentAck + 1;
    serverData.size = dataSize;
    memcpy(serverData.data, inputBuff, dataSize);

    int sent = sendto(sockfd, &serverData, sizeof(serverData), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));

    if(sent != -1){
        serverSeq++;
        cout << "Sent message" << endl;
        return;
    }
    cout << "didnt send message" << endl;
}

void processData(int sockfd, struct sockaddr_in clientAddress, packet* clientPacket, int &lastSentACK, map<uint32_t, packet> &bufferedPackets) {
    // If packet arrives in order, dump and update ack counter
    if (clientPacket->seq == lastSentACK + 1) {
        dumpMessage(clientPacket);
        lastSentACK++;

        // Process buffered packets in order
        while (bufferedPackets.count(lastSentACK) > 0) {
            packet pkt = bufferedPackets[lastSentACK];
            dumpMessage(&pkt);
            bufferedPackets.erase(lastSentACK);
            lastSentACK++;
        }

        // Send acknowledgment for the new highest ACK received
        sendAck(sockfd, clientAddress, lastSentACK);
    }
    // Else if packet arrives out of order, buffer the packet
    else if (clientPacket->seq > lastSentACK) {
        bufferedPackets[clientPacket->seq] = *clientPacket;
    }
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

    // Flags to make read call non-blocking
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

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

    char client_buf[sizeof(packet)];
    struct sockaddr_in clientAddress;
    socklen_t clientSize = sizeof(clientAddress);

    int BUF_SIZE = 1024;
    char inputBuff[BUF_SIZE];

    signal(SIGINT, sig);

    // Current packet number to send
    int serverSeq = 1;
    // Last ack number this server has sent over;
    int lastSentACK = 0;
    // Last ack number the client sent over
    int lastRecAck = 0;

    // Used for packet buffering
    map<uint32_t, packet> bufferedPackets;


    while (status == 1) {
        // Check if there is any read data from stdin
        int messageBytes = read(STDIN_FILENO, inputBuff, BUF_SIZE);

        // If there is data written to stdin, format a packet if we know client address
        if (messageBytes > 0) {
            // Add null character to the message/bytes from stdin
            inputBuff[messageBytes] = '\0';
            cout << "Rec data from stdin" << endl;
            // Send Packet with stdin data
            sendData(sockfd, clientAddress, inputBuff, messageBytes, serverSeq, lastSentACK);
            // serverSeq++;
        }

        // Check if there is data from client
        int numBytesRecv = recvfrom(sockfd, &client_buf, sizeof(client_buf), 0, (struct sockaddr*)&clientAddress, &clientSize);

        if (numBytesRecv > 0) {
            cout << "Rec data from client" << endl;
            // Format the buffer into UDP packet format
            packet* clientPacket = (packet*)&client_buf;
            processData(sockfd, clientAddress, clientPacket, lastSentACK, bufferedPackets);
        }
    }

    cleanup(sockfd);
    return 0;
}
