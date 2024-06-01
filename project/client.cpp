#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <csignal> // Used for SIGINT
#include <fcntl.h> // Used for non-blocking

using namespace std;
volatile sig_atomic_t status = 1;

// Macros for reliable transport
#define PAYLOAD_SIZE 1024
#define HEADER_SIZE 12
#define PACKET_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
#define WINDOW_SIZE 20

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
  cout << "will process data" << endl;
  return;
}

void retransmit(){
  return;
}

void timeout(){
  exit(3);
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
    cout << "not enough arguments for client" << endl;
    return -1;
  }


   // Parse the arguments from the command typed in
  int securityFlag = stoi(argv[1]);
  string host = argv[2];
  string pubKey = argv[4];


  // Last ACKed packet
  int lastAck = 0; 
  // Last sent packet
  int lastPacketSent = 0;

  // Used to handle SIGINT
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
  serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Set receiving port and set byte ordering to Big Endian
  int PORT = stoi(argv[3]);
  serverAddress.sin_port = htons(PORT); 

  const int trueFlag = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) == -1){
    cout << "Flag setting issue" << endl;
    return errno;
  }

  // send data
  int dataSent = sendData(sockfd, "Hello world!", serverAddress);

  // Create buffer to store messages
  int BUF_SIZE = 1024;
  char server_buf[BUF_SIZE];
  socklen_t serversize;


  // Listen to messages from server
  while(status == 1){
    /* 5. Listen for response from server */
    int bytes_recvd = recvfrom(sockfd, server_buf, BUF_SIZE, 
                        // socket  store data  how much
                           0, (struct sockaddr*) &serverAddress, 
                           &serversize);
    // If there is data process, else continue
    // cout << "No data" << endl;
    if (bytes_recvd > 0){
    
    write(1, server_buf, bytes_recvd);
       // Else process the data 
    processData();

    // Check if there need to retransmit
    retransmit();
    }



    // Print out data
    sleep(0.5);
    
  }

  return 0;
}

