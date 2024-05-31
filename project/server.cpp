#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <csignal> // Used for SIGINT
#include <fcntl.h> // Used for non-blocking

// Macros for reliable transport
#define PAYLOAD_SIZE 1024
#define HEADER_SIZE 12
#define PACKET_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
#define WINDOW_SIZE 20

using namespace std;
volatile sig_atomic_t status = 1;


void cleanup(int sockfd){
  close(sockfd);
  return;
}

void dumpMessage( char client_data[1024], int numBytesRecv){
  cout << "In the dump Message function" << endl;
  string message;
  for (int i = 0; i < numBytesRecv; i++) {
        char c = client_data[i];
        message += c;
  }

  cout << message << endl;
  // Flush the output to make sure it appears in the console
  cout.flush();

}

void processData(struct sockaddr_in clientAddress,  char client_data[1024], int numBytesRecv){


  //print out the message
  dumpMessage(client_data, numBytesRecv);




  return;
}

void retransmit(){
  return;
}

void sig(int signal){
  status = 0;
  // flush everything to std out
}


int main(int argc, char *argv[])
{

  // Check for valid amount of arguments passed in
  if(argc != 5){
    // Not sure about this return code
    cout << "Not enough commands " << endl;
    return -1;
  }

  // Parse the arguments from the command typed in
  int securityFlag = stoi(argv[1]);
  cout << "Security flag is: " << securityFlag << endl;
  int PORT = stoi(argv[2]);
  cout << "Port is: " << PORT << endl;
  string privKeyFile = argv[3];
  string certFile = argv[4];

  // Last ACKed packet
  int lastAck = 0; 
  // Current Ack
  int currAck = 0;
  // Last sent packet
  int currPacket = 1;
  //

  // signal(SIGINT,sig);


  // Create socket
  int sockfd = socket(AF_INET, SOCK_DGRAM,0);
  
  // If socket wasn't created properly
  if(sockfd < 0){
    exit(errno);
  }

  // Set up flags so the sokcet is nonblocking
  // int flags = fcntl(sockfd, F_GETFL);
  // flags |= O_NONBLOCK;
  // fcntl(sockfd, F_SETFL, flags);

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



  // Listen to message from clients in non-blocking manner, while running
  while(true){

    // cout << "in the while loop" << endl;
  // Check for data from client
  int numBytesRecv = recvfrom(sockfd, client_buf, BUF_SIZE,
	                           0, (struct sockaddr*) &clientAddress, 
	                           &clientSize);

  // If there is data process, else continue
  if ( numBytesRecv < 0) continue;

    // Inspect client information
  char* client_ip = inet_ntoa(clientAddress.sin_addr);
                // "Network bytes to address string"
  int client_port = ntohs(clientAddress.sin_port); // Little endian

  // Else process the data 
  processData(clientAddress, client_buf, numBytesRecv);

  // Check if there need to retransmit
  retransmit();

  }




  return 0;
}
