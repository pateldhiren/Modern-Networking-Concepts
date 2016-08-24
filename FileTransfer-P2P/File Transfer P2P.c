#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#define MAX_HOSTS 5
#define STDIN_FILENO 0

/* Global variables declaration - Variables are used many places so kept them as global so that
no need to declare again*/
void *addr;
int status;
struct addrinfo hints;
struct addrinfo *servinfo,*p;
struct sockaddr_in *addr_v4;
struct sockaddr_in peer_addr,their_addr;
int sock,newsock;
char self_ip[INET6_ADDRSTRLEN],ip[INET6_ADDRSTRLEN],command[50]="",welmessage[100];;
char receivedcom[50],welmessage[]="You have successfully connected";
int cli_sock[MAX_HOSTS],i,j,binderr,connerr;;
fd_set readfds;
fd_set writefds;
int fdmax;
struct hostent *he;
struct in_addr ipv4addr;
socklen_t addrlen;

int host_no = 0;
/*Below Variables are used to keep trakc of all live clients (SERVER IP LIST)*/
char ip_list[MAX_HOSTS][20];
char hostname_list[MAX_HOSTS][50];
char port_list[MAX_HOSTS][5]; 
int id_list[MAX_HOSTS] ;

char final[1024];
void display_live_clients();

/*Whenever Server Receives command from other clients, It will execute this function to REGISTER clients*/
void execute_com(char receivedcom[50])
{
int argc=0;
char *arg,argv[3][50];

if(host_no==5)
{
	printf("Maximum clients no. reached");
}
else{
arg = strtok(receivedcom," ");
while(arg)
{
strcpy(argv[argc++], arg);
arg = strtok(NULL, " ");
}//while
if(argc ==3)
{
	if(strcmp(argv[0],"REGISTER")==0)
	{
			for(i=0;i<MAX_HOSTS;i++)
			{
				if((strcmp(ip_list[i],"")==0))
				{
					addrlen = sizeof peer_addr;
					memset(&peer_addr, 0, sizeof peer_addr);
					getpeername(newsock , (struct sockaddr*)&peer_addr, &addrlen); 
					id_list[i]= i+1;					
					strcpy(ip_list[i],inet_ntoa(peer_addr.sin_addr));										
					inet_pton(AF_INET, inet_ntoa(peer_addr.sin_addr), &ipv4addr);
					he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
					strcpy(hostname_list[i] ,he->h_name);					
					strcpy(port_list[i],argv[2]);	
					break;
				}
			}
		display_live_clients();
				fflush(stdout);
	}
}//if 

}//else
}

/*This fucntion is used to display all live clients and send it to all connected clients*/
void display_live_clients()
{
	memset(final,0,1024);
	//strcpy(final,"");
 	printf("list of Live Clients : ");
	for(i=0;i<=MAX_HOSTS;i++)
	{   
if(id_list[i]!=0)
{
		printf("\n%d %s %s %s",id_list[i],hostname_list[i],ip_list[i],port_list[i]);
		strcat(final,ip_list[i]);
		strcat(final," ");
		strcat(final,port_list[i]);
		strcat(final," ");
}
	}
int len = strlen(final);	
	for(i=0;i<MAX_HOSTS;i++)
	{
		if(cli_sock[i]!=0)
		{
			if(strcmp(final,"")!=0)
			send(cli_sock[i],final,1024,0);
		}
	}
}

/*Whenever Some clients exits it will update SERVER IP LIST and will send it to all connected clients*/
void update_live_clients(char ip[20])
{
	for(i=0;i<=MAX_HOSTS;i++)
	{   
if(strcmp(ip_list[i],ip)==0)
{
	id_list[i]=0;
	strcpy(ip_list[i],"");
	strcpy(port_list[i],"");
	strcpy(hostname_list[i],"");
	break;
}
}
display_live_clients();
}

/*Server side Program, Will run on timberlake.*/
void server(char *port)
{
int k = 0;
for(k=0;k<MAX_HOSTS;k++)
{
	id_list[k]=0;
	//strcpy(ip_list[k],"");
	//strcpy(port_list[k],"");
	memset(ip_list[k],0,20);
	memset(port_list[k],0,5);
}
memset(&hints,0,sizeof hints);
hints.ai_family= AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
if(getaddrinfo(NULL,port, &hints, &servinfo)!=0)
{
printf("Error in getting address info");
}

char temp_hostname[50];
struct addrinfo *serv;
gethostname(temp_hostname,sizeof(temp_hostname));
if(getaddrinfo(temp_hostname,port, &hints, &serv)!=0)
{
printf("Error in getting address info");
}
for(p=serv;p!=NULL;p = p->ai_next)
{

if(p->ai_family==AF_INET)
{
addr_v4 = (struct sockaddr_in *) p->ai_addr;
addr = &(addr_v4->sin_addr);

}
inet_ntop(p->ai_family,addr,ip,sizeof ip);

printf("%s %d %s\n", ip,addr_v4->sin_port,p->ai_canonname);
}

if((sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol))<0)
{
printf("sock error\n");
exit(0);
}

if((binderr=bind(sock,servinfo->ai_addr,servinfo->ai_addrlen))<0)
{
printf("bind error %d\n",binderr);
exit(0);
}
//printf("bfr lis\n");
listen(sock,5);
fdmax = sock;
//printf("aft lis\n");
printf("Waiting for conncetions\n");
for(i=0;i<MAX_HOSTS;i++)
	cli_sock[i]=0;

while(1)
{
FD_ZERO(&readfds);
FD_SET(sock,&readfds);
fdmax = sock;	
for(i=0;i<MAX_HOSTS;i++)	
{
	newsock = cli_sock[i];
	if(newsock>0)
	{
		FD_SET(newsock,&readfds);
	}
	if(newsock>fdmax)
		fdmax = newsock;
}//for
if(select(fdmax+1,&readfds,NULL,NULL,NULL)==-1)
{
	printf("Select error");
	exit(4);
}	

	if(FD_ISSET(sock,&readfds))   // To listen for all incoming client connection requests
	{
		addrlen = sizeof(their_addr);
		memset(&their_addr, 0, sizeof their_addr);
		if((newsock = accept(sock, (struct sockaddr *)&their_addr,&addrlen))<0)
		{
			printf("accept error");
			exit(0);
		}
		if( send(newsock, welmessage, strlen(welmessage), 0) != strlen(welmessage) ) 
            {
                perror("send");
            }
          printf("Client Connected , ip %s , port %d \n" , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));             
		for(i=0;i<MAX_HOSTS;i++)
		{
			if(cli_sock[i]==0)
			{
				cli_sock[i]=newsock;
				//FD_SET(newsock,&readfds);
				break;
			}
		}
	}

	for(i=0;i<MAX_HOSTS;i++)
	{
		newsock = cli_sock[i];
		if(FD_ISSET(newsock,&readfds)) //To get commands from all connected clients
		{
			//strcpy(receivedcom,"");
			memset(receivedcom,0,50);
			if(recv(newsock,receivedcom,50,0)==0)
			{
				//their_addr = NULL;
				addrlen = sizeof their_addr;
				memset(&their_addr, 0, sizeof their_addr);
				
				getpeername(newsock , (struct sockaddr*)&their_addr , &addrlen);
                printf("Client disconnected , ip %s , port %d \n" , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));             
				close(newsock);
				cli_sock[i]=0;
				update_live_clients(inet_ntoa(their_addr.sin_addr));
				fflush(stdout);
			}
			else{
				printf("\nCommand : %s\n",receivedcom);	
				if(strcmp(receivedcom,"exit")==0)
				{
					//their_addr = NULL;
					addrlen = sizeof their_addr;
					memset(&their_addr, 0, sizeof their_addr);
				getpeername(newsock , (struct sockaddr*)&their_addr ,  &addrlen);
                printf("Client disconnected , ip %s , port %d \n" , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));             
				close(newsock);
				cli_sock[i]=0;
				update_live_clients(inet_ntoa(their_addr.sin_addr));
				fflush(stdout);
				}
				else
				{
				execute_com(receivedcom);
				}
			}
	}
	}
	
}//while(1)
}


/*Client code portion started as below*/
/*All connected active connection list will be maintained in below data structures*/
char p2p_ip_list[MAX_HOSTS][20];
int p2p_port_list[MAX_HOSTS]; 
int p2p_id_list[MAX_HOSTS] = {0,0,0,0,0} ;
char p2p_hostname_list[MAX_HOSTS][50];
int temp_sock;

/*Whenever client wants to connect to other client machines, Below function is used*/
void cli_connect(char ip[20],char port[10])
{
struct addrinfo *temp_info;
memset(&hints,0,sizeof hints);
hints.ai_family= AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
if(getaddrinfo(ip,port, &hints, &temp_info)!=0)
{
printf("Error in getting address info");
}	
if((temp_sock = socket(temp_info->ai_family,temp_info->ai_socktype,temp_info->ai_protocol)) == -1)
{
printf("Getting socket error\n");
exit(0);
}
if((connerr=connect(temp_sock,temp_info->ai_addr,temp_info->ai_addrlen))<0)
{
printf("Connect error %d\n",connerr);
exit(0);
}
//FD_SET(temp_sock,&readfds);
for(i=0;i<MAX_HOSTS;i++)
{
if(p2p_id_list[i]==0)
{
strcpy(p2p_ip_list[i], ip);
inet_pton(AF_INET, p2p_ip_list[i], &ipv4addr);
he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
strcpy(p2p_hostname_list[i] ,he->h_name);
p2p_port_list[i]=atoi(port);
p2p_id_list[i]=temp_sock;
break;
}
}
if(recv(temp_sock,welmessage,100,0)<0)
printf("read error");
else
printf("%s",welmessage);
}
/*End of code change */

char p2pport[10];
struct addrinfo *p2p_info;
int p2p_sock = 0,p2p_newsock = 0;
int p2p_sockconn[MAX_HOSTS];
int register_count = 0,connect_count = 0;

/*This fucntion will create listen socket and will be added to readfds*/
void start_listening(char p2pport[10])
{
memset(&hints,0,sizeof hints);
hints.ai_family= AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
//printf("2");
if(getaddrinfo(NULL,p2pport, &hints, &p2p_info)!=0)
{
printf("Error in getting address info");
}

//printf("[PA1]$ ");
if((p2p_sock = socket(p2p_info->ai_family,p2p_info->ai_socktype,p2p_info->ai_protocol))<0)
{
printf("sock error\n");
exit(0);
}

if((binderr=bind(p2p_sock,p2p_info->ai_addr,p2p_info->ai_addrlen))<0)
{
printf("bind error %d\n",binderr);
exit(0);
}
//printf("bfr lis\n");
listen(p2p_sock,5);	
}

/*Below function is used to send files. As files will be big, so if all can not be in 1 packet then it will be divided in multiple packets*/
int sendall(int s, char *buf, int len)
{
int total = 0; // how many bytes we've sent
int bytesleft = len; // how many we have left to send
int n;
while(total < len) {
n = send(s, buf+total, bytesleft, 0);
if (n == -1) { break; }
total += n;
bytesleft -= n;
}
len = total; // return number actually sent here
return n==-1?-1:0; // return -1 on failure, 0 on success
}

/*Main client code portion*/
void client(char port[10])
{
time_t current_time;
struct tm * time_info;

char temp_string[100000]="";
char temp_hostname[50];
FILE *fp;
char c,output[100000];
int len=0;
char buf[1096];
char p2p_buf[100000];
int argc=0;
char *arg,argv[20][20];
printf("client\n");
memset(&hints,0,sizeof hints);
hints.ai_family= AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
if(getaddrinfo("timberlake.cse.buffalo.edu",port, &hints, &servinfo)!=0)
{
printf("Error in getting address info");
}
for(p=servinfo;p!=NULL;p = p->ai_next)
{
void *addr;
if(p->ai_family==AF_INET)
{
addr_v4 = (struct sockaddr_in*) p->ai_addr;
addr = &(addr_v4->sin_addr);
}
inet_ntop(p->ai_family,addr,ip,sizeof ip);
printf("%s\n", ip);
}
/*sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);*/
if((sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1)
{
printf("Getting socket error\n");
exit(0);
}
printf("sock : %d\n",sock);
/*connect(sock,servinfo->ai_addr,servinfo->ai_addrlen);*/
if((connerr=connect(sock,servinfo->ai_addr,servinfo->ai_addrlen))<0)
{
printf("Connect error %d\n",connerr);
exit(0);
}
printf("conn : %d\n",connerr);
if(recv(sock,welmessage,100,0)<0)
printf("read error");
else
printf("%s\n",welmessage);

for(int i=0;i<MAX_HOSTS;i++)
	p2p_sockconn[i] = 0;

printf("\n[PA1]$ ");
fflush(stdout);
while(1)
{
strcpy(buf,"");
FD_ZERO(&readfds);
FD_SET(fileno(stdin),&readfds);
FD_SET(sock,&readfds); 
if(fileno(stdin)>sock)
fdmax = fileno(stdin);
else 
	fdmax = sock;

/*Start of code change --dhiren*/
if(p2p_sock>0)
{
FD_SET(p2p_sock,&readfds); 
if(p2p_sock>fdmax)    
	fdmax = p2p_sock;
}
for(i=0;i<MAX_HOSTS;i++)	
{
	p2p_newsock = p2p_id_list[i];
	if(p2p_newsock>0)
	{
		FD_SET(p2p_newsock,&readfds);
	}
	if(p2p_newsock>fdmax)
		fdmax = p2p_newsock;
}//for
struct timeval tv;
tv.tv_sec = 3;
tv.tv_usec = 500000;
if(select(fdmax+1,&readfds,NULL,NULL,NULL)==-1)
{
	printf("Select error");
	exit(4);
}	

if(FD_ISSET(sock,&readfds))   // It will get updated SERVER IP LIST from server timberlake
		{
			char temp_buf[1096]="";
			strcpy(buf,"");
			//memset(buf,0,1096);
			if(recv(sock,buf,1096,0)<0)			
				printf("Read error");
			else
			{
				strcpy(temp_buf,buf);
				argc = 0;j=0;
				//printf("%s\n",buf);	
				for(i=0;i<MAX_HOSTS;i++)
				{
					//memset(ip_list,0,20);
					//memset(port_list,0,5);
					strcpy(ip_list[i],"");
					strcpy(port_list[i],"");
				}
				arg = strtok(buf," ");
while(arg)
{
strcpy(argv[argc++], arg);
arg = strtok(NULL, " ");
}//while

for(i=0;i<argc;i++)
{
	
	if(i%2 == 0){
	strcpy(ip_list[j],argv[i]);
	}
	else{
		strcpy(port_list[j],argv[i]);
		j++;
		}
}
printf("\nReceived updated clients table from server :\n");
for(i=0;i<j;i++)
{
	inet_pton(AF_INET,ip_list[i], &ipv4addr);
he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
char temp_hostname[50];
strcpy(temp_hostname ,he->h_name);
	printf("%d   %s    %s  %s\n",(i+1),temp_hostname,ip_list[i],port_list[i]);
}
printf("\n[PA1]$ ");
fflush(stdout);
for(i=0;i<20;i++)
{
	//memset(argv,0,20);
	strcpy(argv[i],"");
}

			}				
		//	printf("[PA1$]");
		}
		
/*Start of code change -- dhiren*/
if(p2p_sock>0)
{
if(FD_ISSET(p2p_sock,&readfds))    //It is used to listen for other client connection requests
{
	addrlen = sizeof(their_addr);
		memset(&their_addr, 0, sizeof their_addr);
		if((p2p_newsock = accept(p2p_sock, (struct sockaddr *)&their_addr,&addrlen))<0)
		{
			printf("accept error");
			exit(0);
		}
		if( send(p2p_newsock, welmessage, strlen(welmessage), 0) != strlen(welmessage) ) 
            {
                perror("send");
            }
          printf("Another Client Connected , ip %s , port %d \n" , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));       
		//		printf("\n[PA1]$ ");
		for(i=0;i<MAX_HOSTS;i++)
		{
			if(p2p_id_list[i]==0)
			{
				p2p_sockconn[i]=p2p_newsock;
				/*12/10*/
				strcpy(p2p_ip_list[i], inet_ntoa(their_addr.sin_addr));
				inet_pton(AF_INET, p2p_ip_list[i], &ipv4addr);
				he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
				strcpy(p2p_hostname_list[i] ,he->h_name);
				p2p_port_list[i]=ntohs(their_addr.sin_port);
				p2p_id_list[i]=p2p_newsock;
				break;
			}
		}
		printf("\n[PA1]$ ");
fflush(stdout);
}
}
/*12/10*/
for(i=0;i<MAX_HOSTS-1;i++)
{
p2p_newsock = p2p_id_list[i];
if(p2p_newsock != 0 ){
if(FD_ISSET(p2p_newsock,&readfds))   //It is used to get commands and other data transfer between connected clients
{
	
	memset(p2p_buf,0,100000);
			if(recv(p2p_newsock,p2p_buf,100000,0)==0)
			{
				//their_addr = NULL;
				addrlen = sizeof their_addr;
				memset(&their_addr, 0, sizeof their_addr);
				
				getpeername(p2p_newsock , (struct sockaddr*)&their_addr , &addrlen);
                printf("Client disconnected , ip %s , port %d \n" , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));             
				close(p2p_newsock);
				p2p_sockconn[i]=0;
				strcpy(p2p_ip_list[i],"");
				strcpy(p2p_hostname_list[i],"");
				p2p_port_list[i] = 0;
				p2p_id_list[i]=0;
				printf("\n[PA1]$ ");
				fflush(stdout);
			}
			else{
				//printf("%s",p2p_buf);
				char temp_buf[100000]="";
				strcpy(temp_buf,p2p_buf);
				char *firstword,*secondword, *context;
				firstword = strtok_r (temp_buf," ", &context);
				
				if(strcmp(firstword,"1234#")==0)
				{
					
					//firstword = strtok_r (p2p_buf," ", &context);
					secondword = strtok_r (NULL," ", &context);
					fp = fopen(secondword,"w");
					fputs(context,fp);
					fclose(fp);
					printf("Save it as file");
					//printf("\n%s",firstword);
					printf("\nSaved file name : %s",secondword);
					//printf("\nSaved file content : %s\n",context);
				}
				else
				{
				printf("%s\n",p2p_buf);					
				fflush(stdout);
				//printf("[PA1]$ ");
				argc = 0;
				arg = strtok(p2p_buf," ");
				while(arg)
				{
				strcpy(argv[argc++], arg);
				arg = strtok(NULL, " ");
				}//while
				if(strcmp(argv[0],"TERMINATE")==0)
				{
				//p2p_newsock = atoi(argv[1]);
					//their_addr = NULL;
					addrlen = sizeof their_addr;
				memset(&their_addr, 0, sizeof their_addr);
				
				getpeername(p2p_newsock , (struct sockaddr*)&their_addr , &addrlen);
                printf("Client disconnected , ip %s , port %d \n" , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));             
				close(p2p_newsock);
				p2p_sockconn[i]=0;
				strcpy(p2p_ip_list[i],"");
				strcpy(p2p_hostname_list[i],"");
				p2p_port_list[i] = 0;
				p2p_id_list[i]=0;
				}
				if(strcmp(argv[0],"GET")==0)
				{
					//p2p_newsock = atoi(argv[1]);
					fp = fopen(argv[2],"r");
					if(fp!=NULL)
					{
					len = 0;
					strcpy(output,"");
					while((c = fgetc(fp)) != EOF)
					{
					output[len] = c;
					len = len +1;
					}
					output[len] = '\0';
					strcpy(temp_string,"");
					strcat(temp_string,"1234# ");
					strcat(temp_string,argv[2]);
					strcat(temp_string," ");
					strcat(temp_string,output);
					len = strlen(temp_string);
					int sentbytes;
					sentbytes=sendall(p2p_newsock,temp_string,len);
					//sentbytes=send(p2p_newsock,temp_string,len,0);
					fclose(fp);
					if(sentbytes == -1)
						printf("File couldn't be sent");
					else
						{
						len = strlen(output);
						printf("File sent successfully, No. of bytes sent %d",len);
						}
					}
					else
					{
						char errormessage[]="Filename entered doesn't exist";
						send(p2p_newsock,errormessage,strlen(errormessage),0);
					}
					//printf("[PA1]$ ");
				}
				if(strcmp(argv[0],"SYNC")==0)
				{
					char timeString[9]=""; 
                    char string[1024]="";
					//strcpy(string,"");
					//strcpy(timeString,"");
					time(&current_time);
					time_info = localtime(&current_time);
					strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
					char *firstword,*context;
				gethostname(temp_hostname,sizeof(temp_hostname));
				firstword = strtok_r (temp_hostname,".", &context);
				strcat(firstword,".txt");
				strcpy(temp_string,"Hi, This is ");
				strcat(temp_string,firstword);
				strcat(temp_string,"\n");
				send(p2p_newsock,temp_string,strlen(temp_string),0);
				strcat(string,"Start ");
				strcat(string,firstword);
				strcat(string," at ");
				strcat(string,timeString);
				strcat(string,"\n");
					strcpy(timeString,"");
					time(&current_time);
					time_info = localtime(&current_time);
					strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
					strcat(string,"END ");
					strcat(string,firstword);
					strcat(string," at ");
					strcat(string,timeString);
					len = strlen(string);
					send(p2p_newsock,string,len,0);
					for(i=0;i<MAX_HOSTS;i++)
					{
						if(p2p_id_list[i]!=0 && p2p_id_list[i]!=p2p_newsock)
						{
							send(p2p_id_list[i],argv[0],strlen(argv[0]),0);
							send(p2p_id_list[i],temp_string,strlen(temp_string),0);
							send(p2p_id_list[i],string,strlen(string),0);
						}
					}
				  // }
				}
			}
printf("\n[PA1]$ ");
fflush(stdout);
}
}
}
}
/*End of code change -- dhiren*/
if(FD_ISSET(fileno(stdin),&readfds))   //To detect Keyboard -- to enter commands
{
strcpy(command,"");
char temp_command[50]="";
/*Start of Code change --dhiren*/
gets(command);	
strcpy(temp_command,command);
argc = 0;
arg = strtok(temp_command," ");
while(arg)
{
strcpy(argv[argc++], arg);
arg = strtok(NULL, " ");
}//while
if(strcmp(command,"QUIT")==0)
{
send(sock,command,50,0);	
break;	
}
else if(strcmp(command,"SYNC")==0)
{
	for(i=0;i<MAX_HOSTS;i++)
		{
			if(p2p_id_list[i]!=0)
			{
				char timeString[9]=""; 
                char string[1024]="";
				send(p2p_id_list[i],command,strlen(command),0);
				char *firstword,*context;
				gethostname(temp_hostname,sizeof(temp_hostname));
				firstword = strtok_r (temp_hostname,".", &context);
				strcat(firstword,".txt");
				strcpy(temp_string,"Hi, This is ");
				strcat(temp_string,firstword);
				strcat(temp_string,"\n");
				send(p2p_id_list[i],temp_string,strlen(temp_string),0);
				//strcpy(string,"");
					//strcpy(timeString,"");
					time(&current_time);
					time_info = localtime(&current_time);
					strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
					strcat(string,"Start ");
				strcat(string,firstword);
				strcat(string," at ");
				strcat(string,timeString);
				strcat(string,"\n");
					strcpy(timeString,"");
					time(&current_time);
					time_info = localtime(&current_time);
					strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
					strcat(string,"END ");
					strcat(string,firstword);
					strcat(string," at ");
					strcat(string,timeString);
					strcat(string,"\n");
					len = strlen(string);
					send(p2p_id_list[i],string,len,0);
					/*}
					else
						printf("Invalid File");*/
				
			}
		}
		
}
else if(strcmp(command,"HELP")==0)
{
	printf("All User Commands list");
	printf("\nHELP : It will display information about all user commands");
	printf("\nCREATOR : It will print student's full name, UBIT number and Email ID");
	printf("\nDISPLAY : It will print student's full name, UBIT number and Email ID, Current machines IP address, Hostname, Port");
	printf("\nREGISTER : It will register client's listening port on server (Usage : REGISTER <Server IP> <client's listening port>)");
	printf("\nCONNECT : It will connect this machine to another live client (Usage : CONNECT <another client IP> <another client Port>)");
	printf("\nLIST : It will display all active client connections");
	printf("\nTERMINATE : It will terminate connection with another client (Usage : Client <conncection id with another client>)");
	printf("\nQUIT : It will break all connection clients and server client also. Server IP list will be updated and send to all clients");
	printf("\nGET : It will get file from mentioned ID number (Usage : GET <Connection Id> <File name>)");
	printf("\nPUT : IT will transfer file to mentioned connection Id number (Usage : PUT <Connection Id> <File name>)");
	printf("\nSYNC : This command will make sure that all peers are up-to-date with their CONNECTED peer");
}
else if(strcmp(command,"CREATOR")==0)
{
	printf("\nName : Dhiren Bharatbhai Patel");
	printf("\nUBIT : 50170084");
	printf("\nEmail Id : dhirenbh@buffalo.edu");
}
else if(strcmp(command,"LIST")==0)
{
	printf("All active p2p connections");
	printf("\nID    Host name                    Ip list    Port list\n");
	inet_pton(AF_INET,ip, &ipv4addr);
he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
char temp_hostname[50];
strcpy(temp_hostname ,he->h_name);
	printf("%d   %s     %s %s\n",sock,temp_hostname,ip,port);
for(i=0;i<MAX_HOSTS;i++)
{
	if(p2p_id_list[i]!=0)
	{
	printf("%d   %s     %s %d\n",p2p_id_list[i],p2p_hostname_list[i],p2p_ip_list[i],p2p_port_list[i]);
}
}
//printf("[PA1]$ ");
}
else if(strcmp(command,"DISPLAY")==0)
{
	printf("\nName : Dhiren Bharatbhai Patel");
	printf("\nUBIT : 50170084");
	printf("\nEmail Id : dhirenbh@buffalo.edu");
	/*
inet_pton(AF_INET,self_ip, &ipv4addr);
he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
strcpy(temp_hostname ,he->h_name);*/
memset(&hints,0,sizeof hints);
hints.ai_family= AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
gethostname(temp_hostname,sizeof(temp_hostname));
if(getaddrinfo(temp_hostname,p2pport, &hints, &p2p_info)!=0)
{
printf("Error in getting address info");
}
  printf("\nHostname : %s",temp_hostname);
  printf("\nPort : %s",p2pport);
for(p=p2p_info;p!=NULL;p = p->ai_next)
{
if(p->ai_family==AF_INET)
{
addr_v4 = (struct sockaddr_in*) p->ai_addr;
addr = &(addr_v4->sin_addr);
}
inet_ntop(p->ai_family,addr,self_ip,sizeof self_ip);
printf("\nIP : %s",self_ip);
printf("\n");
}
fflush(stdout);
}
else if(strcmp(argv[0],"SEND")==0)
{
	int ts;
	ts = atoi(argv[1]);
send(ts,argv[2],100,0);	
}
else if(strcmp(argv[0],"TERMINATE")==0)
{
	int ts;
	ts = atoi(argv[1]);
send(ts,command,50,0);	
//printf("\n[PA1]$ ");
}
else if(strcmp(argv[0],"GET")==0)
{
	int ts;
	ts = atoi(argv[1]);
	if(ts == sock)
		printf("File from server can't b downloaded\n");
	else
	send(ts,command,50,0);
//printf("\n[PA1]$ ");	
}
else if(strcmp(argv[0],"PUT")==0)
{
	p2p_newsock = atoi(argv[1]);
	if(p2p_newsock == sock)
		printf("File can't be uploaded to server\n");
	else{
					fp = fopen(argv[2],"r");
					if(fp!=NULL)
					{
					len = 0;
					strcpy(output,"");
					while((c = fgetc(fp)) != EOF)
					{
					output[len] = c;
					len = len +1;
					}
					output[len] = '\0';
					strcpy(temp_string,"");
					strcat(temp_string,"1234# ");
					strcat(temp_string,argv[2]);
					strcat(temp_string," ");
					strcat(temp_string,output);
					len = strlen(temp_string);
					int sentbytes;
					sentbytes=sendall(p2p_newsock,temp_string,len);
					//sentbytes=send(p2p_newsock,temp_string,len,0);
					fclose(fp);
					if(sentbytes == -1)
						printf("File couldn't be sent");
					else
					{
						len = strlen(output);
						printf("File sent successfully, No. of bytes sent %d",len);
					}
					}
					else
						printf("File doesn't exist");
	}
}
else if(strcmp(argv[0],"CONNECT")==0)
{
	if(connect_count<3)
	{
	int ip_count = 0, port_count = 0;
			for(i=0;i<MAX_HOSTS;i++)
				{
					if(strcmp(ip_list[i],argv[1])==0)
						ip_count++;
					if(strcmp(port_list[i],argv[2])==0)
						port_count++;
				}
		if(ip_count==0 || port_count == 0)
				printf("Enter valid ip or port number");
			else
			{
        cli_connect(argv[1],argv[2]);
		printf("\n");
		fflush(stdout);
		}
		connect_count++;
	}
	else
	{
		printf("Maximum client connection numbers reached");
	}
} 
else if(strcmp(argv[0],"REGISTER")==0)
{      
	if(register_count == 0)
	{
	if(strcmp(argv[1],ip)!=0)
	{
		printf("Enter Valid Server IP address");
	}
	else
	{
        start_listening(argv[2]);
		strcpy(p2pport,argv[2]);
		send(sock,command,50,0);
		register_count++;
	}
	}
	else
		printf("Client is already registered");
} 
/*End of code change --dhiren*/
else{
send(sock,command,50,0);
//printf("\n[PA1]$ ");
}		
printf("\n[PA1]$ ");
fflush(stdout);
}	
}
	
}

void main(int argc, char *argv[])
{
if(argc != 3)
{
printf("Argument expected\n");
exit(0);
}
if(strcmp(argv[1],"s")==0)
{
server(argv[2]);
}
else if(strcmp(argv[1],"c")==0)
{
client(argv[2]);
}
else
{
printf("Wrong input\n");
}
}
