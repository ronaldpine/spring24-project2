#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h> // Used for non-blocking

using namespace std;

void cleanup(int sockfd){
  close(sockfd);
  return;
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
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Set receiving port and set byte ordering to Big Endian
  int PORT = 8080;
  serverAddress.sin_port = htons(PORT); 


  // Bind the socket to the os with server address
  int binded = bind(sockfd, (struct sockaddr*) &serverAddress, 
                    sizeof(serverAddress));

  // If there was an error binding
  if (binded < 0){
    cleanup(sockfd);
    return errno;
  } 

  // Create buffer for messages from clients
  int BUF_SIZE = 1024;
  char client_buf[BUF_SIZE];
  struct sockaddr_in clientAddress;
  socklen_t clientSize = sizeof(clientAddress);



  // Listen to message from clients in non-blocking manner

  while(true){
  // Check for data from client
  int bytes_recvd = recvfrom(sockfd, client_buf, BUF_SIZE,
	                           0, (struct sockaddr*) &clientAddress, 
	                           &clientSize);

  // If there is data process, else continue
  if (bytes_recvd <= 0) continue;
  // Else process the data 
  processData();

  // Check if there need to retransmit
  retransmit();

  






  }




  return 0;
}
