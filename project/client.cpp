#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <csignal>
#include <fcntl.h>
#include <map>

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

void dumpMessage(packet* serverPacket) {
   for(int i = 0; i < serverPacket->size; i++){
    cout << serverPacket->data[i];
   }
//    cout << " , packet number: " << serverPacket->seq << endl;
   cout.flush();
}

void sendAck(int sockfd, struct sockaddr_in clientAddress, packet ACK) {
    int sent = sendto(sockfd, &ACK, sizeof(ACK), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}


void sendData(int sockfd, struct sockaddr_in serverAddress, char inputBuff[1024], int dataSize, int &clientSeq, int lastSentAck){
    packet clientData;
    clientData.seq = clientSeq;
    clientData.ack = lastSentAck + 1;
    clientData.size = dataSize;
    memcpy(clientData.data, inputBuff, dataSize);
    
    int sent = sendto(sockfd, &clientData, sizeof(clientData), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    if(sent != -1){
        clientSeq++;
        cout << "Sent message" << endl;
        return;
    }
    cout << "didnt send message" << endl;
    
}
void processData(int sockfd, struct sockaddr_in serverAddress,packet* serverPacket, int &lastSentACK,map<uint32_t, packet> &bufferedPackets) {
    // First dump message to console
      if (serverPacket->seq == lastSentACK + 1) {
        dumpMessage(serverPacket);
        lastSentACK++;

        // Process buffered packets in order
        while (bufferedPackets.count(lastSentACK) > 0) {
            packet pkt = bufferedPackets[lastSentACK];
            dumpMessage(&pkt);
            bufferedPackets.erase(lastSentACK);
            lastSentACK++;
        }

        // Send acknowledgment for the new highest ACK received
        // sendAck(sockfd, clientAddress, lastSentACK);
    }
    // Else if packet arrives out of order, buffer the packet
    else if (serverPacket->seq > lastSentACK) {
        bufferedPackets[serverPacket->seq] = *serverPacket;
    }
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

    // Flags to make read call non-blocking
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

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

    // Buffer for server 
    char server_buf[sizeof(packet) + MSS];
    socklen_t serverSize;

    // BUffer for stdin
    int BUF_SIZE = 1024;
    char inputBuff[BUF_SIZE];

    // Interrupt signal handler
    signal(SIGINT, sig);

    // Current packet number to send
    int clientSeq = 1;
    // Last ack number this server has sent over;
    int lastSentACK = 0;
    // Last ack number the client sent over
    int lastRecAck = 0;

    
    // Used for packet buffering
    map<uint32_t, packet> bufferedPackets;

    while (status == 1) {

        // Check if there is any read data from stdin
        int messageBytes = read(STDIN_FILENO, inputBuff, BUF_SIZE);

        // If there is data written to stdin, format a packet
        if(messageBytes > 0){
            inputBuff[messageBytes] = '\0';
            cout << "Rec data from stdin" << endl;
            // Send Packet with stdin data
            sendData(sockfd, serverAddress, inputBuff, messageBytes, clientSeq, lastSentACK);
            // clientSeq++;

        }
    
        int bytes_recvd = recvfrom(sockfd, server_buf, BUF_SIZE, 0, (struct sockaddr*)&serverAddress, &serverSize);

        if (bytes_recvd > 0) {
            cout << "Rec data from server" << endl;
            // Format server buffer into a packet
            packet* serverPacket = (packet*) &server_buf;
            processData(sockfd, serverAddress, serverPacket, lastSentACK, bufferedPackets);
        } 


    }

    cleanup(sockfd);
    cout << "Client terminated gracefully" << endl;
    return 0;
}
