#include <stdio.h>
#include <stdlib.h>
#define _NUM_PROC_ 4
typedef struct _proc_llist_item
{
	const char *procname;
	const int version;
}proc_llist_item;

void multi_matrix(int m, int n, int l, int* vmatrix, int* ans_matrix);
void sort(int *ary, int ary_len);
int comp(const void * elem1, const void * elem2);
int max1(int *ans,int *rp,int lenth);
int min1(int *ans,int *rp,int lenth);