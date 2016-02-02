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
#include "./libsci.h"
#include "raptor_code.h"
using namespace std;

#define _MAX_LENTH_PROCNAME 128
#define MAPPER_IP "127.0.1.1"
#define MAPPER_PORT "3490"
#define PORT "3499"  // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold
#define SLEEPTIME 10 //sleeping time for waiting for msg
//#define MIN(a,b) (a>b?b:a)
#define _MAX_PACKET_LEN_ 512
#define _MAX_WORK_LEN_ 2<<9 //1M space
#define TIMER 10

//public constant
#define SYMBOL_SIZE 500
#define TEST_K 256
#define REC_BUFF_SIZE SYMBOL_SIZE*TEST_K
#define TEST_OVERHEAD 0.3
#define TEST_PR_ARG 0.5

int send_datagram(int sk, char *sendbuff,int lenthofbuf,struct sockaddr *remote, int remotesize, int unid){
    raptor_encoder *encoder = new raptor_encoder(TEST_K, SYMBOL_SIZE,TEST_PR_ARG);
    byte *V=new byte[TEST_K*SYMBOL_SIZE];
    byte sendpacket[_MAX_PACKET_LEN_];
    bzero(V,TEST_K*SYMBOL_SIZE);
    memcpy(V,sendbuff,lenthofbuf);
    byte *charptr=NULL;
    int *intptr=NULL;
    int Pr=0;
    encoder->base_process_encode((byte*)V,TEST_K*SYMBOL_SIZE,&Pr);
    MATRIX_ALLOC(erc,Pr,SYMBOL_SIZE+sizeof(int),byte,0);
    if(encoder->get_err_bit())
    {
        printf("error code = %d\n",encoder->get_err_bit());
    }
    for(int i=0;i<encoder->get_Pr();++i){
         intptr=(int*)sendpacket;
         intptr[0]=unid;
         intptr[1]=0;
         charptr=(char*)(&(intptr[2]));
         memset((char*)charptr,0,_MAX_PACKET_LEN_-(sizeof(int)+sizeof(int)));
         encoder->get_sub_block((byte*)(charptr),i);
         sendto(sk,(char*)sendpacket,_MAX_PACKET_LEN_,0,(struct sockaddr *)remote,remotesize);
    }
    return 0;
}


const proc_llist_item PROC_NAME[]={
    {"multi_matrix",1},
    {"sort",1},
    {"max1",1},
    {"min1",1},
};

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

int create_socket_listen(int *sockfd, struct addrinfo *hints,  const char* target_port){
    int rv,yes=1;
    char s[INET6_ADDRSTRLEN];
    struct addrinfo servinfo, *pservinfo,*p;
    memset(hints, 0, sizeof hints);
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE; // use my IP
    pservinfo=&servinfo;
    if ((rv = getaddrinfo(NULL, target_port, hints, &pservinfo)) != 0) {
    //while if you want to change to another system port, modify PORT to NULLss
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    // loop through all the results and bind to the first we can
    for(p = pservinfo; p != NULL; p = p->ai_next) {
        if ((*sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue; 
        }
        if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            return -1;; 
        }
        if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sockfd);
            perror("server: bind");
            continue; 
        }
        break; 
    }
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }
    freeaddrinfo(pservinfo);
    return 0;
}

int _sendall(int sockfd, char* sndbuff, int lenth){
    int numbytes=0;
    int bleft=lenth;
    while(bleft>0){
        if((numbytes = send(sockfd, sndbuff, bleft, 0))==-1){
            perror("Sent");
            return -1;
        }
        bleft-=numbytes;
    }
    return 0;
}

int create_socket_connect(int *sockfd, struct sockaddr_in *remote, socklen_t *len, char* target_ip, char* target_port){
    struct hostent *hp;
    *sockfd=socket(AF_INET,SOCK_STREAM,0);

    remote->sin_family=AF_INET;
    hp=gethostbyname(target_ip);
    bcopy(hp->h_addr,(char*)(&(remote->sin_addr)),hp->h_length);
    remote->sin_port=atoi(target_port);
    connect(*sockfd,(struct sockaddr *)remote,*len);
    return 0; 
}

int create_socket_connect2(int *sockfd, struct sockaddr_in *local, socklen_t *len){
    *len=sizeof(struct sockaddr_in);
    if((*sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        return -1;
    }
    local->sin_family=AF_INET;
    local->sin_addr.s_addr=INADDR_ANY;
    local->sin_port=0;
    bind(*sockfd,(struct sockaddr *)local,*len);
    /*Find out and publish socket name */
    getsockname(*sockfd,(struct sockaddr *)local,len);
    return 0;
}

int create_socket_dgram(int *sockfd, char* target_port){
    struct  sockaddr_in local;
    int sk;
    socklen_t len=sizeof(local);
    sk=socket(AF_INET,SOCK_DGRAM,0);
    local.sin_family=AF_INET;         /* Define the socket domain   */
    local.sin_addr.s_addr=INADDR_ANY; /* Wild-card, machine address */
    local.sin_port=0;                 /* Let the system assign the port number */   
    bind(sk,(struct sockaddr *)&local,sizeof(local));
    getsockname(sk,(struct sockaddr *)&local,&len);
    sprintf(target_port,"%d",local.sin_port); /* Display port number */
    *sockfd=sk;
    return 0;
}


int main(void)
{
    int i;
    int pid=0;
    int request_type;
    struct sigaction sa;
    int flag=0;
    socklen_t sin_size;
    int skmapper,sockfd, client_fd;  // listen on sock_fd, new connection on client_fd
    struct addrinfo hints;
    struct sockaddr_in local,mremote;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t len=sizeof(local),mlen=sizeof(mremote);
    char tbuff[_MAX_PACKET_LEN_];
    char *workbuff=(char*)tbuff;
    char buff[_MAX_PACKET_LEN_];
    char s[INET6_ADDRSTRLEN];
    int numbytes=0, bleft=0;//recv and sepected bytes in packets
    memset((char*)buff,0,_MAX_PACKET_LEN_);
    FILE* fpip=NULL;
    if((fpip=fopen("./localip","r"))==NULL){
        printf("Fatal error: cannot find the file ip_writer\n");
        exit(1);
    }
    char mapper_ip[INET6_ADDRSTRLEN];
    char mapper_port[INET6_ADDRSTRLEN];
    if(fscanf(fpip,"%s",mapper_ip)==EOF){
         printf("Fatal error: EOF of ip_writer\n");
     }
     if((fscanf(fpip,"%s",mapper_port))==EOF){
         printf("Fatal error: EOF of ip_writer\n");
     }
    fclose(fpip);
    printf("Mapper ip and port: %s:%s\n",mapper_ip,mapper_port);
    /*------------------------Internet Settings Init-------------------------*/
    if(create_socket_connect(&skmapper, &mremote, &len, (char*)mapper_ip, (char*)mapper_port)){
        perror("socket_connect");
        fprintf(stderr,"socket cannot create\n");
        exit(1);
    }
    if(create_socket_connect2(&sockfd, &local, &len)){
        perror("socket_connect server to mapper");
    }
    printf("server port:%d\n",local.sin_port);
    /*---------------------Register to Mapper end---------------------------------------*/
    sprintf((char*)buff, "$0000#1#%d",local.sin_port);
    for(i=0;i<_NUM_PROC_;++i)
    {
//      pack_stom_reg_msg((char*)buff, PROC_NAME[i].procname, PROC_NAME[i].version);
        sprintf((char*)tbuff,"#%s#%d",PROC_NAME[i].procname,PROC_NAME[i].version);
        strcat(buff,tbuff);
        memset(tbuff,0,_MAX_PACKET_LEN_*sizeof(char));
    }
    sprintf((char*)tbuff,"$");
    strcat(buff,tbuff);
    set_pack_len(buff,strlen(buff));
    printf("register info: %s\n",buff);

    if(_sendall(skmapper, (char*)buff, strlen(buff)))
    {
        fprintf(stderr, "Register fail\n");
        exit(-1);
    }
    close(skmapper);
    //register has already been done
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
        printf("server: got connection from %s\n", s);
        //fork another process to deal with all the messages
        if ( (pid = fork())==0 ) { // this is the child process
            close(sockfd); //close listening process
            //receive first message from ourside 
            if((numbytes = recv(client_fd,buff,_MAX_PACKET_LEN_-1,0))==-1){
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
            memcpy((char*)workbuff,(char*)buff,numbytes*sizeof(char));
            memset((char*)buff,0,_MAX_PACKET_LEN_);
            char* tp=(char*)workbuff+numbytes;//point to the next position needed
            *tp = '\0';
            //read lenth message
            if(!sscanf(workbuff,"$%d#%d",&bleft,&request_type)){
                fprintf(stdout,"buff is empty");
                exit(2);
            }
           printf("\t+expected lenth:=%d\n",bleft);
           printf("\t+request_type:=%d\n",request_type);
           printf("\t+workbuff:=%s\n",workbuff);
           printf("\t+Now client transmit about %d\n",numbytes); 
            bleft-=numbytes;//left bytes that is needed to transmission
            //and the former part has already been in the workbuff
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
                fprintf(stderr,"ivalied client parameter");
                flag=1;
                sprintf(buff,"$0000#5#0");
            }
            printf("recv request from client: %s\n",workbuff);
            //the buff has already been filled up, get the whole message
            char* ttp=(char*)workbuff+7;
            printf("\t+type4: calculate\n");
            char target_port[INET6_ADDRSTRLEN];
            create_socket_dgram(&sockfd, (char*)target_port);//this port is used by server, is host port alloced
            if(!flag)
                sprintf(buff,"$0000#5#1#%s$",target_port);
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
            close(client_fd);
            int version=0,unqid=0,paramnum=0;
            char procname[_MAX_LENTH_PROCNAME];
            tp=(char*)procname;
            if(!sscanf(ttp,"#%d",&unqid)){
                return -1;
            }
            for(++ttp;*ttp!='#';++ttp);++ttp;
            while(*ttp!='#') *tp++=*ttp++; *tp='\0';// ++tp;
            if(!sscanf(ttp,"#%d#%d",&version,&paramnum)){
                return -1;
            }
            for(++ttp;*ttp!='#';++ttp);
            for(++ttp;*ttp!='#';++ttp);
            printf("\t+procname:%s procversion:%d paramnum:%d\n",procname, version, paramnum);
            printf("\t+wait for connect....\n");
            printf("\t------------------------------------------------------\n");

            struct  sockaddr_in remote;
            socklen_t rlen=sizeof(remote);
            struct timeval timer,*tvptr;
            int N=(int) (((1+ TEST_OVERHEAD)* TEST_K));
            printf("\t+N = %d\n",N);
            fd_set ready_set;
            int maxfd,nready;
            int* intptr=NULL;
            char *charptr=NULL;
            int state=1;
            MATRIX_ALLOC(erc,N,SYMBOL_SIZE+sizeof(int),byte,0);
            char recvbuff[REC_BUFF_SIZE];
            int toperc=-1;

            FD_ZERO(&ready_set);
            FD_SET(sockfd, &ready_set);
            timer.tv_sec = TIMER;
            timer.tv_usec = 0;
            tvptr = &timer;
            bzero(buff, sizeof(buff));
            maxfd=sockfd;

            while(state){
                nready = select(maxfd+1, &ready_set, NULL, NULL, tvptr);
                switch(nready){
                    case -1:
                        perror("\t+select:unexpected error occured\n");
                        exit(-1);//unexpected system call error
                    case 0:
                        perror("\t+timeout...\n");
                        exit(-2);//time out
                    default:
                        if (FD_ISSET(sockfd, &ready_set))
                        {
                            recvfrom(sockfd,buff,_MAX_PACKET_LEN_,0,(struct sockaddr *)&remote,&rlen);
                            intptr = (int*)buff;
                            //printf("packetcontent = %s\n",buff);
                            if(intptr[0]!=unqid)
                                continue;//not the id we want
                            //printf("toperc = %d\n",toperc);
                            charptr=(char*)(&(intptr[2]));
                            memcpy(erc[++toperc],charptr,SYMBOL_SIZE+sizeof(int));
                            if(toperc==N-1) state=0;
                        }
                }
            }
            //printf("Decode Start");
            int *lost_ESIs=new int[N];
            int lostcount_ESIs;
            raptor_decoder *decoder = new raptor_decoder(TEST_K, SYMBOL_SIZE, TEST_OVERHEAD, TEST_PR_ARG );
            decoder->process_decode((byte **)erc, N, recvbuff, TEST_K*SYMBOL_SIZE, lost_ESIs, &lostcount_ESIs);
            if(decoder->get_err_bit())
            {
                printf("\t+Decode fail\n");
                decoder->clear_err_bit();
                exit(-1);//decode fail
            }
            //printf("raptor code return: %s\n",recvbuff);
            int *array=(int*)recvbuff;
            int buff_matrix[REC_BUFF_SIZE];
            int ans=0;
            int vret=0;
            //determine which process need to be called
            int i;
            int* pmatrix=array;
            int m,n,l;
            for(i=0;(i<_NUM_PROC_)&&(strcmp(PROC_NAME[i].procname,procname)!=0)&&PROC_NAME[i].version;++i);
            switch(i){
                case 0://multi_matrix
                        printf("\t+cal: %s\n",PROC_NAME[0].procname);
                        m=pmatrix[0]; n=pmatrix[1]; l=pmatrix[2];
                        printf(" \t+m=%d, n=%d, l=%d\n",m,n,l);
                        pmatrix+=3;
                        multi_matrix(m, n, l, (int*)pmatrix, (int*)buff_matrix);
                        memcpy((char*)array,(char*)buff_matrix,REC_BUFF_SIZE);
                        break;
                case 1://sort
                        printf("\t+cal: %s\n",PROC_NAME[1].procname);
                        sort((int *)array, paramnum);
                        break;
                case 2://max1
                        printf("\t+cal: %s\n",PROC_NAME[2].procname);
                        vret=max1((int *)array,&ans,paramnum);
                        array[0]=vret;
                        array[1]=ans;
                        break;
                case 3://min1
                        printf("\t+cal: %s\n",PROC_NAME[3].procname);
                        vret=min1((int *)array,&ans,paramnum);
                        array[0]=vret;
                        array[1]=ans;
                        break;
                default://error
                        printf("\t+cal error\n");
                        break;
            }
            printf("\t+Return from server\n");
            send_datagram(sockfd,(char*)recvbuff,paramnum*sizeof(int),(struct sockaddr *)&remote,sizeof(remote),unqid);
            close(sockfd);
            exit(0);
        }
        close(client_fd);  // parent doesn't need this
    }
    close(sockfd);
    return 0; 
}
