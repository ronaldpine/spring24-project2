#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <map>
#include <arpa/inet.h>
#include <string.h>

#define MSS 1024
#define HEADER_SIZE 12
#define CWND 20
#define PACKET_SIZE (MSS + HEADER_SIZE)
#define RTO 1  // 1 second

using namespace std;

int sockfd;
volatile sig_atomic_t status = 1;

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

void sendAck(int sockfd, struct sockaddr* dest_addr, socklen_t addrlen, uint32_t ack_num) {
    Packet pkt = {0};
    pkt.ack = htonl(ack_num);
    sendto(sockfd, &pkt, HEADER_SIZE, 0, dest_addr, addrlen);
}

void handle_signal(int) {
    status = 0;
    close(sockfd);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Incorrect arguments for client" << endl;
        return 1;
    }


    // Parse the port and security flage
    int security = stoi(argv[1]);
    int PORT = stoi(argv[2]);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr = {0}, client_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Make stdin and socket non blocking
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
    fcntl(STDIN_FILENO, F_SETFL, flags);

    signal(SIGINT, handle_signal);

    // Used to keep track of packets
    uint32_t seq = 1;
    uint32_t last_ack = 0;
    map<uint32_t, Packet> sentPackets;
    map<uint32_t, Packet> incoming_packets;

    while (status) {
        // Buffer for client packets
        char clientBuf[PACKET_SIZE];
        
        // Buffer for std input
        char inputBuf[MSS];

        // Read from stdin
        int n = read(STDIN_FILENO, inputBuf, MSS);
        if (n > 0 ) {
            
            // Format into a packet and send
            Packet pkt = {0};
            pkt.seq = seq;
            pkt.ack = last_ack;
            pkt.size = n;
            memcpy(pkt.data, inputBuf, n);

            // Convert to big endian
            littleToBigEndian(&pkt);

            // Store the packet in case of retransmission
            sentPackets[pkt.seq] = pkt;

            // Send the Packet
            sendto(sockfd, &pkt, HEADER_SIZE + n, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

            // Increase seq
            seq++;
        }


        // Receive packets from client
        socklen_t addrlen = sizeof(client_addr);
        n = recvfrom(sockfd, clientBuf, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &addrlen);

        if (n > 0) {
            // Format into a packet
            Packet* pkt = (Packet*)clientBuf;
            bigToLittleEnd(pkt);

            // If this is a purely an ACK packet 
            if (pkt->seq == 0) {  

                // Erase packets that have been cumulatively acked to avoid retransmission
                uint32_t eraseUpToAck = pkt->ack;
                for (auto it = sentPackets.begin(); it != sentPackets.end() && it->first <= eraseUpToAck; ){
                    it = sentPackets.erase(it);
                }
               

            } 
            // Testing to see if packets arrive in order
            // else if (pkt->seq == last_ack + 1) {
                
            //     // Write out data, update ack counter, and send ack packet
            //     write(STDOUT_FILENO, pkt->data, pkt->size);
            //     last_ack = pkt->seq;
            //     sendAck(sockfd, (struct sockaddr*)&client_addr, addrlen, last_ack);
            // }
            else {

                // Store the incoming packet if it is out of order
                incoming_packets[pkt->seq] = *pkt;

                //  Send an ack 
                // if()
                sendAck(sockfd, (struct sockaddr*)&client_addr, addrlen, last_ack);

                while (incoming_packets.count(last_ack + 1) > 0) {
                    last_ack++;
                    write(STDOUT_FILENO, incoming_packets[last_ack].data, incoming_packets[last_ack].size);
                    incoming_packets.erase(last_ack);
                }
            }


        }

            auto it = sentPackets.begin();
            sendto(sockfd, &it->second, HEADER_SIZE + it->second.size, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        
    }

    return 0;
}