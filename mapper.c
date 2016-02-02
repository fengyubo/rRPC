
#include "mapper.h"
//-------------------------------------------------------
extern unsigned long long* page_bitmap;
extern Server* seg_heap;
extern Mapper_item* Mapper;
//----------------------------Malloc and Dealloc---------------------------------//
//func of malloc and free, which works on heap
//_base_addr is heap address, not bit_map address
int _mmalloc(unsigned long long* pgbitmap, int len_bitmap){
	int index=0,offset=0;
	int tmp;
	for(index=0; (index<len_bitmap) && (pgbitmap[index]==0xffffffffffffffff); ++index);//space for new request
	if(index==len_bitmap)
		return -1;
	for(offset=0,tmp=pgbitmap[index]; (offset<64) && ((tmp>>offset)&(unsigned long long)0x1); ++offset);
	SETBITMAP(offset,pgbitmap);
	return index*_BLOCK_SIZE_+offset;
}

int _mmfree(int _free_addr, unsigned long long* pgbitmap){
	if( _free_addr==-1)
		return -1;
	CLEARBITMAP(_free_addr, pgbitmap);
	return 0;
}
//-------------------------------------------------------------------------------//

void initial_mapper()
{
	int i;
	for(i=0 ; i<=SIZE_OF_MAPPER-1 ; i++)
	{
		//initial_char_array(Mapper[i].proc_name);
		memset(Mapper[i].proc_name, 0, PROC_NAME_LENGTH);
		Mapper[i].proc_version = -1;
		Mapper[i].server_list = -1;
		Mapper[i].next_avail_server = -1;
	}
	//memset(Mapper, '0', sizeof(Mapper)*SIZE_OF_MAPPER); ?
}



int add_service(char proc_name[ADD_STR_LENGTH], int proc_version, char addstr[PROC_NAME_LENGTH], int port)
{
	int i;

	//the procedure already exists
	for(i=0 ; i<=SIZE_OF_MAPPER-1 ; i++)
	{
		if(Mapper[i].proc_name[0] != '\0' &&
		   !strcmp(proc_name, Mapper[i].proc_name) &&
		   Mapper[i].proc_version == proc_version)
		{
			//proc's new server
			int server_index = _mmalloc(page_bitmap,MAX_BITMAP_SIZE);
			Server* new_server = &(seg_heap[server_index]);
			strcpy(new_server->addstr, addstr);
			new_server->port = port;
			new_server->next = -1;

			//add the new server to the end
			int index_server_ptr = Mapper[i].server_list;
			for( ; seg_heap[index_server_ptr].next != -1 ; index_server_ptr =  seg_heap[index_server_ptr].next);
			seg_heap[index_server_ptr].next = server_index;

			return 0;
		}
	}

	//new procedure
	for(i=0 ; i<=SIZE_OF_MAPPER-1 ; i++)
	{
		if (Mapper[i].proc_name[0] == '\0')
		{
			//new mapper item
			strcpy(Mapper[i].proc_name, proc_name);
			Mapper[i].proc_version = proc_version;

			//proc's new server
			int server_index = _mmalloc(page_bitmap,MAX_BITMAP_SIZE);
			Server* new_server = &(seg_heap[server_index]);
			strcpy(new_server->addstr, addstr);
			new_server->port = port;
			new_server->next = -1;

			//add new server
			Mapper[i].server_list = server_index;

			//next available server
			Mapper[i].next_avail_server = server_index;

			return 0;
		}
	}

	//fail to add
	return -1;
}


int delete_service(char proc_name[ADD_STR_LENGTH], int proc_version, char addstr[PROC_NAME_LENGTH], int port)
{
	int i;
	for(i=0 ; i<=SIZE_OF_MAPPER-1 ; i++)
	{
		if(Mapper[i].proc_name[0] != '\0' &&
		   !strcmp(proc_name, Mapper[i].proc_name) &&
		   Mapper[i].proc_version == proc_version)
		{
			int head = Mapper[i].server_list;
			int cur_server = Mapper[i].server_list;
			int pre_server = cur_server;
			for( ; cur_server != -1 ; pre_server = cur_server, cur_server=seg_heap[cur_server].next)
			{
				if(!strcmp(seg_heap[cur_server].addstr,addstr) &&
					seg_heap[cur_server].port == port)
				{
					if(cur_server == head)
					{
						Mapper[i].server_list = seg_heap[head].next;
						_mmfree(cur_server, page_bitmap);
					}
					else
					{
						seg_heap[pre_server].next = seg_heap[cur_server].next;
						_mmfree(cur_server, page_bitmap);
					}

					//if server list is empty, delete the mapper item
					if(Mapper[i].server_list == -1)
					{
						memset(Mapper[i].proc_name, 0, PROC_NAME_LENGTH);
						Mapper[i].proc_version = -1;
						Mapper[i].server_list = -1;
						Mapper[i].next_avail_server = -1;
						//memset(Mapper[i], 0, sizeof(Mapper));
					}
					return 0;
				}
			}
		}
	}

	return -1;
}


Server* lookup_service(char proc_name[ADD_STR_LENGTH], int proc_version)
{
	int i;
	Server* target=NULL;

	for(i=0 ; i<=SIZE_OF_MAPPER-1 ; i++)
	{
		if(Mapper[i].proc_name[0] != 0 &&
		   !strcmp(proc_name, Mapper[i].proc_name) &&
		   Mapper[i].proc_version == proc_version)
		{
			target = &(seg_heap[Mapper[i].next_avail_server]);

			//update next available server
			//the last server in list
			if(seg_heap[Mapper[i].next_avail_server].next == -1)
				Mapper[i].next_avail_server = Mapper[i].server_list;
			//not the last in list
			else
				Mapper[i].next_avail_server = seg_heap[Mapper[i].next_avail_server].next;
			break;
		}
	}

	return target;
}


void print_mapper()
{
	int i;
	int sPtr;
	printf("\tMapper: \n");
	for(i=0 ; i<=SIZE_OF_MAPPER-1 ; i++)
	{
		if(Mapper[i].proc_name[0] != '\0')
		{
			printf("\titem[%d]: %s:%d: ", i+1, Mapper[i].proc_name, Mapper[i].proc_version);
			for(sPtr=Mapper[i].server_list ; sPtr !=-1 ; sPtr = seg_heap[sPtr].next)
			{
				printf("\t\t%s, %d %d \n", seg_heap[sPtr].addstr, seg_heap[sPtr].port,sPtr);
			}
		}
	}
}

