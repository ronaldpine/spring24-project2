// #include <iostream>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <string.h>
// #include <unistd.h>
// #include <errno.h>
// #include <csignal>
// #include <fcntl.h>
// #include <map>

// using namespace std;
// volatile sig_atomic_t status = 1;

// #define PAYLOAD_SIZE 1024
// #define HEADER_SIZE 12
// #define PACKET_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
// #define WINDOW_SIZE 20
// #define MSS 1024

// typedef struct __attribute__((__packed__)) {
//     uint32_t seq;
//     uint32_t ack;
//     uint16_t size;
//     uint8_t padding[2];
//     uint8_t data[MSS];
// } packet;

// void packet_to_network_order(packet* pkt) {
//     pkt->seq = htonl(pkt->seq);
//     pkt->ack = htonl(pkt->ack);
//     pkt->size = htons(pkt->size);
// }

// void packet_to_host_order(packet* pkt) {
//     pkt->seq = ntohl(pkt->seq);
//     pkt->ack = ntohl(pkt->ack);
//     pkt->size = ntohs(pkt->size);
// }

// void cleanup(int sockfd) {
//     close(sockfd);
//     // cout.flush();
// }


// void dumpMessage(packet* serverPacket) {
//     if (serverPacket->seq == 0) {
//         return;
//     }
//     write(STDOUT_FILENO, serverPacket->data, serverPacket->size);
// }

// void sendAck(int sockfd, struct sockaddr_in serverAddress, int &lastSentAck) {
//     packet ACK = {0};
//     ACK.seq = 0; // Sequence number for ACK packet is set to 0
//     ACK.ack = htonl(lastSentAck); // Convert ack to network byte order
//     int sent = sendto(sockfd, &ACK, HEADER_SIZE, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
//     if (sent != -1) {
//         // cout << "Sent ack for: " << lastSentAck << endl;
//     }
// }

// void sendData(int sockfd, struct sockaddr_in serverAddress, char inputBuff[1024], int dataSize, int &clientSeq, int lastSentAck) {
//     // cout << dataSize << endl;
//     packet clientData = {0};
//     clientData.seq = htonl(clientSeq); // Convert seq to network byte order
//     clientData.ack = htonl(lastSentAck); // Convert ack to network byte order
//     clientData.size = htons(dataSize); // Convert size to network byte order
//     memcpy(clientData.data, inputBuff, dataSize);
//     // cout << clientData.size << endl;
//     // cout << "size of server data " << HEADER_SIZE + dataSize<< endl;
//     // cout << "yes" << endl;
//     // write(STDOUT_FILENO, inputBuff, dataSize);
//     // write(STDOUT_FILENO, clientData.data, dataSize);

//     // cout << "Packet Size: "<< HEADER_SIZE + dataSize << endl;
//     // cout << dataSize << "," << clientData.size << endl;
//     // exit(1);

//     // cout << HEADER_SIZE + dataSize << endl;

//     int sent = sendto(sockfd, &clientData, HEADER_SIZE + dataSize, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

//     if (sent != -1) {
//         clientSeq++;
//     }
// }

// void processData(int sockfd, struct sockaddr_in serverAddress, packet* serverPacket, int &lastSentACK, int &lastRecAck, map<uint32_t, packet> &bufferedPackets) {
//     packet_to_host_order(serverPacket);
//     // cout << "Received packet with seq: " << serverPacket->seq << " and ack: " << serverPacket->ack << endl;
    
//     if (serverPacket->seq == 0) {
//         lastRecAck = serverPacket->ack;
//         // cout << "RECV 0- ACK " <<  serverPacket->ack << endl;
//         return;
//     }
    
//     if (serverPacket->seq == lastSentACK + 1) {
//         dumpMessage(serverPacket);
//         // cout << "RECV " << serverPacket->seq << " ACK " << serverPacket->ack << endl;
//         lastSentACK++;

//         // while (bufferedPackets.count(lastSentACK) > 0) {
//         //     packet pkt = bufferedPackets[lastSentACK];
//         //     dumpMessage(&pkt);
//         //     bufferedPackets.erase(lastSentACK);
//         //     lastSentACK++;
//         // }

//     } 
//     // else if (serverPacket->seq > lastSentACK) {
//     //     bufferedPackets[serverPacket->seq] = *serverPacket;
//     // }

//     sendAck(sockfd, serverAddress, lastSentACK);
// }

// int sockfd;

// void sig(int signal) {
//     status = 0;
//     close(STDIN_FILENO);
//     cleanup(sockfd);
//     exit(0);
// }

// int main(int argc, char *argv[]) {
//     if (argc != 5) {
//         // cout << "Not enough arguments for client" << endl;
//         return -1;
//     }

//     int securityFlag = stoi(argv[1]);
//     string host = argv[2];
//     string pubKey = argv[4];

//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket");
//         exit(errno);
//     }

//     int flags = fcntl(sockfd, F_GETFL);
//     flags |= O_NONBLOCK;
//     fcntl(sockfd, F_SETFL, flags);

//     int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
//     fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

//     struct sockaddr_in serverAddress;
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

//     int PORT = stoi(argv[3]);
//     serverAddress.sin_port = htons(PORT);

//     const int trueFlag = 1;
//     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) == -1) {
//         perror("setsockopt");
//         return errno;
//     }

//     char server_buf[sizeof(packet)];
//     socklen_t serverSize;

//     int BUF_SIZE = 1024;
//     char inputBuff[BUF_SIZE];

//     signal(SIGINT, sig);

//     int clientSeq = 1;
//     int lastSentACK = 0;
//     int lastRecAck = 0;

//     map<uint32_t, packet> bufferedPackets;

//     // cout << sizeof(server_buf) << endl;

//     while (status == 1) {

//         int messageBytes = read(STDIN_FILENO, inputBuff, BUF_SIZE);

//         if (messageBytes > 0) {
//             // cout << messageBytes << endl;
//             // cout << "Message bytes: "<< messageBytes << endl;
//             // write(STDOUT_FILENO, messageBytes, sizeof(messageBytes));
//             sendData(sockfd, serverAddress, inputBuff, messageBytes, clientSeq, lastSentACK);
//         }
//         // exit(1);

//         int bytes_recvd = recvfrom(sockfd, &server_buf, 1036, 0, (struct sockaddr*)&serverAddress, &serverSize);
    
//         if (bytes_recvd > 0) {
//             // cout << "bytes_recvd: "<< bytes_recvd << endl;
//             // exit(1);
//             // cout << "bytes_recvd" << endl;
//             packet* serverPacket = (packet*)&server_buf;
//             processData(sockfd, serverAddress, serverPacket, lastSentACK, lastRecAck, bufferedPackets);
//         }
//     }

//     cleanup(sockfd);
//     return 0;
// }



// client.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <map>
#include <arpa/inet.h>
#include <cstring>

#define MSS 1024
#define HEADER_SIZE 12
#define PACKET_SIZE (MSS + HEADER_SIZE)
#define WINDOW_SIZE 20
#define RTO 1  // 1 second

struct __attribute__((__packed__)) Packet {
    uint32_t seq;
    uint32_t ack;
    uint16_t size;
    uint8_t padding[2];
    uint8_t data[MSS];
};

void packet_to_network_order(Packet* pkt) {
    pkt->seq = htonl(pkt->seq);
    pkt->ack = htonl(pkt->ack);
    pkt->size = htons(pkt->size);
}

void packet_to_host_order(Packet* pkt) {
    pkt->seq = ntohl(pkt->seq);
    pkt->ack = ntohl(pkt->ack);
    pkt->size = ntohs(pkt->size);
}

void send_ack(int sockfd, struct sockaddr* dest_addr, socklen_t addrlen, uint32_t ack_num) {
    Packet ack_pkt = {0};
    ack_pkt.ack = htonl(ack_num);
    sendto(sockfd, &ack_pkt, HEADER_SIZE, 0, dest_addr, addrlen);
}

using namespace std;

// client.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <map>
// #include "common.h"

using namespace std;

int sockfd;
volatile sig_atomic_t running = 1;

void handle_signal(int) {
    running = 0;
    close(sockfd);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: client <flag> <hostname> <port> <ca_public_key_file>" << endl;
        return 1;
    }

    int security_flag = stoi(argv[1]);
    const char* hostname = argv[2];
    int port = stoi(argv[3]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, hostname, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    signal(SIGINT, handle_signal);

    uint32_t next_seq = 1, last_ack = 0;
    map<uint32_t, Packet> outgoing_packets;
    map<uint32_t, Packet> incoming_packets;

    while (running) {
        char buf[PACKET_SIZE];
        int n = read(STDIN_FILENO, buf, MSS);
        if (n > 0) {
            Packet pkt = {0};
            pkt.seq = next_seq++;
            pkt.ack = last_ack;
            pkt.size = n;
            memcpy(pkt.data, buf, n);
            packet_to_network_order(&pkt);
            outgoing_packets[pkt.seq] = pkt;
            sendto(sockfd, &pkt, HEADER_SIZE + n, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        }

        socklen_t addrlen = sizeof(server_addr);
        n = recvfrom(sockfd, buf, PACKET_SIZE, 0, (struct sockaddr*)&server_addr, &addrlen);
        if (n > 0) {
            Packet* pkt = (Packet*)buf;
            packet_to_host_order(pkt);

            if (pkt->seq == 0) {  // ACK packet
                uint32_t ack = pkt->ack;
                for (auto it = outgoing_packets.begin(); it != outgoing_packets.end() && it->first <= ack; it = outgoing_packets.erase(it));
            } else {
                incoming_packets[pkt->seq] = *pkt;
                send_ack(sockfd, (struct sockaddr*)&server_addr, addrlen, pkt->seq);

                while (incoming_packets.count(last_ack + 1) > 0) {
                    last_ack++;
                    write(STDOUT_FILENO, incoming_packets[last_ack].data, incoming_packets[last_ack].size);
                    incoming_packets.erase(last_ack);
                }
            }
        }

        // Handle retransmission (simplified for brevity)
        if (!outgoing_packets.empty()) {
            static time_t last_send = time(NULL);
            if (time(NULL) - last_send >= RTO) {
                auto it = outgoing_packets.begin();
                sendto(sockfd, &it->second, HEADER_SIZE + it->second.size, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                last_send = time(NULL);
            }
        }
    }

    return 0;
}