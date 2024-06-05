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