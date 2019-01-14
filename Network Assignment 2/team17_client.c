# include <stdio.h>
# include <string.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/socket.h>
# include <errno.h>
# include <arpa/inet.h>


void cli_conn(int *serv_filedes, struct sockaddr_in *servaddr, int argv3, char argv2[4]);
void sendrecv_info(int i, int serv_filedes, char argv1[16]);
void separate(char buffer[256], char username[16], int serv_filedes);
void consolidate(int serv_filedes, char argv1[16]);
void join(int serv_filedes1, char username[16]);


int main(int argc, char *argv[])
{

    struct sockaddr_in servaddr;

    int serv_filedes=0;
    int max_fd, i;


    int argv3=atoi(argv[3]);




    fd_set main_set, temp_set;

    fflush(stdout);
    printf("\nHello %s! You will reach the server in a while...\n\n",argv[1]);
   

    cli_conn(&serv_filedes, &servaddr, argv3, argv[2]);
    sleep(1);
    fflush(stdout);

    printf("\n%s, You are in the chatroom.\n\n", argv[1]);

    
    sleep(1);
    printf("\nWelcome to the chat room\n\n");
    fflush(stdout);
    sleep(1);


   
    sleep(1);
    int serv_filedes1=serv_filedes;
    join(serv_filedes1,argv[1]);

    char user_list[1024]={'\0'};

    int l;



    l=recv(serv_filedes1,user_list,1024,0);

    if(strcmp(user_list,"error_name")==0)
    {
        printf("Error encountered. Please enter a different name.\n\n");
        exit(0);
    }

    if(strcmp(user_list,"error_max")==0)
    {
        printf("Error encountered. Maximum users already present in chatroom.\n\n");
        exit(0);
    }


    printf("\nRULES ::::::\n\n");
    printf("1. Please do not have your username as 'error'. This is because server is using this keyword for something else.\n\n");
    printf("2. Using the message 'STOP' will exit you from the chat room.\n\n");

    printf("Enjoy chatting!\n\n");

    printf("\n\nActive users: %s\n\n",user_list);

    FD_ZERO(&main_set);
    FD_ZERO(&temp_set);

    FD_SET(0,&main_set);
    FD_SET(serv_filedes,&main_set);

    max_fd=serv_filedes;
    fflush(stdout);
    printf("%s:",argv[1]);
    fflush(stdout);


    while(1)
    {

        temp_set=main_set;

        if(select(max_fd+1,&temp_set,NULL,NULL,NULL)==-1)
        {
            error("Cannot select()");
            exit(4);
        }


        for(i=0;i<=max_fd;i++)
        {
            if(FD_ISSET(i,&temp_set))
            {
                sendrecv_info(i,serv_filedes,argv[1]);
            }
        }
    }


    return 0;
}









void separate(char buffer[256], char username[16], int serv_filedes)
{

    int buff_len=strlen(buffer);
    int user_len=strlen(username);

    struct attr_payload
    {

        char client_username[16];
        char message[512];
        char reason[32];
        int client_count;

    };


    struct sbcp_attribute
    {

    int attribute_type;
    int attribute_length;
    struct attr_payload sbcp_payload;

    }msg_username, msg, msg_reason;

    struct sbcp_message
    {

        int ver;
        int msg_type;
        int msg_length;
        struct sbcp_attribute msg_payload[4];

    }final_msg,join_msg;

    memset(&final_msg, 0, sizeof(final_msg));

    strcpy(msg_username.sbcp_payload.client_username,username);
    msg_username.attribute_type=2;
    msg_username.attribute_length=user_len+2+2;

    strcpy(msg.sbcp_payload.message,buffer);
    msg.attribute_type=4;
    msg.attribute_length=buff_len+2+2;

    final_msg.ver=3;
    final_msg.msg_type=4;
    final_msg.msg_length=sizeof(msg_username)+sizeof(msg)+2+2;

    final_msg.msg_payload[0]=msg_username;
    final_msg.msg_payload[1]=msg;

    send(serv_filedes, &final_msg,sizeof(final_msg),0);
    memset(&final_msg, 0, sizeof(final_msg));
    fflush(stdout);
    printf("%s:",username);
    fflush(stdout);
}


void sendrecv_info(int i, int serv_filedes, char argv1[16])
{

    char send_buffer[256];
    char receive_buffer[256];

    int received_bytes;
    if(i==0)
    {

        fgets(send_buffer,256,stdin);
        if(strcmp(send_buffer,"STOP\n")!=0)
				 separate(send_buffer,argv1,serv_filedes);
	    else
           exit(0);
            fflush(stdout);
    }

    else
    {

        consolidate(serv_filedes, argv1);

    }
}


void consolidate(int serv_filedes, char argv1[16])
{

    struct attr_payload
    {

        char client_username[16];
        char message[512];
        char reason[32];
        int client_count;

    };


   struct sbcp_attribute
    {

        int attribute_type;
        int attribute_length;
        struct attr_payload sbcp_payload;

    };

    struct sbcp_message
    {

        int ver;
        int msg_type;
        int msg_length;
        struct sbcp_attribute msg_payload[4];

    }final_msg;

    int received_bytes;
    char receive_buffer[256];

    received_bytes=recv(serv_filedes, &final_msg, sizeof(final_msg),0);

    strcpy(receive_buffer,final_msg.msg_payload[1].sbcp_payload.message);
    receive_buffer[received_bytes]='\0';

    printf("\n%s: %s",final_msg.msg_payload[0].sbcp_payload.client_username,receive_buffer);
    fflush(stdout);
    printf("%s:",argv1);
    fflush(stdout);
    memset(&final_msg, 0, sizeof(final_msg));

    fflush(stdout);

}


void cli_conn(int *serv_filedes, struct sockaddr_in *servaddr, int argv3, char argv2[4])
{
    if((*serv_filedes=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        error("Cannot create socket.");
        exit(1);
    }

    servaddr->sin_family=AF_INET;
    servaddr->sin_port=htons(argv3);
    servaddr->sin_addr.s_addr=inet_addr(argv2);


    memset(servaddr->sin_zero,'\0',sizeof servaddr->sin_zero);

    if(connect(*serv_filedes,(struct sockaddr *) servaddr, sizeof(struct sockaddr))==-1)
    {
        error("Failed to connect to socket.");
        exit(1);
    }
}


void join(int serv_filedes1, char username[16])
{


    int user_len=strlen(username);

    struct attr_payload
    {

        char client_username[16];
        char message[512];
        char reason[32];
        int client_count;

    };


    struct sbcp_attribute
    {

        int attribute_type;
        int attribute_length;
        struct attr_payload sbcp_payload;

    }msg_username, msg, msg_reason;

    struct sbcp_message
    {

        int ver;
        int msg_type;
        int msg_length;
        struct sbcp_attribute msg_payload[4];

    }join_msg;

    memset(&join_msg, 0, sizeof(join_msg));

    strcpy(msg_username.sbcp_payload.client_username,username);
    msg_username.attribute_type=2;
    msg_username.attribute_length=user_len+2+2;


    join_msg.ver=3;
    join_msg.msg_type=2;
    join_msg.msg_length=sizeof(msg_username)+2+2;

    join_msg.msg_payload[0]=msg_username;

    send(serv_filedes1, &join_msg,sizeof(join_msg),0);

    memset(&join_msg, 0, sizeof(join_msg));
}
