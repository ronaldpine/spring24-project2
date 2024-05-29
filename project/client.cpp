#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h> // Used for non-blocking

using namespace std;

void cleanup(int sockfd){
  close(sockfd);
  return;
}

int sendData(int sockfd, string message, struct sockaddr_in serverAddress){
  char client_buf[1024];
  strcpy(client_buf, message.c_str());
  int did_send = sendto(sockfd, client_buf, strlen(client_buf),
                      0, (struct sockaddr*) &serverAddress,
                      sizeof(serverAddress));
  if (did_send < 0) return errno;
  return 1;
}


void processData(){
  return;
}

void retransmit(){
  return;
}


int main(int argc, char *argv[])
{

  // Last ACKed packet
  int lastAck = 1; 
  // Last sent packet
  int lastSentPacket = 1;


  // Create socket
  int sockfd = socket(AF_INET, SOCK_DGRAM,0);

  // Set up flags so the sokcet is nonblocking
  int flags = fcntl(sockfd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(sockfd, F_SETFL, flags);

  // Create the server address with IPV4 and for any connections
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Set receiving port and set byte ordering to Big Endian
  int PORT = 8080;
  serverAddress.sin_port = htons(PORT); 

  // send data
  int dataSent = sendData(sockfd, "", serverAddress);

  // Create buffer to store messages
  int BUF_SIZE = 1024;
  char server_buf[BUF_SIZE];
  socklen_t serversize;


  // Listen to messages from server

  return 0;
}

