#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "mapper.h"
//#include "socketlib.h"

#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold

#define _MAX_PACKET_LEN_ 256
#define _MAX_WORK_LEN_ 2<<9 //1M space
//shared memory space for all cases 
#define _SHM_BITMAP_ID_ 25534
#define _SHM_MAPPER_ID_ 25535
#define _SHM_SERVER_ID_ 25536
#define _SHM_MAPPER_NODE_ SIZE_OF_MAPPER*sizeof(Mapper_item)
#define _SHM_BITMAP_NODE_ MAX_BITMAP_SIZE*sizeof(unsigned long long)	//1024item space shared memeory
#define _SHM_SERVER_NODE_ MAX_SERVER_NODE*sizeof(Server) 	// Server space item
#define MIN(a,b) (a>b?b:a)

char *shm = NULL;
unsigned long long* page_bitmap=NULL;
Server* seg_heap=NULL;
Mapper_item* Mapper=NULL;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
	}
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//modify the lenth in buff: mark the pack lenth
void set_pack_len(char *buff,const int lenth)
{//the buff has already set everthing except for the lenth flag,
 //for it is the last thing known
    const int tmp_len = 64;
    char tmp_buff[tmp_len],*start=buff+1;
    int i=0;
    sprintf((char*)tmp_buff,"%04d",lenth);
    for(i=0;i<4;++i){
        start[i]=tmp_buff[i];
    }
    return ;
}

//get other end's name
int get_peername(int sockfd, char *ipstr, int len_ip, int* port){
    if( sockfd>0 && (!ipstr) && (!port))
        return -1;
    //get peer ip and port
    struct sockaddr_storage addr;
    socklen_t len=sizeof(addr);
    getpeername(sockfd, (struct sockaddr*)&addr, &len);
    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        *port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, len_ip);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        *port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, len_ip);
    }
    return 0;
}

int main(void)
{
    int sockfd, client_fd;  // listen on sock_fd, new connection on client_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int pid=0;
	int numbytes=0, bleft=0;//recv and sepected bytes in packets
	int request_type;
	char buff[_MAX_PACKET_LEN_];//used to receive the packet from client part, 
	//regarless of whether it is a server or client(in view of mapper)
	char workbuff[_MAX_WORK_LEN_];//used to deal with packet command
	char *tp=NULL;//used for set the pointer of msg buffer
	memset((char*)buff,0,_MAX_PACKET_LEN_);
	memset((char*)workbuff,0,_MAX_WORK_LEN_);
	/*-----------------------Shared memory init---------------------------*/
	int shm_bitmap_id = 0, shm_server_id =0, shm_mapper_id=0;
	if((shm_bitmap_id=shmget(_SHM_BITMAP_ID_,_SHM_BITMAP_NODE_,IPC_CREAT | 0666 ))<0){
		perror("shmget");
		exit(1);
	}
	if((shm_server_id=shmget(_SHM_SERVER_ID_,_SHM_SERVER_NODE_,IPC_CREAT | 0666 ))<0){
		perror("shmget");
		exit(1);
	}
	if((shm_mapper_id=shmget(_SHM_MAPPER_ID_,_SHM_MAPPER_NODE_,IPC_CREAT | 0666 ))<0){
		perror("shmget");
		exit(1);
	}
	if ((shm = shmat(shm_bitmap_id, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }
    page_bitmap=(unsigned long long*)shm;
    if ((shm = shmat(shm_server_id, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }
  	seg_heap=(Server*)shm;
 	if ((shm = shmat(shm_mapper_id, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }
    Mapper=(Mapper_item*)shm;
	initial_mapper();

    /*------------------------Internet Settings Init-------------------------*/
    struct sockaddr_in local;
    int len=sizeof(local);
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket error at mapper");
        exit(-1);
    }
    local.sin_family=AF_INET;
    local.sin_addr.s_addr=INADDR_ANY;
    local.sin_port=0;
    bind(sockfd,(struct sockaddr *)&local,sizeof(local));
    /*Find out and publish socket name */
    getsockname(sockfd,(struct sockaddr *)&local,&len);
    printf("Socket has port %d\n",local.sin_port);
    system("./ip_writer > localip");
    usleep(500);
    sprintf((char*)buff,"echo \"%d\" >> localip ",local.sin_port);
    system(buff);
    memset((char*)buff,0,_MAX_PACKET_LEN_);
    /*---------------------Main Loop: listen and fork process to handle-----------------*/
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
	}
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
		exit(1); 
	}
    printf("server: waiting for connections...\n");
    for(;;) {  // main accept() loop
        sin_size = sizeof their_addr;
        client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
			continue; 
		}
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("mapper: got connection from %s\n", s);
        //fork another process to deal with all the messages
        if ( (pid = fork())==0 ) { // this is the child process
            close(sockfd); //close listening process
            //receive first message from ourside 
            if((numbytes = recv(client_fd,buff,_MAX_PACKET_LEN_-1 ,0))==-1){
            	//limit the recv number: _MAX_PACKET_LEN_-1: cause the max return value will be _MAX_WORK_LEN_-1, 
            	//then if we use the statment ment as follow will not cause core dump 
            	perror("recv");
            	fprintf(stderr,"recv:=%d\n, message:=%s\n",numbytes,buff);
            	exit(1);
            }
            if(!numbytes){
            	fprintf(stdout,"client has closed the connect\n");
            	exit(2);
            }
            tp=(char*)workbuff;
            memcpy((char*)tp,(char*)buff,numbytes*sizeof(char));
            tp+=numbytes;
            memset((char*)buff,0,_MAX_PACKET_LEN_);
            //read lenth message
            //attention: there is a $ here
            if(!sscanf(workbuff,"$%d#%d",&bleft,&request_type)){
            	fprintf(stdout,"buff is empty");
            	exit(2);
            }
           printf("\t+expected lenth next:=%d\n",bleft);
           bleft-=numbytes;
           int buffer_left=_MAX_WORK_LEN_-numbytes-1;//left space for '\0'
            //keep reading if there is not comming to the end
            while( bleft > 0 && buffer_left>0){
            	if((numbytes = recv(client_fd,buff,_MAX_PACKET_LEN_,0))==-1){
            		perror("recv");
            		fprintf(stderr,"recv:=%d\n, message:=%s\n",numbytes,buff);
            		exit(1);
            	}
            	if(!numbytes){
            		fprintf(stdout,"client has closed the connect\n");
            		exit(2);
            	}
            	memcpy(tp,buff,MIN(buffer_left,numbytes)*sizeof(char));//copy all the data received
            	memset(buff,0,_MAX_PACKET_LEN_*sizeof(char));
            	tp+=MIN(buffer_left,numbytes);
       			buffer_left-=MIN(buffer_left,numbytes);
            	bleft-=numbytes;
            }
            if(buffer_left>0){//means bleft=0
            	*tp='\0';
            }
            else{//means buffer_left<0
            	fprintf(stderr,"ivalied client parameter\n");
            	exit(1);
            }
            //the buff has already been filled up, get the whole message
            char procname[PROC_NAME_LENGTH],*procn=NULL,*ttp=NULL;
            int version,port;
            int flag=0;
            char ipname[INET6_ADDRSTRLEN];
    		memset((char*)procname,0,PROC_NAME_LENGTH);
            get_peername(client_fd, ipname, INET6_ADDRSTRLEN, &port);
    		Server* tserver=NULL;
            switch(request_type){
            	case 1://regitster
            		//unpack_stom_reg_msg((char *)workbuff, (char *)procname, &version);
             		ttp=(char*)workbuff+7;
             		printf("\t+type1: add server#\n");
                    //procn=(char*)ipname;
                    //while((*ttp)!='#') *procn++=*ttp++; *procn='\0';
                    sscanf(ttp,"#%d",&port);
                    for(++ttp;*ttp!='#';++ttp); 
					while((*ttp)!='$'){
					 	ttp++;
					 	procn=(char*)procname;
					 	while((*ttp)!='#')	*procn++ = *ttp++;
					 	*procn='\0';
					 	if(!sscanf(ttp,"#%d",&version)){
             				fprintf(stdout,"ttp is empty");
             				exit(2);
             			}
             			if(add_service((char*)procname,version,ipname,port)){
					 		perror("addserver");
					 		sprintf((char*)buff, "$0000#2#0#%s#%d#%s#%d$",(char*)procname,version,ipname,port);
					 		set_pack_len(buff,strlen(buff));
					 		flag=1;
					 		break;
             			}
					 	memset((char*)procname,0,PROC_NAME_LENGTH);
             			++ttp;
             			while((*ttp)!='#' && (*ttp)!='$')	++ttp;
					}
					if(!flag)
					{
						sprintf((char*)buff, "$0000#2#1$");
						set_pack_len(buff,strlen(buff));
						print_mapper();
					}
            		bleft=strlen(buff);
            		while(bleft>0){
	            		if ((numbytes = send(client_fd, buff, strlen(buff)+1,0))==-1)
    						{
        	 					perror("Sent");
         						exit(1);
    						}
    					bleft-=numbytes;
    				}
					break;
            	case 3://lookup
            		//unpack_ctom_qry_msg((char *)workbuff, (char *)procname, &version);
             		ttp=(char*)workbuff+8;
             		printf("\t+type2: lookup a server#\n");
             		printf("type: %s\n",ttp);
					procn=(char*)procname;
					while((*ttp)!='#' && (*ttp)!='$' )	*procn++ = *ttp++; *procn='\0';
					if(!sscanf(ttp,"#%d",&version)){
             			fprintf(stdout,"ttp is empty");
             			exit(2);
             		}
            		printf("\t+lookup for %s %d\n",procname,version);
            		tserver = lookup_service((char*)procname, version);
            		if(!tserver){//find nothing: there is no avail server could help
            			//pack_mtoc_qry_msg((char*)buff, 0, NULL, 0);
            			sprintf((char*)buff, "$0000#0$");
            		}
            		else{//we find one server could help
            			sprintf((char*)buff, "$0000#1#%s#%d$",(char*)(tserver->addstr),tserver->port);
            			//pack_mtoc_qry_msg((char*)buff,1,(char*)(tserver->addstr), tserver->port);
            		}
            		set_pack_len(buff,strlen(buff));
            		bleft=strlen(buff);
            		while(bleft>0){
	            		if ((numbytes = send(client_fd, buff, strlen(buff)+1,0))==-1)
    						{
        	 					perror("Sent");
         						exit(1);
    						}
    					bleft-=numbytes;
    				}
            		break;
            	default://error: 
            		fprintf(stderr,"ivaled type");
            		printf("\tivalued type\n");
            		exit(1);
            }
           	printf("\tworkbuff:=%s\n",workbuff);
            close(client_fd);
			exit(0); 
		}
        close(client_fd);  // parent doesn't need this 
    }
	return 0; 
}