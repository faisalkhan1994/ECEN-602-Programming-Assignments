#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

int parse_URL (char* URL, char *hostname, int *port, char *path) {
  char *token;
  char *host_temp, *path_temp;
  char *tmp1, *tmp2;
  int num = 0;
  char s[16];
  if (strstr(URL,"http") != NULL){
    token = strtok(URL, ":");
    tmp1 = token + 7;
  }
  else{
    tmp1 = URL;
  }
  tmp2 = malloc (64);
  memcpy(tmp2, tmp1, 64);
  if(strstr(tmp1, ":") != NULL){
    host_temp = strtok(tmp1, ":");
    *port = atoi(tmp1 + strlen(host_temp) + 1);
    sprintf(s, "%d", *port);
    path_temp = tmp1 + strlen(host_temp) + strlen(s) + 1;
  }
  else{
    host_temp = strtok(tmp1, "/");   
    *port = 80;
    path_temp = tmp2 + strlen(host_temp);
  }
  if (strcmp(path_temp, "") == 0)
    strcpy(path_temp, "/");
  memcpy(hostname, host_temp, 64);
  memcpy(path, path_temp, 256);
  return(0);
}




int err_sys(const char* x)    // Error display source code
{ 
  perror(x); 
  exit(1); 
}


int main(int argc,char *argv[])
{
  int sockfd, inet_a2n_ret, conn_ret, n;
  char buff[100000] = {0};
  int sendret = 0;
  int recvret = 0;
  unsigned int port_number ;
  char *p, *ptr;
  char req[100];
  char path[256] = {0};
  char hostname[64] = {0};
  int port = 80;
  char URL[256] = {0};

  if (argc != 4){
    err_sys ("USAGE: ./client <Server_IP_Address> <Port_Number> <URL>");
    exit(1);
  }

  port_number = atoi(argv[2]);
  
  struct sockaddr_in servaddr;

  bzero(&servaddr,sizeof servaddr);
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(port_number);
  inet_a2n_ret = inet_aton(argv[1], (struct in_addr *)&servaddr.sin_addr.s_addr);      // FIXME: Maybe a problem
  if (inet_a2n_ret <= 0)
    err_sys ("ERR: inet_aton error");

  sockfd=socket(AF_INET,SOCK_STREAM,0);
  if (sockfd < 0)
    err_sys ("ERR: Socket Error");

  conn_ret = connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
  if (conn_ret < 0)
    err_sys ("ERR: Connect Error");
  memset(req, 0, 100);
  sprintf(req, "GET %s HTTP/1.0\r\n", argv[3]);
  printf("Request sent to proxy server: \n%s\n", req);
  sendret = send(sockfd, req, strlen(req), 0);
  if (sendret == -1) {
      err_sys("CLIENT: Send");
      exit(2);
  }
  memset(buff, 0, 100000);
  parse_URL(argv[3], hostname, &port, path);
  FILE *fp;
  fp=fopen(hostname, "w");
  printf("Waiting for response\n");
  recvret = recv(sockfd, buff, 100000, 0);
  if (recvret <= 0) {
    err_sys("CLIENT: Recv");
    fclose(fp);
    close(sockfd);
    return 1;
  }
  if((strstr(buff, "200")) != NULL)
    printf("'200 OK' received. Saving to file: %s\n", hostname);
  else if ((strstr(buff, "400") != NULL))
    printf("'400 Bad Request' received. Saving to file: %s\n", hostname);
  else if ((strstr(buff, "404") != NULL))
    printf("'404 Page Not Found' received. Saving to file: %s\n", hostname);
  ptr = strstr(buff, "\r\n\r\n");
  fwrite(ptr+4, 1, strlen(ptr)-4, fp);
  fclose(fp);
  close(sockfd);

  return 0;

}
