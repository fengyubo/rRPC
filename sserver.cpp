#include <stdio.h>
#include <fstream>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include "raptor_code.h"
using namespace std;

#define _MAX_LENTH_PROCNAME 128
#define SSERVER_PORT "3971"
#define LENTH_UPACKET 512
#define TIMER 10
#define CONTINUE 1

//public constant
#define SYMBOL_SIZE 500
#define TEST_K 128
#define REC_BUFF_SIZE SYMBOL_SIZE*TEST_K
#define TEST_OVERHEAD 0.3
#define TEST_PR_ARG 0.5

int recv_datagram_timer(int N, int unid, char *port, char* recvbuff )
{
	struct	sockaddr_in local,remote;
	int	sk,len=sizeof(local);
	int     status;
	socklen_t fromlen=sizeof(remote);
	char	buf[LENTH_UPACKET];
	struct timeval before;
	struct timeval timer;
	struct timeval *tvptr;
	struct timezone tzp;
	fd_set ready_set;
	fd_set test_set;
	int maxfd;
	int nready;
	int nbytes;	
	sk=socket(AF_INET,SOCK_DGRAM,0);
	maxfd = sk;
 	FD_ZERO(&ready_set);
  	FD_ZERO(&test_set);
  	FD_SET(sk, &test_set);
	timer.tv_sec = TIMER;
  	timer.tv_usec = 0;
  	tvptr = &timer;

	local.sin_family=AF_INET;         /* Define the socket domain   */
	local.sin_addr.s_addr=INADDR_ANY; /* Wild-card, machine address */
	local.sin_port=atoi(port);              
	bind(sk,(struct sockaddr *)&local,sizeof(local));
	gettimeofday(&before, &tzp);
	int* intptr=NULL;
	char* charptr=NULL;
  	status=CONTINUE;
  	MATRIX_ALLOC(erc,N,SYMBOL_SIZE+sizeof(int),byte,0);
  	int toperc=-1;
  	while(status==CONTINUE){
  		  	read(sk,buf,LENTH_UPACKET);//,0,(struct sockaddr *)&remote,&fromlen);
	intptr = (int*)buf;
	printf("packetcontent = %s\n",buf);
	if(intptr[0]!=unid)
		continue;//no the id we want
	charptr=(char*)(&(intptr[2]));
	memcpy(erc[++toperc],charptr,SYMBOL_SIZE+sizeof(int));
	if(toperc==N)
		status=-1;
  	}
  	 // while (status==CONTINUE)
   // 	{
   //  	memcpy(&ready_set, &test_set, sizeof(test_set));
   //    	nready = select(maxfd+1, &ready_set, NULL, NULL, tvptr);
   //     	{
   //          switch(nready)
   //          {
   //           	case -1:
			//         perror("\nSELECT: unexpected error occured " );
   //                  return -1;//unexpected system call error
   //                  break;
   //             	case 0:
			// 		return -2;//time out
			// 		break;
			// 	break;
			// 	default:
   //                  if (FD_ISSET(sk, &ready_set))
   //                  {
			// 			recvfrom(sk,buf,LENTH_UPACKET,0,(struct sockaddr *)&remote,&fromlen);
			// 			intptr = (int*)buf;
			// 			printf("packetcontent = %s\n",buf);
			// 			if(intptr[0]!=unid)
			// 				continue;//no the id we want
			// 			charptr=(char*)(&(intptr[2]));
			// 			memcpy(erc[++toperc],charptr,SYMBOL_SIZE+sizeof(int));
			// 			if(toperc==N)
	  //                       status=-1;
   //                  }
			// }
   //     }
   // }
    int *lost_ESIs=new int[N];
	int lostcount_ESIs;
    raptor_decoder *decoder = new raptor_decoder(TEST_K, SYMBOL_SIZE, TEST_OVERHEAD, TEST_PR_ARG );
	decoder->process_decode((byte **)erc, N, recvbuff, TEST_K*SYMBOL_SIZE, lost_ESIs, &lostcount_ESIs);
	if(decoder->get_err_bit())
	{
		perror("Decode fail");
		decoder->clear_err_bit();
		return -3;//decode fail
	}
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

int main(int argc, char* argv[])
{
//info format: sserver 127.0.0.1 3610 #923#sort#1#12$ 
	for(int i=0;i<argc;++i){
		printf("%s ",argv[i]);
	}
	printf("\n");
	
	int unqid=0;
	char procname[_MAX_LENTH_PROCNAME];
	int version=0;
	int paramnum=0;
	int groupnum=0;
	char *cip=argv[1];
	char *cport=argv[2];//client port
	char *sport=argv[3];//server port needed to be created
	char* tp=argv[4];
	char *ttp=(char*)procname;
	if(!sscanf(argv[4],"#%d",&unqid)){
		return -1;
	}
	for(++tp;*tp!='#';++tp);++tp;
	while(*tp!='#')	*ttp++=*tp++; *ttp='\0';// ++tp;
	if(!sscanf(tp,"#%d#%d",&version,&paramnum)){
		return -1;
	}
	printf("procname:%s\nprocversion:%d\nparamnum:%d\nclient ip:%s\nclientport:%s\nserverport:%s\n",
			procname, version, paramnum, cip,cport, sport);
	char recvbuff[REC_BUFF_SIZE];
	int N=(int) (((1+ TEST_OVERHEAD)* TEST_K));
	printf("unqid=%d\n",unqid);
	int ret = recv_datagram_timer(N, unqid, sport, (char*)recvbuff);
	switch(ret){
		case -1:
			printf("port error\n");
			break;
		case -2:
			printf("time out error\n");
			break;
		case -3:
			printf("decode error\n");
		default:
			printf("decode succ");
	}


	return 0;
}