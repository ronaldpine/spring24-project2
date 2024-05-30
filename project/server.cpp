#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <csignal> // Used for SIGINT
#include <fcntl.h> // Used for non-blocking

using namespace std;
volatile sig_atomic_t status = 1;

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

void sig(int signal){
  status = 0;
}


int main(int argc, char *argv[])
{

  // Check for valid amount of arguments passed in
  if(argc != 5){
    // Not sure about this return code
    return -1;
  }

  // Parse the arguments from the command typed in
  int securityFlag = stoi(argv[1]);
  int PORT = stoi(argv[2]);
  string privKeyFile = argv[3];
  string certFile = argv[4];

  // Last ACKed packet
  int lastAck = 1; 
  // Last sent packet
  int lastSentPacket = 1;

  signal(SIGINT,sig);


  // Create socket
  int sockfd = socket(AF_INET, SOCK_DGRAM,0);
  
  // If socket wasn't created properly
  if(sockfd < 0){
    exit(errno);
  }

  // Set up flags so the sokcet is nonblocking
  int flags = fcntl(sockfd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(sockfd, F_SETFL, flags);

  // Create the server address with IPV4 and for any connections
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Set receiving port and set byte ordering to Big Endian
  serverAddress.sin_port = htons(PORT); 

  // Be able to reuse the socket address and port
  const int trueFlag = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) == -1){
    // cout << "Flag setting issue" << endl;
    return errno;
  }

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
