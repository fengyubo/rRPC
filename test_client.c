#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>
#include <math.h>
#include "raptor_code.h"

#define PORT "3610" // the port client will be connecting to
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define MAPPER_IP "127.0.1.1"
#define MAPPER_PORT "3490"
#define LENTH_UPACKET 512
#define TIMER 20

#define SYMBOL_SIZE 500
#define TEST_K 256
#define REC_BUFF_SIZE SYMBOL_SIZE*TEST_K //max:125K
#define TEST_OVERHEAD 0.3
#define TEST_PR_ARG 0.5

//sizeof sendbuff: SYMBOL_SIZE+sizeof(int)
//hipaddr is dest addr, then hport is port number of the other end
int send_datagram(int sk, char *sendbuff,int lenthofbuf,struct sockaddr *remote, int remotesize, int unid){
    raptor_encoder *encoder = new raptor_encoder(TEST_K, SYMBOL_SIZE,TEST_PR_ARG);
    byte *V=new byte[TEST_K*SYMBOL_SIZE];
    byte sendpacket[LENTH_UPACKET];
    bzero(V,TEST_K*SYMBOL_SIZE);
    memcpy(V,sendbuff,lenthofbuf);
    byte *charptr=NULL;
    int *intptr=NULL;
    //printf("sendbuff = %s\n",sendbuff);
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
         memset((char*)charptr,0,LENTH_UPACKET-(sizeof(int)+sizeof(int)));
         encoder->get_sub_block((byte*)(charptr),i);
         sendto(sk,(char*)sendpacket,LENTH_UPACKET,0,(struct sockaddr *)remote,remotesize);
    }
    return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }   
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void set_pack_len(char *buff,const int lenth){//the buff has already set everthing except for the lenth flag,
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

int create_socket_oldstyle(int *sockfd, struct sockaddr_in *remote, socklen_t *len, char* target_ip, char* target_port){
    struct hostent *hp;
    *sockfd=socket(AF_INET,SOCK_STREAM,0);

    remote->sin_family=AF_INET;
    hp=gethostbyname(target_ip);
    bcopy(hp->h_addr,(char*)(&(remote->sin_addr)),hp->h_length);
    remote->sin_port=atoi(target_port);
    connect(*sockfd,(struct sockaddr *)remote,*len);
    return 0; 
}

int create_socket_connect(int *sockfd, struct addrinfo *hints, const char* target_ip, const char* target_port){
    int rv;
    struct addrinfo servinfo, *pservinfo,*p;
    char s[INET6_ADDRSTRLEN];
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    pservinfo = &servinfo;
    if ((rv = getaddrinfo(target_ip, target_port, hints, &pservinfo)) != 0) {
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
        }//connect with the mapper in order to register with info
        if (connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sockfd);
            perror("server: connect");
            continue;
        }
        break; 
    }
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("\tclient: connecting to %s\n", s);
    freeaddrinfo(pservinfo); // all done with this structure 
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

int test_cstub(int* _sortarray, int lentofsort, const char* calname, const int calv, int *errorbit,int unicid, int reverse){
    int rv;
    int skmapper,sockfd, numbytes, udpsk;
    char buf[REC_BUFF_SIZE];

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in local,mremote;
    socklen_t mlen=sizeof(mremote);
    char s[INET6_ADDRSTRLEN];
    char name[INET6_ADDRSTRLEN];
    int port;
    int *sortarray = _sortarray;
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
    //connect with the mapper

    if(create_socket_oldstyle(&skmapper, &mremote, &mlen, (char*)mapper_ip, (char*)mapper_port)){
        perror("socket_connect");
        fprintf(stderr,"socket cannot create\n");
        exit(1);
    }
    /*---------------------request for Mapper end---------------------------------------*/
    //sprintf(buf,"$0000#1#add#1#sub#2$");
    sprintf(buf,"$0000#3#%s#%d$",calname,calv);
    set_pack_len((char*)buf,strlen(buf));
    //printf("%s lenth = %d\n",buf,(int)strlen(buf));
    if ((numbytes = send(skmapper, buf, strlen(buf)+1,0))==-1)
    {
         perror("Sent");
         *errorbit=2;
         return -1;
    }

    if ((numbytes = recv(skmapper, buf, REC_BUFF_SIZE-1, 0)) == -1) {
        perror("recv");
        *errorbit=3;
        return -1;
    }
    buf[numbytes] = '\0';
    printf("\tclient: received '%s'\n",buf);
    close(skmapper);

    /*---------------------Stract server IP and Port-------------------------------------*/
    char* tp=(char*)buf;
    int lenth=0;
    int succ=0;
    char ipaddr[INET_ADDRSTRLEN];
    char tarport[INET_ADDRSTRLEN];
    //struct sockaddr_in sa;

    sscanf(tp,"$%d#%d",&lenth,&succ);
    if(!succ){
        *errorbit=4;
        return -1;
    }
    tp+=7;
    char *ttp=(char*)ipaddr;
    for(++tp;*tp!='#';)     *ttp++=*tp++;   *ttp='\0';
    ttp=(char*)tarport;
    for(++tp;*tp!='$';) *ttp++=*tp++;   *ttp='\0';
    printf("\tClient: target IP&port#: %s:%s\n",ipaddr,tarport);
    //inet_pton(AF_INET, (char*)ipaddr, &(sa.sin_addr));
    /*-------------------construct msg sent to server @the TCP contact------------------- */
    memset((char*)(&mremote),0,sizeof(mremote));
    if(create_socket_oldstyle(&sockfd, &mremote, &mlen, (char*)ipaddr, (char*)tarport)){
        perror("socket_connect");
        fprintf(stderr,"socket cannot create\n");
        *errorbit=1;
        return -1;
    }
    bzero(buf,REC_BUFF_SIZE);
    sprintf(buf,"$0000#4#%d#%s#%d#%d$",unicid,calname,calv,lentofsort);
    set_pack_len((char*)buf,strlen(buf));
    printf("\tClient: sent: %s\n",buf);
    //communicate with server: send the msg out to server, 
    //tell what client want and the port that should be communicate
    if ((numbytes = send(sockfd, buf, strlen(buf)+1,0))==-1)
    {
         perror("Sent");
         *errorbit=2;
        return -1;
    }

    if ((numbytes = recv(sockfd, buf, REC_BUFF_SIZE-1, 0)) == -1) {
        perror("recv");
        *errorbit=3;
        return -1; 
    }
    buf[numbytes] = '\0';
    printf("\tClient: received '%s'\n",buf);
    close(sockfd);
    ttp=(char*)buf;ttp+=8;
    sscanf(ttp,"#%d",&succ);
    if(!succ){
        *errorbit=4;
        return -1;
    }
    ttp+=1;
    int sport=0;
    sscanf(ttp,"#%d",&sport);
    sprintf((char*)tarport,"%d",sport);printf("\tClient:target port num: %d\n",sport);
    struct  sockaddr_in remote;
    struct  hostent *hp,*gethostname();
    int sk=socket(AF_INET,SOCK_DGRAM,0);
    remote.sin_family=AF_INET;
    hp=gethostbyname(ipaddr);
    bcopy(hp->h_addr,&remote.sin_addr.s_addr,hp->h_length);
    remote.sin_port=atoi(tarport);
    bzero(buf,REC_BUFF_SIZE);
    memcpy(buf,(char*)sortarray,lentofsort*sizeof(int));
    send_datagram(sk,(char*)buf,lentofsort*sizeof(int),(struct sockaddr *)&remote,sizeof(remote),unicid);
    //time control: recv result
    struct timeval timer,*tvptr;
    int N=(int) (((1+ TEST_OVERHEAD)* TEST_K));
    fd_set ready_set;
    int maxfd,nready;
    int* intptr=NULL;
    char *charptr=NULL;
    int state=1;
    MATRIX_ALLOC(erc,N,SYMBOL_SIZE+sizeof(int),byte,0);
    char recvbuff[REC_BUFF_SIZE];
    int toperc=-1;

    FD_ZERO(&ready_set);
    FD_SET(sk, &ready_set);
    timer.tv_sec = TIMER;
    timer.tv_usec = 0;
    tvptr = &timer;
    maxfd=sk;
    //printf("N = %d\n",N);
    while(state){
        nready = select(maxfd+1, &ready_set, NULL, NULL, tvptr);
        switch(nready){
            case -1:
                perror("select:unexpected error occured\n");
                *errorbit=5;
                return -1;
            case 0:
                perror("timeout...\n");
                *errorbit=5;
                return -1;
            default:
                if (FD_ISSET(sk, &ready_set))
                {
                    //recvfrom(sk,buf,LENTH_UPACKET,0,(struct sockaddr *)&remote,&rlen);
                    read(sk,buf,LENTH_UPACKET);
                    intptr = (int*)buf;
                    //printf("packetcontent = %s\n",buff);
                    if(intptr[0]!=unicid)
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
    decoder->process_decode((byte **)erc, N, (char*)buf, TEST_K*SYMBOL_SIZE, lost_ESIs, &lostcount_ESIs);
    if(decoder->get_err_bit())
    {
        printf("Decode fail\n");
        decoder->clear_err_bit();
        *errorbit=6;
        return -1;
    }
    memcpy((char*)sortarray,buf,lentofsort*sizeof(int));
    // for(int i=0;i<lentofsort;++i)
    //     printf("%d\n",sortarray[i]);
    close(sk);
    return  0;
}

int max(int *array,int arraylenth,int *ans){
    int errorbit=0;
    int unqid = 0;
    srand(time(NULL));
    unqid = rand()%getpid();
    int *buff = (int*)malloc(arraylenth*sizeof(int));
    if((buff=(int*)malloc(arraylenth*sizeof(int)))==NULL){
        perror("error system alloc");
        return -1;
    }
    memcpy(buff, array,arraylenth*sizeof(int));
    if(test_cstub((int*)buff, arraylenth, "max1", 1, &errorbit, unqid,0))
        printf("error = %d\n",errorbit);
    memcpy(array,buff,arraylenth*sizeof(int));
    if(buff!=NULL) free(buff);
    if(array[0]==-1)
        return -1;
    *ans=array[1];
    return 0;
}

int min(int *array,int arraylenth,int *ans){
    int errorbit=0;
    int unqid = 0;
    srand(time(NULL));
    unqid = rand()%getpid();
    int *buff = (int*)malloc(arraylenth*sizeof(int));
    if((buff=(int*)malloc(arraylenth*sizeof(int)))==NULL){
        perror("error system alloc");
        return -1;
    }
    memcpy(buff, array,arraylenth*sizeof(int));
    if(test_cstub((int*)buff, arraylenth, "min1", 1, &errorbit, unqid,0))
        printf("error = %d\n",errorbit);
    memcpy(array,buff,arraylenth*sizeof(int));
    if(buff!=NULL) free(buff);
    if(array[0]==-1)
        return -1;
    *ans=array[1];
    return 0;
}


void sort(int *array,int arraylenth){
    //we have to copy the array, cause probably user only want to sort subset(prefix) elements in array 
    int errorbit=0;
    int unqid = 0;
    srand(time(NULL));
    unqid = rand()%getpid();
    if(test_cstub((int*)array, arraylenth, "sort", 1, &errorbit, unqid,0))
        printf("error = %d\n",errorbit);
    return ;
}

int  mulmatrix(int *m1, int m, int n, int* m2, int p, int q, int* m3, int w, int r){
    int errorbit=0;
    int unqid = 0;
    int arraylenth=3+m*n+p*q;
    if( n!=p || w!=m || r!=q )
        return -1;
    if(arraylenth>REC_BUFF_SIZE)
        return -2;
    srand(time(NULL));
    unqid = rand()%getpid();
    int _array[REC_BUFF_SIZE];
    _array[0]=m;_array[1]=n;_array[2]=q;
    int *array=&(_array[3]);
    memcpy(array,m1,n*m*sizeof(int));
    array+=n*m;
    memcpy(array,m2,p*q*sizeof(int));
    test_cstub((int*)_array, arraylenth, "multi_matrix", 1, &errorbit, unqid,0);
    if(errorbit)
        printf("error = %d\n",errorbit);
    memcpy(m3,_array,r*w*sizeof(int));
    return 0;
}

int main(int argc, char*argv[])
{
    int sortarray[20]={1,2,3,4,5,12,11,99,654,555,67,43,345,14,15,53,1111,656,393,222};
    int m1[10000]={1};
    int m2[10000]={1};
    int m3[10000]={1};
    int time1 = clock();
    int errorbit=0;
    int ans=0;
    int i;
    for(i=0;i<10000;i++){
        m1[i]=1;
        m2[i]=1;
    }
    ans = mulmatrix((int*)m1,100,100,(int*)m2,100,100,(int*)m3,100,100);
    int time2 = clock();
    printf("%d\n",ans);
    for(i=0;i<100;++i){
        for(int j=0;j<100;++j)
            printf("%d ",m3[i*100+j]);
        printf("\n");  
    }
    printf("time = %d\n",time2-time1);
    return 0;
}

