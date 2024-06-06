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
#define CWND 20
#define RTO 1  // 1 second

int sockfd;
volatile sig_atomic_t running = 1;

using namespace std;

struct __attribute__((__packed__)) Packet {
    uint32_t seq;
    uint32_t ack;
    uint16_t size;
    uint8_t padding[2];
    uint8_t data[MSS];
};

void littleToBigEndian(Packet* pkt) {
    pkt->seq = htonl(pkt->seq);
    pkt->ack = htonl(pkt->ack);
    pkt->size = htons(pkt->size);
}

void bigToLittleEnd(Packet* pkt) {
    pkt->seq = ntohl(pkt->seq);
    pkt->ack = ntohl(pkt->ack);
    pkt->size = ntohs(pkt->size);
}

void send_ack(int sockfd, struct sockaddr* dest_addr, socklen_t addrlen, uint32_t ack_num) {
    Packet pkt = {0};
    pkt.ack = htonl(ack_num);
    sendto(sockfd, &pkt, HEADER_SIZE, 0, dest_addr, addrlen);
}

void handle_signal(int) {
    running = 0;
    close(sockfd);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Incorrect client args" << endl;
        return 1;
    }

    // Parse the port,security flags, and port
    int security = stoi(argv[1]);
    const char* hostname = argv[2];
    int PORT = stoi(argv[3]);

    // Make a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, hostname, &server_addr.sin_addr);
    server_addr.sin_port = htons(PORT);

    // Make stdin and socket non blocking
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
    fcntl(STDIN_FILENO, F_SETFL, flags);
 
    
    signal(SIGINT, handle_signal);

    // Used to keep track of packets
    uint32_t seq = 1;
    uint32_t last_ack = 0;
    map<uint32_t, Packet> outgoing_packets;
    map<uint32_t, Packet> incoming_packets;

    while (running) {
        
        // Buffer for client packets
        char serverBuf[PACKET_SIZE];
        
        // Buffer for std input
        char inputBuf[MSS];


        // read from stdin
        int n = read(STDIN_FILENO, inputBuf, MSS);
        if (n > 0) {
            
            // Format into a packet and send
            Packet pkt = {0};
            pkt.seq = seq;
            pkt.ack = last_ack;
            pkt.size = n;
            memcpy(pkt.data, inputBuf, n);

            // Convert to big endian
            littleToBigEndian(&pkt);

            // Store the packet in case of retransmission
            outgoing_packets[pkt.seq] = pkt;

            // Send the Packet
            sendto(sockfd, &pkt, HEADER_SIZE + n, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            
            // Increase seq
            seq++;
        }

        socklen_t addrlen = sizeof(server_addr);
        n = recvfrom(sockfd, serverBuf, PACKET_SIZE, 0, (struct sockaddr*)&server_addr, &addrlen);
        if (n > 0) {
            Packet* pkt = (Packet*)serverBuf;
            bigToLittleEnd(pkt);

            // If this is purely an ACK packet
            if (pkt->seq == 0) {  
                uint32_t ack = pkt->ack;
                for (auto it = outgoing_packets.begin(); it != outgoing_packets.end() && it->first <= ack; it = outgoing_packets.erase(it));
            } 

            // Else if the packet also contains data
            else {

                // Store the incoming packet if it is out of order
                incoming_packets[pkt->seq] = *pkt;

                //  Send an ack 
                send_ack(sockfd, (struct sockaddr*)&server_addr, addrlen, pkt->seq);

                while (incoming_packets.count(last_ack + 1) > 0) {
                    last_ack++;
                    write(STDOUT_FILENO, incoming_packets[last_ack].data, incoming_packets[last_ack].size);
                    incoming_packets.erase(last_ack);
                }
            }
        }

        // Check for Retransmission
     
    }

    return 0;
}