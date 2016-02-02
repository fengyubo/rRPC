#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MAPPER_H_
#define _MAPPER_H_
#define ADD_STR_LENGTH 20
#define PROC_NAME_LENGTH 50
#define SIZE_OF_MAPPER 50
#define MAX_SERVER_NODE 1024
#define _BLOCK_SIZE_ (sizeof(unsigned long long)<<3)
#define MAX_BITMAP_SIZE ((MAX_SERVER_NODE/sizeof(unsigned long long))>>3)
//SETBITMAP & CLEARBITMAP are used to set/clear accordinated bit in bitmap 
#define SETBITMAP(pos,_base_map) do{ (_base_map[pos/_BLOCK_SIZE_]) |= (((unsigned long long)1)<<(pos%_BLOCK_SIZE_));}while(0)
#define CLEARBITMAP(pos,_base_map) do{ (_base_map[pos/_BLOCK_SIZE_]) &= (~(((unsigned long long)1)<<(pos%_BLOCK_SIZE_)));}while(0)

typedef struct server
{
	char addstr[ADD_STR_LENGTH];
	int port;
	int next;

}Server;

typedef struct mapper_item
{
	char proc_name[PROC_NAME_LENGTH];
	int proc_version;
	int server_list;
	int next_avail_server;
}Mapper_item;

int add_service(char proc_name[ADD_STR_LENGTH], int proc_version, char addstr[PROC_NAME_LENGTH], int port);
//int add_service();
void initial_mapper();
int delete_service();
Server* lookup_service(char proc_name[ADD_STR_LENGTH], int proc_version);
//Server* lookup_service();
void print_mapper();
#endif /* _MAPPER_H_ */
