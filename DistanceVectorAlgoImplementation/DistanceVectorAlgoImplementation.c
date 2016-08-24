#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include<time.h>
#include <sys/timerfd.h>
char  *INF = "10000";
//structure to store neighbour info
struct neighbour
{
	int id;
	char *ip;
	int port;
	char *direct_cost;
	int no_of_packets;
};

//structure to maintain routing table
struct DV
{
	int ur_id;
	int neigh_id;
	int next_hop;
	char *cost;
};

//maintain list of all ip and port from topology file
struct list_ip_port
{
	int id;
	char *ip;
	int port;
};

//It's not packet structure..it just helps in preparing packet for conversion -- packet details is in make_packet() function
struct packet
{
	uint32_t ip;
	uint16_t port;
};
/*Socket connection */
	//Declaration for socket
	struct addrinfo hints;
	void *addr;
	struct addrinfo *servinfo;
	int get_addr_info;
	int serv_sock;
	int cli_sock;
	int bind_err;
struct in_addr ton;
//Declaration for making neighbour and distance vector table
char *self_ip;
int self_port,self_id,update_id;
char *update_cost;
struct neighbour n[10];
struct DV d[10];
struct list_ip_port list[10];
int line_no,total_lines,d_index=0,n_index=0,temp,list_index=0;
int argc=0,interval,total_no_of_packets=0,should_receive = 1;
char *arg,argv[5][25];

//- is used to separate words from line using strtok.
void separate_words(char *str)
{
	
		arg = strtok(str," ");
		argc = 0;
		while(arg)
		{
		strcpy(argv[argc++], arg);
		arg = strtok(NULL, " ");
		}//while
}

//gives total number of lines in topology file.
int totalline(FILE *fp)
{
	int line_count=0;
	char *str;
	str = malloc(sizeof(char) * 50);
	rewind(fp);
	while(fgets(str,50,fp)!=NULL)
	{
		line_count++;
	}
	return line_count;
}

//-- Reads particular line from topology file given by n
char * readline(int n,FILE *fp)
{
	char *str;
	str = malloc(sizeof(char) * 50);
	rewind(fp);
	n--;
	while(n>0)
	{
		fgets(str,50,fp);
		n--;
	}
	fgets(str,50,fp);
	temp = strlen(str)-1;
    if ((temp > 0) && (str[temp] == '\n'))
    str[temp] = '\0';
	return str;
}

//– At program startups it populates initial distance vector and neighbour’s table
void populate_initial_DV_and_neighbour_tables(FILE *fp)
{
char *str;
str = malloc(sizeof(char) * 50);
str = readline(1,fp);
for(temp = 0;temp<atoi(str);temp++)
{
	d[temp].ur_id = 0;
	d[temp].neigh_id = temp+1;
	d[temp].next_hop = temp+1;
	d[temp].cost = malloc(sizeof(char)*5);
	strcpy(d[temp].cost,"inf");
	d_index++;
}


line_no = atoi(str) + 3;
total_lines=totalline(fp);	
	while(line_no<=total_lines)
	{
		str = readline(line_no,fp);
		separate_words(str);
		
		temp = 0;
		while(temp<d_index)
		{
		    d[temp].ur_id = atoi(argv[0]);	
			if(d[temp].neigh_id == atoi(argv[1]))
			strcpy(d[temp].cost,argv[2]);
		    if(d[temp].ur_id == d[temp].neigh_id)
			strcpy(d[temp].cost,"0");
			temp++;
		}
		n[n_index].direct_cost = malloc(sizeof(char)*5);
		strcpy(n[n_index].direct_cost,argv[2]);
		str = readline((atoi(argv[1])+2),fp);
		separate_words(str);
		n[n_index].id = atoi(argv[0]);
		n[n_index].ip = malloc(sizeof(char)*20);
		strcpy(n[n_index].ip,argv[1]);
		n[n_index].port = atoi(argv[2]);
		n[n_index].no_of_packets = 0;
		n_index ++;
	line_no++;
	}//while
	
	
	/*temp = 0;
	while(temp<n_index)
	{
		int jk = 5;
		printf("%d %s %d %s\n",n[temp].id,n[temp].ip,n[temp].port,n[temp].direct_cost);
		temp++;
	}	*/
}

void store_list_ip_port(FILE *fp)
{
	char *line;
	line = malloc(sizeof(char)*50);
	int no_of_routers,temp;
	line = readline(1,fp);
	no_of_routers = atoi(line);
	//struct list_ip_port list[no_of_routers];
	temp = 0;
	while(temp<no_of_routers)
	{
		line = readline(temp+3,fp);
		separate_words(line);
		list[list_index].id = atoi(argv[0]);
		list[list_index].ip = malloc(sizeof(char)*25);
		strcpy(list[list_index].ip,argv[1]);
		list[list_index].port = atoi(argv[2]);
		list_index++;
		temp++;
	}
	
	line = readline(total_lines,fp);
	separate_words(line);
		line = readline((atoi(argv[0])+2),fp);
		separate_words(line);
		self_ip = malloc(sizeof(char)*25);
		strcpy(self_ip,argv[1]);
		self_port = atoi(argv[2]);
		self_id = atoi(argv[0]);
		//printf("\n%d %s %d",self_id,self_ip,self_port);
}

//is used to display routing table
void display_routing_table()
{
	printf("Routing table : \n");
	printf("Source-Id   Destination-Id    Next-Hop    Cost of Path\n");
	temp = 0;
	while(temp<d_index)
	{
		printf("    %d             %d              %d            %s\n",d[temp].ur_id,d[temp].neigh_id,d[temp].next_hop,d[temp].cost);
		temp++;
	}
}

//–  updates routing table after processing input packet
void update_routing_table(char *str,int dummy_list_index)
{
	int temp1,updated = 0,temp2;
	char bfr_updated[10];
	separate_words(str);
if(dummy_list_index ==1)
{
	temp2 = 0;
					while(temp2<n_index)
					{
						if(n[temp2].id == atoi(argv[0]))
						{
							if(atoi(argv[2])>=10000)
								strcpy(n[temp2].direct_cost,"inf");
							else
								strcpy(n[temp2].direct_cost,argv[2]);
						}
						temp2++;
					}
					
if(atoi(argv[2])>=10000)
						{
							temp2 = 0;
							while(temp2<d_index)
							{
							if(d[temp2].next_hop == atoi(argv[0]) && self_id == atoi(argv[1]))
								strcpy(d[temp2].cost,"inf");
							temp2++;
							}
						}						
}

		if(strcmp(argv[2],"inf")==0)
			strcpy(argv[2],INF);
		temp = atoi(argv[0])-1;
		if(strcmp(d[temp].cost,"inf")==0)
			strcpy(d[temp].cost,INF);
		
		if( self_id != atoi(argv[1]))
		{
		temp1 = atoi(argv[1])-1;
		if(d[temp1].next_hop != atoi(argv[0]))
		{	
		strcpy(bfr_updated,d[temp1].cost);
		if(strcmp(d[temp1].cost,"inf")==0)
			strcpy(d[temp1].cost,INF);
		if((atoi(d[temp].cost)+atoi(argv[2]))<atoi(d[temp1].cost))
		{
			updated = 1;
			if(atoi(d[temp].cost)+atoi(argv[2])>=10000)
				strcpy(d[temp1].cost,"inf");
			else
				sprintf (d[temp1].cost, "%d", atoi(d[temp].cost)+atoi(argv[2]));
			d[temp1].next_hop = d[temp].next_hop;//atoi(argv[0]); -essential
			//check for neighbour here and update cost if less
			//d[temp1].nei_id with nei
			//next hop is nei
		}
		if(updated == 0)
		{
		strcpy(d[temp1].cost,bfr_updated);	
		}
		}
		else
		{
			sprintf (d[temp1].cost, "%d", atoi(d[temp].cost)+atoi(argv[2]));
			//d[temp1].nei_id with nei
			//check for neighbour here and update cost if less
			//next hop is nei
			temp2 = 0;
					while(temp2<n_index)
					{
						if(n[temp2].id == d[temp1].neigh_id) //or atoi(argv[1])
						{			
							char temp_co[10];
							strcpy(temp_co,n[temp2].direct_cost);
							if(strcmp(temp_co,"inf")==0)
								strcpy(temp_co,INF);					
							if(atoi(temp_co)<atoi(d[temp1].cost))
							{
							strcpy(d[temp1].cost,n[temp2].direct_cost);
							d[temp1].next_hop = n[temp2].id;
							}
						}
						temp2++;
					}
				if(atoi(d[temp1].cost)>=10000)
				strcpy(d[temp1].cost,"inf");
		}	
		}
		else
		{
			if(d[temp].next_hop != atoi(argv[0]))
			{
				if(atoi(argv[2]) < atoi(d[temp].cost))
				{
					sprintf (d[temp].cost, "%d",atoi(argv[2]));
					if(atoi(d[temp].cost)>=10000)
						strcpy(d[temp].cost,"inf");
					d[temp].next_hop = atoi(argv[0]);
				}
			}
			else
			{
				sprintf (d[temp].cost, "%d",atoi(argv[2]));
				if(atoi(d[temp].cost)>=10000)
						strcpy(d[temp].cost,"inf");
			}
		}
}

/*Processes received data and updates it's distance vector table*/
void process_received_packet()
{
			char *ip,*port,x[10],y[5];
			int temp,dummy_list_index,recei_id;
			ip = malloc(sizeof(char)*25);
			port = malloc(sizeof(char)*5);
			FILE *fp;
			fp = fopen("packet.bin","rb");
			fseek(fp,0,SEEK_END);
			if(ftell(fp)<22)
				dummy_list_index = 1;
			else
				dummy_list_index = list_index;
			rewind(fp);
			uint32_t ip_t;
			uint16_t port_t;
			fread(&port_t,sizeof(port_t),1,fp);
			sprintf(port,"%d",ntohs(port_t));
			//printf("%s ",port);
			fread(&port_t,sizeof(port_t),1,fp);
			sprintf(port,"%d",ntohs(port_t));
			//printf("%s ",port);
			fread(&ip_t,sizeof(ip_t),1,fp);
			inet_ntop(AF_INET,&ip_t,ip,INET_ADDRSTRLEN);
			temp = 0;
			while(temp<n_index)
			{
				if(strcmp(ip,n[temp].ip)==0)
				{
					recei_id = n[temp].id;
					break;
				}
				temp++;
			}
			
			printf("\nRECEIVED A MESSAGE FROM SERVER %d\n",recei_id);
			temp = 0;
			while(temp<n_index)
			{
				if(strcmp(ip,n[temp].ip)==0)
				{
					n[temp].no_of_packets++;
					break;
				}
				temp++;
			}
			temp = 0;
			while(temp<list_index)
			{
				if(strcmp(list[temp].ip,ip)==0)
				{
					sprintf(x,"%d",list[temp].id);
					break;
				}
				temp++;
			}
			strcpy(y,x);
			//printf("%s\n",ip);
			temp = 0;
			while(temp<dummy_list_index)
			{
				fread(&ip_t,sizeof(ip_t),1,fp);
				inet_ntop(AF_INET,&ip_t,ip,INET_ADDRSTRLEN);
				//printf("%s ",ip);
				fread(&port_t,sizeof(port_t),1,fp);
				sprintf(port,"%d",ntohs(port_t));
				//printf("%s ",port);
				fread(&port_t,sizeof(port_t),1,fp);
				sprintf(port,"%d",ntohs(port_t));
				//printf("%s ",port);
				fread(&port_t,sizeof(port_t),1,fp);
				sprintf(port,"%d",ntohs(port_t));
				strcat(x," ");
				strcat(x,port);
				//printf("%s ",port);
				fread(&port_t,sizeof(port_t),1,fp);
				sprintf(port,"%d",ntohs(port_t));
				strcat(x," ");
				strcat(x,port);
				//printf("%s\n",x);
				update_routing_table(x,dummy_list_index);
				//memset(x,0,sizeof x);
				strcpy(x,y);
				//printf("%s\n",port);
				temp++;
			}
}

//– is used to disable link between neighbours
void disable_link(int temp)
{
	int temp1 = 0;
	while(temp1<n_index)
	{
		if(n[temp1].id == temp)
			strcpy(n[temp1].direct_cost,"inf");
	temp1++;
	}
	temp1 = 0;
	while(temp1<d_index)
	{
		if(d[temp1].neigh_id==temp && d[temp1].neigh_id == d[temp1].next_hop)
		{
			strcpy(d[temp1].cost,"inf");
		}
		temp1++;
	}
	
}

void client(int ,int);
//creates UDP socket connection at program startup
void server(FILE *fp)
{
	int timer_count=0;
	time_t start,end;
	struct itimerspec new_value;
    struct timespec now;
	char *str;
	str = malloc(sizeof(char) * 50);
	str = readline(total_lines,fp);

	/*Below code gets ip and port of current machine from topology file*/
	separate_words(str);
	str = readline((atoi(argv[0])+2),fp);
	separate_words(str);
		
	//Declaration for select
	fd_set readfds;
	fd_set writefds;
	int fdmax,timer_fd;
	
	//Declaration to convert ip and port from argv
	int temp_port;
	char *ip,*port;
	inet_pton(AF_INET,argv[1],&ton);
	ip = malloc(sizeof(char)*25);
	inet_ntop(AF_INET,&ton,ip,INET_ADDRSTRLEN);
	temp_port = atoi(argv[2]);
	port = malloc(sizeof(char)*5);
	sprintf(port,"%d",temp_port);
	
	//Below code to open server socket
	memset(&hints,0,sizeof hints);
	hints.ai_family= AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	if((get_addr_info = getaddrinfo(ip,port, &hints, &servinfo))!=0)
	{
		//printf("Error in getting address info");
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_addr_info));
	}
	if((serv_sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol))<0)
	{
		printf("sock error\n");
		exit(0);
	}
	if((bind_err=bind(serv_sock,servinfo->ai_addr,servinfo->ai_addrlen))<0)
	{
		printf("bind error %d\n",bind_err);
		exit(0);
	}
	
	FD_SET(serv_sock,&readfds);	
	FD_SET(fileno(stdin),&readfds);
	if(fileno(stdin)>serv_sock)
		fdmax = fileno(stdin);
	else
		fdmax = serv_sock;
	
	
	start = time(NULL);
	end = start + interval;
struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

printf("\n[PA1]$ ");
fflush(stdout);
char ch;
	while(1)
	{
		if((end-start)<0)
			new_value.it_value.tv_sec =  0 ;
		else if((end-start)>interval)
			new_value.it_value.tv_sec =  interval ;
		else
			new_value.it_value.tv_sec =  (end-start) ;
    new_value.it_value.tv_nsec = 0;
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec = 0;
	timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);	
	if (timer_fd == -1)
		printf("Timer fd create error");
	if (timerfd_settime(timer_fd, 0, &new_value, 0) == -1)
	{
        printf("Timer fd settime error");
		printf("   %d\n",end-start);
	}
	
		FD_ZERO(&readfds);
		if(should_receive == 1)
		FD_SET(serv_sock,&readfds);	
	FD_SET(fileno(stdin),&readfds);
	FD_SET(timer_fd,&readfds);
	if(fileno(stdin)>serv_sock)
		fdmax = fileno(stdin);
	else
		fdmax = serv_sock;
	if(timer_fd>fdmax)
		fdmax = timer_fd;
	
		if(select(fdmax+1,&readfds,NULL,NULL,NULL)==-1)
		{
			printf("Select error");
			exit(4);
		}
		
		if(FD_ISSET(serv_sock,&readfds))   
		{
			//while ((ch = getchar()) != '\n' && ch!=EOF);
			struct sockaddr_storage their_addr;
			socklen_t addr_len;
			char packet[70];
			addr_len = sizeof their_addr;
			int numbytes;
			if ((numbytes=recvfrom(serv_sock,&packet,sizeof(packet) , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
			{
			perror("recvfrom");
			exit(1);
			}
			total_no_of_packets++;
			FILE *fp;
			fp = fopen("packet.bin","wb");
			fwrite(&packet,numbytes,1,fp);
			fclose(fp);
			process_received_packet();
			/*printf("Neighbour table :\n");
temp = 0;
	while(temp<n_index)
	{
		printf("%d %s %d %s\n",n[temp].id,n[temp].ip,n[temp].port,n[temp].direct_cost);
		temp++;
	}*/
			printf("\n[PA1]$ ");
			fflush(stdout);
			//if(start > end)
				//start = end;
			//else
			
			start = time(NULL);
		}
	
		if(FD_ISSET(fileno(stdin),&readfds))   //To detect Keyboard -- to enter commands
		{
			char command[50],temp_command[50];
			scanf("%[^\n]s",command);
			while ((ch = getchar()) != '\n' && ch!=EOF);
			strcpy(temp_command,command);
			argc = 0;
			separate_words(temp_command);
			if(strcmp(argv[0],"step")==0)
			{
				printf("%s  SUCCESS",command);
				client(-10,0);
			}
			else if(strcmp(argv[0],"display")==0)
			{
				display_routing_table();
			}
			else if(strcmp(argv[0],"packets")==0)
			{
				printf("no. of packets received till last time of this command : %d",total_no_of_packets);
				total_no_of_packets = 0;
			}
			else if(strcmp(argv[0],"disable")==0)
			{
				//check whether it is your neighbour or not
				int is_nei = 0;
				temp = 0;
				while(temp<n_index)
				{
					if(n[temp].id == atoi(argv[1]))
					{
						is_nei = 1;
					}
					temp++;
				}
				if(is_nei == 0)
					printf("%s -- %s is not your neighnour",command,argv[1]);		
				else
				{
				disable_link(atoi(argv[1]));
				printf("%s  SUCCESS",command);
				}
			}
			else if(strcmp(argv[0],"crash")==0)
			{
				int temp2=0;
				should_receive = 0;
				FD_CLR(serv_sock,&readfds);
				while(temp2<n_index)
				{
					disable_link(n[temp2].id);
					temp2++;
				}
				printf("%s  SUCCESS",command);
			}
			else if(strcmp(argv[0],"update")==0) 
			{
				//check whether it is your neighbour or not
				int is_nei = 0;
				temp = 0;
				while(temp<n_index)
				{
					if(n[temp].id == atoi(argv[2]))
					{
						is_nei = 1;
					}
					temp++;
				}
				if(is_nei == 0)
					printf("%s -- %s is not your neighnour",command,argv[2]);
				else
				{
					int temp = 0;
					//updating neighbour table cost
					temp = 0;
					while(temp<n_index)
					{
						if(n[temp].id == atoi(argv[2]))
						{
							strcpy(n[temp].direct_cost,argv[3]);
						}
						temp++;
					}
					//updating its dv
					temp = 0;
					while(temp<d_index)
					{
						if(strcmp(argv[3],"inf")==0)
							strcpy(argv[3],INF);
						if(atoi(argv[3])>=10000)
						{
							if(d[temp].next_hop == atoi(argv[2]))
								strcpy(d[temp].cost,"inf");
						}
						if(d[temp].neigh_id == atoi(argv[2]))
						{
							int updated = 0;
							char *str_original;
							str_original = malloc(sizeof(char)*10);
							
						    strcpy(str_original,d[temp].cost);
							if(strcmp(d[temp].cost,"inf")==0)
							strcpy(d[temp].cost,INF);
							if(d[temp].next_hop == atoi(argv[2]))
							{
								updated = 1;
								if(atoi(argv[3])>=10000)
									strcpy(d[temp].cost,"inf");
								else
									strcpy(d[temp].cost,argv[3]);
							}
							if(atoi(argv[3])<atoi(d[temp].cost))
							{
								updated = 1;
								if(atoi(argv[3])>=10000)
									strcpy(d[temp].cost,"inf");
								else
									strcpy(d[temp].cost,argv[3]);
							}
							if(updated ==0)
								strcpy(d[temp].cost,str_original);
						}
						temp++;
					}
				update_id = atoi(argv[2]);
				update_cost = malloc(sizeof(char)*7);
				strcpy(update_cost,argv[3]);
				client(atoi(argv[2]),1); //Sending updated DV table to specified neighbour
				printf("\n%s  SUCCESS\n",command);
				printf("Link cost updated successfully\n");
				printf("Please make note that Routing table might or might not get updated (Depends on shortest path between neighbours)");
				}
			}
			else
			{
				printf("Invalid Command");
			}
			printf("\n[PA1]$ ");
			fflush(stdout);
			//if(start > end)
				//start = end;
			//else
				start = time(NULL);
		}
		
		if(FD_ISSET(timer_fd,&readfds))   
		{
			if(timer_count == 2)
			{
				temp = 0;
				while(temp<n_index)
				{
					if(n[temp].no_of_packets == 0)
					{
						disable_link(n[temp].id);
					}
					else
						n[temp].no_of_packets = 0;
					temp++;
				}
				timer_count = 0;
			}
			timer_count++;
			client(-10,0);
			if(should_receive == 1)
			printf("\n[PA1]$ ");
			fflush(stdout);
			start = time(NULL);
			end = start + interval;
		}
	}
	
} //server method ends

/*Building packet into binary file*/
void make_packet(int checkpoint)
{
	uint32_t ip_t;
	uint16_t port_t;
	FILE *fp;
	char p[25];
	fp = fopen("packet.bin","wb");
	port_t = htons(5);
	fwrite(&port_t,sizeof(port_t),1,fp);
	port_t = htons(self_port);
	fwrite(&port_t,sizeof(port_t),1,fp);
	inet_pton(AF_INET,self_ip,&ton);
	ip_t = ton.s_addr;
	fwrite(&ip_t,sizeof(ip_t),1,fp);
	if(checkpoint == 0)
	{
	temp = 0;
	while(temp<list_index)
	{
		inet_pton(AF_INET,list[temp].ip,&ton);
		ip_t = ton.s_addr;
		fwrite(&ip_t,sizeof(ip_t),1,fp);
		port_t = htons(list[temp].port);
		fwrite(&port_t,sizeof(port_t),1,fp);
		port_t = htons(0);
		fwrite(&port_t,sizeof(port_t),1,fp);
		port_t = htons(list[temp].id);
		fwrite(&port_t,sizeof(port_t),1,fp);
		if(strcmp(d[temp].cost,"inf")==0)
			port_t = htons(10000);
		else
		port_t = htons(atoi(d[temp].cost));
		fwrite(&port_t,sizeof(port_t),1,fp);
		temp++;
	}
	}
	else
	{
		temp = 0;
	while(temp<list_index)
	{
		if(list[temp].id == update_id)
		{
			inet_pton(AF_INET,list[temp].ip,&ton);
		ip_t = ton.s_addr;
		fwrite(&ip_t,sizeof(ip_t),1,fp);
		port_t = htons(list[temp].port);
		fwrite(&port_t,sizeof(port_t),1,fp);
		port_t = htons(0);
		fwrite(&port_t,sizeof(port_t),1,fp);
		port_t = htons(list[temp].id);
		fwrite(&port_t,sizeof(port_t),1,fp);
		//if(strcmp(update_cost,"inf")==0)
			//port_t = htons(10000);
		//else
		port_t = htons(atoi(update_cost));
		fwrite(&port_t,sizeof(port_t),1,fp);
		}
		temp++;
	}
	}
	fclose(fp);
}

void client_send(char *ip,char *port,int checkpoint)
{
	int temp,send_id;
	memset(&hints,0,sizeof hints);
	hints.ai_family= AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	if((get_addr_info = getaddrinfo(ip,port, &hints, &servinfo))!=0)
	{
		//printf("Error in getting address info");
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_addr_info));
	}
	if((cli_sock = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol))<0)
	{
		printf("sock error\n");
		exit(0);
	}
	make_packet(checkpoint);
	FILE *fp;
	fp = fopen("packet.bin","rb");
	fseek(fp,0,SEEK_END);
	int index=0,packet_size = ftell(fp);
	char temp_c,packet[packet_size];
	rewind(fp);
	while(1)
	{
		temp_c = fgetc(fp);
		if(temp_c ==EOF)
			break;
		packet[index]=temp_c;
		index++;
	}
	int numbytes;
	if((numbytes = sendto(cli_sock,&packet,sizeof(packet),0,servinfo->ai_addr,servinfo->ai_addrlen))==-1)
	{
		printf("Send Error");
		exit(1);
	}
	temp = 0;
	while(temp<n_index)
	{
		if(strcmp(ip,n[temp].ip)==0)
		{
			send_id = n[temp].id;
			break;
		}
		temp++;
	}
	printf("\nPacket successfully sent to server %d",send_id);
}
//is used to send UDP messages to other servers
void client(int id,int checkpoint)
{
	int temp = 0;
	char *ip,*port;
	ip = malloc(sizeof(char)*25);
	port = malloc(sizeof(char)*5);
	if(id != -10)
	{
	temp = 0;
	while(temp<n_index)
	{			
		if(n[temp].id == id)
		{
			strcpy(ip,n[temp].ip);
			sprintf(port,"%d",n[temp].port);
			client_send(ip,port,checkpoint);
			break;
		}
		temp++;
	}
	}
	else
	{
		temp = 0;
		while(temp<n_index)
		{
			strcpy(ip,n[temp].ip);
			sprintf(port,"%d",n[temp].port);
			if(strcmp(n[temp].direct_cost,"inf")!=0)
			client_send(ip,port,checkpoint);
			temp++;
		}
	}
}

void main(int argc, char *argv[])
{
	char str[10];
	FILE *fp,*fp_binary;
if(argc != 5)
{
printf("It's not in proper format\n");
	printf("Format : ./a.out -t <topology-file-name> -i <routing-update-interval>\n");
	exit(0);
}
if(strcmp(argv[1],"-t")!=0)
{
	printf("It's not in proper format\n");
	printf("Format : ./a.out -t <topology-file-name> -i <routing-update-interval>\n");
	exit(0);
}
if(strcmp(argv[3],"-i")!=0)
{
	printf("It's not in proper format\n");
	printf("Format : ./a.out -t <topology-file-name> -i <routing-update-interval>\n");
	exit(0);
}
interval = atoi(argv[4]);
	fp = fopen(argv[2],"r+");
	if(fp == NULL)
	{
		printf("File doesn't exist\n");
		exit(0);
	}
	populate_initial_DV_and_neighbour_tables(fp);
	display_routing_table();
	store_list_ip_port(fp);
    server(fp);
}














