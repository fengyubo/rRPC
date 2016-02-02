#include "libsci.h"
#define _NUM_PROC_ 4

extern const proc_llist_item PROC_NAME[];

//mxn * n*l
void multi_matrix(int m, int n, int l, int* vmatrix, int* ans_matrix){
	int tmp;
	int *A=vmatrix;
	int *B=vmatrix+m*n;
	int *C=ans_matrix;
	int i,j,k;
	int line=0;
	for(i=0;i<m;++i){
		line=i*n;
		for(j=0;j<l;++j){
		tmp=0;
			for(k=0;k<n;++k){
				tmp+=A[line+k]*B[k*l+j];
			}
		C[i*l+j]=tmp;
		}
	}
	return ;
}

int comp(const void * elem1, const void * elem2) {
	int f = *((int*) elem1);
	int s = *((int*) elem2);
	if (f > s)
		return -1;
	if (f < s)
		return 1;
	return 0;
}

//Sort Descending
void sort(int *ary, int ary_len) {
	qsort(ary, ary_len, sizeof(*ary), comp);
}

int max1(int *ans,int *rp,int lenth){
	if((!ans)||(!rp)||(lenth<0))
		return -1;
	if(!lenth)	return 0;
	int max=ans[0];
	int i=0;
	for(i=1;i<lenth;++i){
		if(max<ans[i])
			max=ans[i];
	}
	*rp=max;
	return 0;
}

int min1(int *ans,int *rp,int lenth){
	if((!ans)||(!rp)||(lenth<0))
		return -1;
	if(!lenth)	return 0;
	int min=ans[0];
	int i=0;
	for(i=1;i<lenth;++i){
		if(min>ans[i])
			min=ans[i];
	}
	*rp=min;
	return 0;
}
