
// #include <iostream>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <errno.h>
// #include <csignal>
// #include <cstring>
// #include <fcntl.h>
// #include <map>

// #define PAYLOAD_SIZE 1024
// #define HEADER_SIZE 12
// #define PACKET_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
// #define WINDOW_SIZE 20
// #define MSS 1024

// using namespace std;
// volatile sig_atomic_t status = 1;

// // Struct for packet
// typedef struct __attribute__((__packed__)) {
//     uint32_t seq;
//     uint32_t ack;
//     uint16_t size;
//     uint8_t padding[2];
//     uint8_t data[MSS];
// } packet;

// // Convert packet fields to network byte order
// void packet_to_network_order(packet* pkt) {
//     pkt->seq = htonl(pkt->seq);
//     pkt->ack = htonl(pkt->ack);
//     pkt->size = htons(pkt->size);
// }

// // Convert packet fields to host byte order
// void packet_to_host_order(packet* pkt) {
//     pkt->seq = ntohl(pkt->seq);
//     pkt->ack = ntohl(pkt->ack);
//     pkt->size = ntohs(pkt->size);
// }

// // Cleanup function
// void cleanup(int sockfd) {
//     close(sockfd);
//     // cout.flush();
// }

// // Function to dump received data
// void dumpMessage(packet* clientPacket) {
//     if (clientPacket->seq == 0) {
//         return;
//     }
//     write(STDOUT_FILENO, clientPacket->data, clientPacket->size);
// }

// // Function to send acknowledgment
// void sendAck(int sockfd, struct sockaddr_in clientAddress, int &lastSentAck) {
//     packet ACK = {0};
//     ACK.seq = 0; // Sequence number for ACK packet is set to 0
//     ACK.ack = htonl(lastSentAck); // Convert ack to network byte order
//     int sent = sendto(sockfd, &ACK, HEADER_SIZE, 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
//     if (sent == -1) {
//         perror("sendto");
//     }
// }

// // Function to send data
// void sendData(int sockfd, struct sockaddr_in clientAddress, char inputBuff[1024], int dataSize, int &serverSeq, int lastSentAck) {
//     packet serverData = {0};
//     serverData.seq = htonl(serverSeq); // Convert seq to network byte order
//     serverData.ack = htonl(lastSentAck); // Convert ack to network byte order
//     serverData.size = htons(dataSize); // Convert size to network byte order
//     memcpy(serverData.data, inputBuff, dataSize);

//     // cout << "Client sin family value is: " << clientAddress.sin_family << endl;

//     int sent = sendto(sockfd, &serverData, HEADER_SIZE + dataSize, 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
//     if (sent != -1) {
//         serverSeq++;
//     } 
// }

// // Function to process received data
// void processData(int sockfd, struct sockaddr_in clientAddress, packet* clientPacket, int &lastSentACK, int &lastRecAck, map<uint32_t, packet> &bufferedPackets) {



//     packet_to_host_order(clientPacket);
//     // write(STDOUT_FILENO, clientPacket->data, clientPacket->size);
//     // cout << clientPacket->size + HEADER_SIZE << endl;
//     // return;
    

//     if (clientPacket->seq == 0) {
//         lastRecAck = clientPacket->ack;
//         // cout << "RECV 0 ACK " <<  clientPacket->ack << endl;
//         return;
//     }
    
//     if (clientPacket->seq == lastSentACK + 1) {
//         // cout << "RECV " << clientPacket->seq << " ACK " << clientPacket->ack << endl;
//         dumpMessage(clientPacket);
//         // write(STDOUT_FILENO, clientPacket->data, clientPacket->size);
//         lastSentACK++;
        

//         // while (bufferedPackets.count(lastSentACK) > 0) {
//         //     packet pkt = bufferedPackets[lastSentACK];
//         //     dumpMessage(&pkt);
//         //     bufferedPackets.erase(lastSentACK);
//         //     lastSentACK++;
//         // }
//     } 
//     // else if (clientPacket->seq > lastSentACK) {
//     //     bufferedPackets[clientPacket->seq] = *clientPacket;
//     // }

//     sendAck(sockfd, clientAddress, lastSentACK);
// }

// int sockfd;

// // Signal handler for cleanup
// void sig(int signal) {
//     status = 0;
//     close(STDIN_FILENO);
//     cleanup(sockfd);
//     exit(0);
// }


// int main(int argc, char *argv[]) {
//     if (argc != 5) {
//         // cerr << "Not enough commands" << endl;
//         return -1;
//     }

//     int securityFlag = stoi(argv[1]);
//     string privKeyFile = argv[3];
//     string certFile = argv[4];

//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         // perror("socket");
//         exit(errno);
//     }

//     int flags = fcntl(sockfd, F_GETFL);
//     flags |= O_NONBLOCK;
//     fcntl(sockfd, F_SETFL, flags);

//     struct sockaddr_in serverAddress;
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_addr.s_addr = INADDR_ANY;

//     int PORT = stoi(argv[2]);
//     serverAddress.sin_port = htons(PORT);

//     const int trueFlag = 1;
//     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) == -1) {
//         // perror("setsockopt");
//         cleanup(sockfd);
//         return errno;
//     }

//     if (bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
//         // perror("bind");
//         cleanup(sockfd);
//         return errno;
//     }

//     char client_buf[sizeof(packet)];
//     struct sockaddr_in clientAddress;
//     socklen_t clientSize = sizeof(clientAddress);

//     // char inputBuff[MSS];

//     signal(SIGINT, sig);

//     int serverSeq = 1;
//     int lastSentACK = 0;
//     int lastRecAck = 0;

//     map<uint32_t, packet> bufferedPackets;

//     // Make stdin non-blocking
//     int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
//     fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

//     while (status == 1) {

//         int BUF_SIZE = 1024;
//         char inputBuff[BUF_SIZE];

//         int messageBytes = read(STDIN_FILENO, inputBuff, BUF_SIZE);

//         if (messageBytes > 0) {
    
//             sendData(sockfd, clientAddress, inputBuff, messageBytes, serverSeq, lastSentACK);
//         }

//         // cout << sizeof(client_buf)<< endl;
//         int numBytesRecv = recvfrom(sockfd, &client_buf, 1036, 0, (struct sockaddr*)&clientAddress, &clientSize);

//         if (numBytesRecv > 0) {
//             packet* clientPacket = (packet*)&client_buf;
//             processData(sockfd, clientAddress, clientPacket, lastSentACK, lastRecAck, bufferedPackets);
//         } 
//     }

//     cleanup(sockfd);
//     return 0;
// }




/***********/
/***********/
/***********/


// server.cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
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

int sockfd;
volatile sig_atomic_t running = 1;

void handle_signal(int) {
    running = 0;
    close(sockfd);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: server <flag> <port> <private_key_file> <certificate_file>" << endl;
        return 1;
    }

    int security_flag = stoi(argv[1]);
    int port = stoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr = {0}, client_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    signal(SIGINT, handle_signal);

    uint32_t next_seq = 1, last_ack = 0;
    map<uint32_t, Packet> incoming_packets;
int connected = 0;
    char buf[PACKET_SIZE];
    int buf_size = 0;

    while (running) {
        int n = read(STDIN_FILENO, buf + buf_size, MSS - buf_size);
        if (n > 0) {
            buf_size += n;
        }

        while (buf_size > 0 && connected) {
            Packet pkt = {0};
            pkt.seq = next_seq++;
            pkt.ack = last_ack;
            pkt.size = min(buf_size, MSS);
            memcpy(pkt.data, buf, pkt.size);
            packet_to_network_order(&pkt);
            sendto(sockfd, &pkt, HEADER_SIZE + pkt.size, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

            memmove(buf, buf + pkt.size, buf_size - pkt.size);
            buf_size -= pkt.size;
        }

        socklen_t addrlen = sizeof(client_addr);
        n = recvfrom(sockfd, buf, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &addrlen);

        if (n > 0 && connected) {
            connected = 1;
            Packet* pkt = (Packet*)buf;
            packet_to_host_order(pkt);

            if (pkt->seq == 0) {  // ACK packet
                uint32_t ack = pkt->ack;
                auto it = incoming_packets.begin();
                while (it != incoming_packets.end() && it->first <= ack) {
                    it = incoming_packets.erase(it);
                }
            } else {
                incoming_packets[pkt->seq] = *pkt;
                send_ack(sockfd, (struct sockaddr*)&client_addr, addrlen, pkt->seq);

                while (incoming_packets.count(last_ack + 1) > 0) {
                    last_ack++;
                    const auto& pkt = incoming_packets[last_ack];
                    write(STDOUT_FILENO, pkt.data, pkt.size);
                    incoming_packets.erase(last_ack);
                }
            }
        }
    }

    return 0;
}