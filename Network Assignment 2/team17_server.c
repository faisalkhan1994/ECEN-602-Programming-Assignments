# include <stdio.h>
# include <string.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <stdlib.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <netdb.h>

/* Global Structures */
int count;
void reply_join(int socket_fd1, int argv3);
char users[1024];

struct attr_payload
{
    char client_usrname[16];
    char message[512];
    char reason[32];
    int client_count;
}a_payload;

struct attr_sbcp
{
    int attr_type;
    int attr_length;
    struct attr_payload sbcp_attr_payload;
}msg_username, msg, msg_reason;

struct msg_sbcp
{
    int ver;
    int msg_type;
    int msg_length;
    struct attr_sbcp msg_payload[4];
}final_msg, join_msg;

/* Declarations of used functions */
void connect_server(int *socket_fd,struct sockaddr_in *serv_addr,int argv2, int argv3, char argv1[4]);
void accept_new_client(fd_set *main_set, int *max_fd, int socket_fd, struct sockaddr_in *client_addr, char argv2[10], int argv3);
void send_receive_data(int num, fd_set *main_set, int socket_fd, int max_fd);
void broadcast_data (int count, int num, int socket_fd, int recvd_bytes, struct msg_sbcp receive_buffer, fd_set *main_set);
void delete_client(fd_set *main_set, int *max_fd, int socket_fd, struct sockaddr_in *client_addr, char argv2[10], int argv3);

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr, client_addr;
    fd_set main_set;
    fd_set temp_set;
    int max_fd;
    int i;
    int argv2=atoi(argv[2]);
    int argv3=atoi(argv[3]);
    int socket_fd=0;
    
    FD_ZERO(&main_set);
    FD_ZERO(&temp_set);
    
    connect_server(&socket_fd, &serv_addr,argv2,argv3,argv[1]);
    FD_SET(socket_fd, &main_set);
    max_fd=socket_fd;
    
    fflush(stdout);
    printf("\nServer being started...\n\n");
    fflush(stdout);
    sleep(1);
    fflush(stdout);
    printf("\nServer up on IP %s and Port %s.\n\n",argv[1], argv[2]);
    fflush(stdout);
    
    while(1)
    {
        temp_set=main_set;
    
        if(select(max_fd+1,&temp_set,NULL,NULL,NULL)==-1)
        {
            error("Error in select()..!");
            exit(4);
        }
        
        for(i=0;i<=max_fd;i++)
        {
            if(FD_ISSET(i, &temp_set))
            {
                if(i==socket_fd)
                {
                    accept_new_client(&main_set,&max_fd,socket_fd,&client_addr,argv[2],argv3);
                }
            
                else
                    send_receive_data(i,&main_set,socket_fd,max_fd);
            }
        }
    }
    
    return 0;
}

void connect_server(int *socket_fd,struct sockaddr_in *serv_addr, int argv2,int argv3, char argv1[4])
{
    if((*socket_fd = socket(AF_INET,SOCK_STREAM,0))==-1)
        error("Error in socket()..!");
    
    serv_addr->sin_family      = AF_INET;
    serv_addr->sin_port        = htons(argv2);
    serv_addr->sin_addr.s_addr = inet_addr(argv1);
    
    memset(serv_addr->sin_zero, '\0', sizeof serv_addr->sin_zero);
    int flag=1;
    
    if(setsockopt(*socket_fd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(int))==-1)
    {
        error("Error in socket opt()...\n");
        exit(1);
    }
    
    if(bind(*socket_fd, (struct sockaddr *)serv_addr, sizeof (struct sockaddr))==-1)
    {
        error("Error in bind()...\n");
        exit(1);
    }
    
    if(listen(*socket_fd,argv3) == -1)
    {
        error("Error in listen()...\n");
        exit(1);
    }
}

void accept_new_client(fd_set *main_set, int *max_fd, int socket_fd, struct sockaddr_in *client_addr,char argv2[10], int argv3)
{
    int new_socket_fd;
    socklen_t length;
    length=sizeof(struct sockaddr_in);
    
    if((new_socket_fd=accept(socket_fd,(struct sockaddr*)client_addr,&length))==-1)
    {
        error("Client error... Cannot accept...");
        exit(1);
    }
	
    else
    {
        FD_SET(new_socket_fd,main_set);
		
        if(new_socket_fd>*max_fd)
        {
        	*max_fd=new_socket_fd;
        }
        
        printf("Connection Up: (%s : %s) on Client port: %d\n",inet_ntoa(client_addr->sin_addr),argv2,ntohs(client_addr->sin_port));
        int new_socket_fd1=new_socket_fd;
        reply_join(new_socket_fd1,argv3);
    }
}

void send_receive_data(int num, fd_set *main_set, int socket_fd, int max_fd)
{
    
    int recvd_bytes;
    
    memset(&final_msg, 0, sizeof(final_msg));
    
    if((recvd_bytes= recv(num, &final_msg, sizeof(final_msg), 0)) > 0)
    {
		int count;
        for(count=0;count<=max_fd;count++)
        {
            broadcast_data(count,num,socket_fd,recvd_bytes,final_msg,main_set);
        }
        
		fflush(stdout);
        printf("\nSending Message to all the clients...Done!\n");
        fflush(stdout);
    }
    
    else
    {
        if(recvd_bytes==0)
        {
            printf("IDLE: No message from client...\n", num);
        }
		
        else
        {
            error("Error in data sent...");
        }
        
        close(num);
        FD_CLR(num,main_set);
    }
}

void reply_join(int socket_fd1, int argv3)
{
    int recvd_bytes;
    memset(&join_msg, 0, sizeof(join_msg));
    
    if((recvd_bytes= recv(socket_fd1, &join_msg, sizeof(join_msg), 0)) <=0)
    {
        if(recvd_bytes==0)
        {
            printf("Socket not replying...", socket_fd1);
        }
        
		else
        {
            error("Error in received bytes...");
        }
    
        close(socket_fd1);
    }

    char name[16];
    strcpy(name,join_msg.msg_payload[0].sbcp_attr_payload.client_usrname);
    
    if(strstr(users,name))
    {
        fflush(stdout);
        printf("Give different name of clients, disconnecting client...");
        fflush(stdout);
        
        char error[]="error_name";
        send(socket_fd1,error,strlen(error),0);
        memset(&join_msg, 0, sizeof(join_msg));
    }
    
    else
    {
        if(count<argv3-1)
        {
			strcat(users,join_msg.msg_payload[0].sbcp_attr_payload.client_usrname);
            
            int m=strlen(users);
            
            send(socket_fd1,users,strlen(users),0);
            users[m]=',';
            memset(&join_msg, 0, sizeof(join_msg));
            count++;  
        }
    
        else
        {
            printf("Max client supported reached...!");
            fflush(stdout);
            
            char error[]="error_max";
            send(socket_fd1,error,strlen(error),0);
            memset(&join_msg, 0, sizeof(join_msg));
        }
    }
}

void broadcast_data (int count, int num, int socket_fd, int recvd_bytes, struct msg_sbcp final_msg, fd_set *main_set)
{
    if(FD_ISSET(count,main_set))
    {
        if(count!=socket_fd && count!=num)
        {
            if(send(count,&final_msg,recvd_bytes,0)==-1)
            {
                error("Error in send()...");
            }
        
            memset(&final_msg, 0, sizeof(final_msg));
        }
    }
}


