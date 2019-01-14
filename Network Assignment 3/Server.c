#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MAX_FILE_NAME 512


int timeout_indicator(int FD, int sec)
{
    fd_set rset;
    struct timeval tv;
    FD_ZERO(&rset);
    FD_SET (FD, &rset);
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    
    return (select (FD + 1, &rset, NULL, NULL, &tv));
}

int error_sys(const char* x)
{ 
    perror(x); 
    exit(1); 
}

int main(int argc, char *argv[])
{
    char data[512] = {0};
    int sockfd, newsock_fd;
    struct sockaddr_in client;
    socklen_t clientlength = sizeof(struct sockaddr_in);
    int g, ret;
    int send_res;
    int last_block;
    
    if (argc != 3){
        error_sys ("USAGE: ./server <Server_IP> <Port_Number>");
        return 0;
    }

    char *serverIP = argv[1];
    unsigned short int serverPort = atoi(argv[2]);

    printf("\nServerIP %s serverPort: %d\n\n", serverIP, serverPort );

    int recbyte;
    char buff [1024] = {0};
    char ack_packet[32] = {0};
    char file_payload[516] = {0};
    char file_payload_copy[516] = {0};
    char filename[MAX_FILE_NAME];
    char Mode[512];
    unsigned short int opcode1, opcode2, BlockNo;
    int b, j;
    FILE *fp;
    struct addrinfo hints, *ai, *clientinfo, *p;
    int yes = 1;
    int pid, read_ret;
    int blocknum = 0, timeout_count = 0, NACK = 0;
    char *ephemeral_port;

    ephemeral_port = (char *)malloc (sizeof ephemeral_port);
  
    socklen_t addrlen;
  
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    if ((ret = getaddrinfo(NULL, argv[2], &hints, &ai)) != 0) {
        fprintf(stderr, "SERVER: %s\n", gai_strerror(ret));
        exit(1);
    }
    
	for(p = ai; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            continue;
        }
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "Failed to bind socket\n");
        return 1;
    }
  
    freeaddrinfo(ai);
    printf("SERVER: Ready for association with clients...\n");
    
    while(1) {
        recbyte = recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr*)&client, &clientlength);
        if (recbyte < 0) {
            error_sys("ERROR: Couldn't receive data");
            return 1;
        }
        else {
	}
        memcpy(&opcode1,&buff,2);                    
        opcode1 = ntohs(opcode1);
        pid = fork();
        if (pid == 0) {
            if (opcode1 == 1){    
                bzero(filename, MAX_FILE_NAME);
                for (b = 0; buff[2+b] != '\0'; b++) {
                filename[b]=buff[2+b];
            }	
            filename[b]='\0';
            bzero(Mode, 512);
            g = 0;
            for (j = b+3; buff[j] != '\0'; j++) { 
                Mode[g]=buff[j];
                g++;
            }  	
            Mode[g]='\0';
            printf("RRQ received, filename: '%s' mode: %s\n", filename, Mode);
            fp = fopen (filename, "r");
            if (fp != NULL) {
                close(sockfd);
            *ephemeral_port = htons(0);
            if ((ret = getaddrinfo(NULL, ephemeral_port, &hints, &clientinfo)) != 0) {
              fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
              return 10;
            }
            for(p = clientinfo; p != NULL; p = p->ai_next) {
              if ((newsock_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
                error_sys("ERROR: SERVER (child): socket");
                continue;
              }
              setsockopt(newsock_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
              if (bind(newsock_fd, p->ai_addr, p->ai_addrlen) == -1) {
           	    close(newsock_fd);
                error_sys("ERROR: SERVER (newsock_fd): bind");
                continue;
              }
              break;
            }	
            freeaddrinfo(clientinfo);
            
            // Create data packet
            bzero(file_payload, sizeof(file_payload));
            bzero(data, sizeof(data)); 
            read_ret = fread (&data,1,512,fp);// Retrieve data from file pointer corresponding to filename received in RRQ
            if(read_ret>=0) {
              data[read_ret]='\0';
              printf("Result is %d\n",read_ret);
            }
            if(read_ret < 512)
              last_block = 1;
            BlockNo = htons(1);                            // 1st 512 Byte block 
            opcode2 = htons(3);                                // Opcode = 3: Data 
            memcpy(&file_payload[0], &opcode2, 2);             // 2 Bytes
            memcpy(&file_payload[2], &BlockNo, 2);         // 4 Bytes 
            for (b = 0; data[b] != '\0'; b++) {        
              file_payload[b+4] = data[b];
            }
	    int p = 0;
            bzero(file_payload_copy, sizeof(file_payload_copy));
            memcpy(&file_payload_copy[0], &file_payload[0], 516);
            send_res=sendto(newsock_fd, file_payload, (read_ret + 4), 0, (struct sockaddr*)&client, clientlength);
            NACK = 1;
            if (send_res < 0)
              error_sys("Couldn't send first packet: ");
            else 
              printf("Sent first block successfully.\n");
      
            while(1){
              if (timeout_indicator (newsock_fd, 1) != 0) {
                bzero(buff, sizeof(buff));
                bzero(file_payload, sizeof(file_payload));
                recbyte = recvfrom(newsock_fd, buff, sizeof(buff), 0, (struct sockaddr*)&client, &clientlength);
                timeout_count = 0;
                if (recbyte < 0) {
                  error_sys("Couldn't receive data\n");
                  return 6;
                }
                else {}
                memcpy(&opcode1, &buff[0], 2);
                if (ntohs(opcode1) == 4) {                               // Opcode = 4: ACK
                  bzero(&blocknum, sizeof(blocknum));
                  memcpy(&blocknum, &buff[2], 2);
                  blocknum = ntohs(blocknum);
                  printf("ACK %i received\n", blocknum);
                  //check reach end of file
                  if(blocknum == NACK) {
                    printf("Expected ACK has arrived\n");
                    NACK = (NACK + 1)%65536;
            	if(last_block == 1){
                      close(newsock_fd);
                      fclose(fp);
            	  printf("SERVER: Full file is sent and connection is closed.\n");
            	  exit(5);
            	  last_block = 0;
            	}
            	else {
                      bzero(data, sizeof(data));
                      read_ret = fread (&data,1,512,fp);
                      if(read_ret>=0) {
                        if(read_ret < 512)
                          last_block = 1;
                        data[read_ret]='\0';
                        BlockNo = htons(((blocknum+1)%65536));
                        opcode2 = htons(3);
                        memcpy(&file_payload[0], &opcode2, 2);
                        memcpy(&file_payload[2], &BlockNo, 2);            
                        for (b = 0; data[b] != '\0'; b++) {        
                          file_payload[b+4] = data[b];
                        }                     
                        bzero(file_payload_copy, sizeof(file_payload_copy));
                        memcpy(&file_payload_copy[0], &file_payload[0], 516);
                        int send_res = sendto(newsock_fd, file_payload, (read_ret + 4), 0, (struct sockaddr*)&client, clientlength);
                        if (send_res < 0) 
                          error_sys("ERROR: Sendto ");
                      }
                    }
                  }
                  else {
                    printf("Expected ACK hasn't arrived: NACK: %d, blocknum: %d\n", NACK, blocknum);
                  }
                }
              }
              else {
                timeout_count++;
                printf("Timeout occurred.\n");
                if (timeout_count == 10){
                  printf("Timeout occurred 10 times. Closing socket connection with client.\n");
                  close(newsock_fd);
                  fclose(fp);
                  exit(6);
                }
                else {
                  bzero(file_payload, sizeof(file_payload));
                  memcpy(&file_payload[0], &file_payload_copy[0], 516);
                  memcpy(&BlockNo, &file_payload[2], 2);
                  BlockNo = htons(BlockNo);
                  printf ("Retransmitting Data with BlockNo: %d\n", BlockNo);
                  send_res = sendto(newsock_fd, file_payload_copy, (read_ret + 4), 0, (struct sockaddr*)&client, clientlength);
                  bzero(file_payload_copy, sizeof(file_payload_copy));
                  memcpy(&file_payload_copy[0], &file_payload[0], 516);
                  if (send_res < 0) 
                    error_sys("ERROR: Sendto ");
                } 
              }
            }
          }
          else {
            unsigned short int ERRCode = htons(1);
            unsigned short int ERRoc = htons(5);             // Opcode = 5: Error
            char ERRMsg[512] = "File not found";
            char ERRBuff[516] = {0};
            memcpy(&ERRBuff[0], &ERRoc, 2);
            memcpy(&ERRBuff[2], &ERRCode, 2);
            memcpy(&ERRBuff[4], &ERRMsg, 512);
            sendto(sockfd, ERRBuff, 516, 0, (struct sockaddr*)&client, clientlength);
            printf("Server clean up as filename doesn't match.\n");
            close(sockfd);
            fclose(fp);
            exit(4);
          }
        }
        else if (opcode1 == 2) {                                 // WRQ processing
          *ephemeral_port = htons(0);
          if ((ret = getaddrinfo(NULL, ephemeral_port, &hints, &clientinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
            return 10;
          }
          for(p = clientinfo; p != NULL; p = p->ai_next) {
            if ((newsock_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
              error_sys("ERROR: SERVER (child): socket");
              continue;
            }
            setsockopt(newsock_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
            if (bind(newsock_fd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(newsock_fd);
              error_sys("ERROR: SERVER (newsock_fd): bind");
              continue;
            }
            break;
          }	
          freeaddrinfo(clientinfo);
	  printf("SERVER: WRQ received from client.\n");
	  FILE *fp_wr = fopen("WRQ_data.txt", "w+");
	  if (fp_wr == NULL)
	    printf("SERVER: WRQ: Problem in opening file");
          opcode2 = htons(4);
	  BlockNo = htons (0);
          bzero(ack_packet, sizeof(ack_packet));
          memcpy(&ack_packet[0], &opcode2, 2);
          memcpy(&ack_packet[2], &BlockNo, 2);
          send_res = sendto(newsock_fd, ack_packet, 4, 0, (struct sockaddr*)&client, clientlength);
	  NACK = 1;
          if (send_res < 0) 
            error_sys("WRQ ACK ERROR: Sendto ");
	  while(1){
            bzero(buff, sizeof(buff));
            recbyte = recvfrom(newsock_fd, buff, sizeof(buff), 0, (struct sockaddr*)&client, &clientlength);
            if (recbyte < 0) {
              error_sys("WRQ: Couldn't receive data\n");
              return 9;
            }
      
            bzero(data, sizeof(data));
            memcpy(&BlockNo, &buff[2], 2);
	    g = 0;
            for (b = 0; buff[b+4] != '\0'; b++) {        
 	      if (buff[b+4] == '\n'){
	        printf("LF character spotted.\n");
	        g++;
	        if (b-g<0)
	          printf("ERROR: b-g is less than 0");
	        data[b-g] = '\n';
              }	      
	      else
                data[b - g] = buff[b+4];
            }	
	    fwrite(data, 1, (recbyte - 4 - g), fp_wr);
	    BlockNo = ntohs(BlockNo);
	    if (NACK == BlockNo){
	      printf("SERVER: Received data block #%d\n", NACK);
	      printf("SERVER: Expected data block received.\n");
              opcode2 = htons(4);
	      BlockNo = ntohs(NACK);
              bzero(ack_packet, sizeof(ack_packet));
              memcpy(&ack_packet[0], &opcode2, 2);
              memcpy(&ack_packet[2], &BlockNo, 2);
	      printf("SERVER: Sent ACK #%d\n", htons(BlockNo));
              send_res = sendto(newsock_fd, ack_packet, 4, 0, (struct sockaddr*)&client, clientlength);             
	      if (recbyte < 516) {
                printf("Last data block has arrived. Closing client connection and cleaning resources. \n");
                close(newsock_fd);
                fclose(fp_wr);
                exit(9);
	      }
              NACK = (NACK + 1)%65536;
	    }
	  }
        }
      }
  }  
}











