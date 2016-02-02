#include <stdio.h>
#include "mapper.h"
char  _page_bitmap[MAX_BITMAP_SIZE*sizeof(unsigned long long)]={0};//manage space
char _seg_heap[MAX_SERVER_NODE*sizeof(Server)];//tiny heap segment 


int main()
{
	int i;
	char procName[50] = "add";
	char procName2[50] = "add";
	int version = 1;
	int version2 = 1;
	char ip[20] = "1.1.1.1";
	char ip2[20] = "2.2.2.2";
	int port = 11111;
	int port2 = 22222;
	Server *dest = NULL;


	initial_mapper();
	printf("initial succeed!!");

	printf("add service:\n");

	//scanf("%s,%d,%s,%d",procName,&version,ip,&port);
	if(add_service(procName,version,ip,port) == 0)
		printf("add 1 succeed!!\n");
	else
		printf("add 1 failed\n");

	if(add_service(procName2,version2,ip2,port2) == 0)
			printf("add 2 succeed!!\n");
		else
			printf("add 2 failed\n");

	print_mapper();

	for(i=0 ; i<=2 ; i++)
	{
	dest = lookup_service(procName,version);
	if(dest !=NULL)
		printf("lookup available for add: IP %s, port %d\n",dest->addstr, dest->port);
	}


	if(delete_service(procName2,version2,ip2,port2) == 0)
		printf("delete succeed!!\n");
	else
		printf("delete failed\n");

	print_mapper();

	/*if(delete_service(procName,version,ip,port) == 0)
			printf("delete succeed!!\n");
		else
			printf("delete failed\n");

	print_mapper();*/

	return 0;
}

