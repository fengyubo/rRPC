#include "raptor_code.h"
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>

raptor_encoder::raptor_encoder(int _K, int _SYMBOL_SIZE, double PR_PRECPENT)
{
	K = _K;
	if( (K>K_MAX) || (K<K_MIN) )
	{
		error=7;
		K=K_RECOMMEND;
	}
	SYMBOL_SIZE = _SYMBOL_SIZE;
	BLOCK_SIZE = K*SYMBOL_SIZE;
	error=0;
	start=0;
	finish=0;
	int X=1;
	int i=0;
	while((X<combination_cn2_size)&(combination_cn2_set[X]<K))	++X;
	for(S=1;S<ceil(0.01*K)+X;++S)	;
	for(i=0;(i<prime_size)&(prime_numset[i]<S);++i)	;	S=prime_numset[i];	
	for(H=1;(H<combination_size)&(combination_numset[H]<K+S);++H)	;	
	L=K+S+H;//The most important statement: define the lenth of L, which are used to construct the code vector
	for(Lp=L,i=0;(i<prime_size)&(prime_numset[i]<Lp);++i); Lp=prime_numset[i];
	Pr=(int) ((1+PR_PRECPENT)*K);//redundancy of single sending packets, rate is Redunance
	mtr_op=new elim_stack[L*L];// this is a max number of matrix opr
	mtr_op_size=0;
	MATRIX_ALLOC_NODEF(U,_TRIPLE_ROW_,Pr,int,0);
	MATRIX_ALLOC(A,L,INT_CEIL64(L),long long,0);
	MATRIX_ALLOC_NODEF(source_block,K,SYMBOL_SIZE,byte,0);
	MATRIX_ALLOC_NODEF(des_block,Pr,SYMBOL_SIZE,byte,0);
	TripleCalculate(K, Lp, Pr,(int**)U);
	ppscalculate(K, H, L, Lp, S, Pr,(long long**)A, (int**)U);
	Inversecalculate( (long long **)A, L, H, K, S, (elim_stack*)mtr_op, &mtr_op_size);
	MATRIX_DEL(A,L);
}

raptor_encoder::~raptor_encoder()
{
	MATRIX_DEL(U,_TRIPLE_ROW_);
	SAFE_DELETE_ARR(mtr_op);
	MATRIX_DEL(source_block,K);
	MATRIX_DEL(des_block,Pr);
}

void raptor_encoder::reset_raptor(int _K, int _SYMBOL_SIZE, double PR_PRECPENT)
{
	MATRIX_DEL(U,_TRIPLE_ROW_);
	SAFE_DELETE_ARR(mtr_op);
	MATRIX_DEL(source_block,K);
	MATRIX_DEL(des_block,Pr);
	K = _K;
	if( (K>K_MAX) || (K<K_MIN) )
	{
		error=7;
		K=K_RECOMMEND;
	}
	SYMBOL_SIZE = _SYMBOL_SIZE;
	BLOCK_SIZE = K*SYMBOL_SIZE;
	error=0;
	start=0;
	finish=0;
	int X=1;
	int i=0;
	while((X<combination_cn2_size)&(combination_cn2_set[X]<K))	++X;
	for(S=1;S<ceil(0.01*K)+X;++S)	;
	for(i=0;(i<prime_size)&(prime_numset[i]<S);++i)	;	S=prime_numset[i];	
	for(H=1;(H<combination_size)&(combination_numset[H]<K+S);++H)	;	
	L=K+S+H;//The most important statement: define the lenth of L, which are used to construct the code vector
	for(Lp=L,i=0;(i<prime_size)&(prime_numset[i]<Lp);++i); Lp=prime_numset[i];
	Pr=(int) ((1+PR_PRECPENT)*K);//redundancy of single sending packets, rate is Redunance
	mtr_op=new elim_stack[L*L];
	mtr_op_size=0;
	MATRIX_ALLOC_NODEF(U,_TRIPLE_ROW_,Pr,int,0);
	MATRIX_ALLOC(A,L,INT_CEIL64(L),long long,0);
	MATRIX_ALLOC_NODEF(source_block,K,SYMBOL_SIZE,byte,0);
	MATRIX_ALLOC_NODEF(des_block,Pr,SYMBOL_SIZE,byte,0);
	TripleCalculate(K, Lp, Pr,(int**)U);
	ppscalculate(K, H, L, Lp, S, Pr,(long long**)A, (int**)U);
	Inversecalculate( (long long **)A, L, H, K, S, (elim_stack*)mtr_op, &mtr_op_size);
	MATRIX_DEL(A,L);
	return ;
}

void raptor_encoder::base_process_encode(byte *source_buffer, int lenth_buffer, int *count_of_packets)
{//recommend: lenth_buffer <= K x SYMBOL_SIZE, dealing with former K x SYMBOL_SIZE bytes 
	byte *p_buffer=(byte*)source_buffer;
	long long lenth_sum=lenth_buffer;
	int kk=0;
	if(source_buffer == NULL)
	{
		error = 6;
		return ;
	}
	while( (lenth_sum>=SYMBOL_SIZE) && kk<K )
	{
		memcpy((byte*)(source_block[kk]),p_buffer,SYMBOL_SIZE*(sizeof(byte)));
		lenth_sum-=SYMBOL_SIZE;
		p_buffer+=SYMBOL_SIZE;
		++kk;
	}
	if( (lenth_sum>0) && (kk<K) )
	{
		memcpy((byte*)(source_block[kk]),p_buffer,lenth_sum*(sizeof(byte)));
		memset((byte*)( source_block[kk] + lenth_sum),0,(SYMBOL_SIZE-lenth_sum)*sizeof(byte));
		++kk;
	}
	for(int i=kk;i<K;++i)
	{
		memset((byte*)( source_block[kk]),0,SYMBOL_SIZE*sizeof(byte));
	}
	EncoderCalculate(K, L, Lp, (elim_stack *)mtr_op, mtr_op_size, source_block, H, S, SYMBOL_SIZE, Pr, U, des_block);
	if(count_of_packets!=NULL)
		*count_of_packets=get_Pr();
	return ;
}

int raptor_encoder::get_sub_block(byte* pdes_block, int k)
//the lenth of buffer pdes_block SHOULD be SYMBOL_SIZE+sizeof(int)
{
	if(k>Pr || k<0 || !pdes_block)	return -1;	
	int *tp=(int*)pdes_block;
	tp[0]=k;
	memcpy(((byte*)pdes_block)+sizeof(int),(byte*)(des_block[k]),SYMBOL_SIZE*sizeof(byte));
	return 0;
}

int raptor_encoder::get_err_bit()
{
	return error;
}

void raptor_encoder::clear_err_bit()
{
	error=0;
	return ;
}

void raptor_encoder::start_encode_clock()
{//start to count clocks of cpus, to test the effect of encode-process
	start = clock();
	return ;
}

clock_t raptor_encoder::get_encode_clock()
{//stop counting clocks of cpus, to test the effect of encode-process
	clock_t time_pass=clock();
	return (time_pass - start);
}

void raptor_encoder::clear_src_blocks()
{//clear dirty data in source_blocks
	for(int i=0;i<K;++i)
	{
		memset((byte*)(source_block[i]),0,SYMBOL_SIZE*sizeof(byte));
	}
	return ;
}

void raptor_encoder::clear_des_blocks()
{//clear dirty data in des_block
	for(int i=0;i<Pr;++i)
	{
		memset((byte*)(des_block[i]),0,SYMBOL_SIZE*sizeof(byte));
	}
	return ;
}

int raptor_encoder::get_Pr()
{//get value of Pr
	return Pr;
}

void raptor_encoder::get_A(long long **_A)
{
	//MATRIX_PRINT(A,L,INT_CEIL64(L));
	for(int i=0;i<L;++i)
	{
		for(int j=0;j<L;++j)
		{
			printf("%lld",GET_MATRIX_ij(_A,i,j));
		}
		printf("\n");
	}
	printf("\n");
	return ;
}

void raptor_encoder::ppscalculate(int _K, int _H, int _L, int _Lp, int _S, int _Pr, long long **AA, int **_U)
{//pps:"pre-coding process", calculate the Matrix A, the A is used to mark the symbols that join in the LTEnc calculation latterly, and Matrix A is a 0-1 matrix, just show the relationships
//The pre-coding relationships amongst the _L intermediate symbols are defined by expressing the last _L-_K intermediate symbols in terms of the first _K intermediate symbols.
//Generate a matrix which is marks all realtionships from _L to _L-_K		
	double Hp=0;
	int *d=(int*)(_U[0]);
	int *a=(int*)(_U[1]);
	int *b=(int*)(_U[2]);
//TRIPLE_PRINT(U,_K);
	Hp= ceil(((double)_H)/2.);
//printf("Hp = %f\n",Hp);
	MATRIX_ALLOC(GL,_H,_K+_S,int,0);
	MATRIX_ALLOC(GT,_K,_L,int,0);
	MATRIX_ALLOC(GH,_S,_K,int,0);

//**************************************************************************
/*
Matrix G_Half:			
	_K+_S
	kk
		-----------------------------
i		|	 |   					|
		|----|   					|	
_H		|	 |						|
		|	 |						|
		|	 |						|
		-----------------------------
		qq
When fill in this matrix, we use the generating gray code p, from row 0 to row _H, to arrange it as a collum, for example, in first col, we have gray code is 11100, so ,in the first col is 11100
*/			
	int j=1;
	int kk=0;

	int bit_count=0;
	int gray_code=0;
	//operation on GL also known as G_Half
	//_K+_S column, then fill the row
	while ((kk+1) <= _K+_S)
	{
		bit_count=0;
		sh(j, _H, &gray_code);
		bit_count = count1_of_int32(gray_code,_H);
//printf("gray_code = %d \n sum = %d",gray_code,bit_count);
		if (bit_count==Hp)//the m[j,k] is solited into m[j,_H'],so the m could be calc just with j
		{
			for(int i=0; i< _H; i++)
			{
				if( (gray_code>>i) & 0x1)
					GL[i][kk]=1;
			}//fill the matrix: GL, the kk column all, if not satisified, then there is 0 with init
			kk++;
		}
		j++;
	}
//MATRIX_PRINT(GL,_H,_K+_S);printf("\n");
//************************************************************************************
/*
Matrix G_LT:	
				_L
				
		-----------------------------
	jj	|---------------------------|
		|							|	
_K		|							|
		|	 						|
		|	   						|
		----------------------------- 
"G_LT(i,j) = 1 if and only if C[j] is included in the
symbols that are XORed to produce LTEnc[_K, (C[0], ..., C[_L-1]),
(d[i], a[i], b[i])]" --- mark all the symbols that are used to join the XOR operation
*/
	int tem;

	for (int jj=0; jj<_K; jj++)
	{
		tem=b[jj];//b
		while (tem>=_L)
			tem=((tem+a[jj]) % _Lp);
		
		GT[jj][tem]= 1;
		
		int SS=MIN(d[jj]-1, _L-1);
		
		for (int ss=0; ss<SS; ss++)
		{
			tem=((tem+a[jj]) % _Lp);
			while (tem>=_L)
				tem=((tem+a[jj]) % _Lp);
			GT[jj][tem]= 1;
		}
	}
//printf("GT = ");MATRIX_PRINT(GT,_K,_L);printf("\n");
//*******************************************************************************	
/*
Matrix G_LDPC:			
					_K
	i
		-----------------------------
		|	 |   					|
		|	 |   					|	
	_S	|	 |						|
		|	 |						|
		|	 |						|
		-----------------------------
*/			
	
	int aa=0;
	int bb=0;	
	for (int i=0; i<_K; i++)
	{
		double W=floor( ((double)(i))/((double)_S));
		aa= (int) (1+ ((int)W % (_S-1)));
		bb= i % _S;
		GH[bb][i]= 1;
		bb=(bb+aa) % _S;	
		GH[bb][i]= 1;
		bb=(bb+aa) % _S;
		GH[bb][i]= 1;
	}
//MATRIX_PRINT(GT,_K,_L);printf("\n");
//*******************************************************************************
	MATRIX_ALLOC(eye1,_S,_S,byte,0);
	MATRIX_ALLOC(eye2,_H,_H,byte,0);
	MATRIX_ALLOC(zero,_S,_S,byte,0);
	
	for(int row=0; row < _S; ++row)
	{
		eye1[row][row]=1;
	}	

	for (int row=0; row < _H; row++)
	{
		eye2[row][row]=1;
	}
	
//************************************************************************************
//Copy all the data from 6 Matrix into the AA
	int t=0;	
	int tt=0;
	int p=-1;
	int pp=-1;
	int ppp=-1;
	for (int i=0; i<_L; i++)
	{
		t=0;	
		tt=0;
		if (_S<=i && i<_S+_H)
			p++;
		if (i>= _S+_H)
			pp++;
		if (_S<=i && i<_S+_H)
			ppp++;
				
		for (int jj=0; jj<_L; jj++)
		{
			if (i<_S)
			{
				if (jj<_K)
//					AA[i][jj]=GH[i][jj];
					SET_MATRIX_ij(AA,i,jj,GH[i][jj]);
				if (_K<=jj && jj<_K+_S)
				{
//					AA[i][jj]= eye1[i][t];
					SET_MATRIX_ij(AA,i,jj,eye1[i][t]);
					t++;
				}
			}
			if (_S<=i && i<_S+_H) 
			{
				if (jj<_K+_S)
				{
//					AA[i][jj]= GL[p][jj];
					SET_MATRIX_ij(AA,i,jj,GL[p][jj]);
					
				}
				if (jj>=_K+_S)
				{
//					AA[i][jj]= eye2[ppp][tt];
					SET_MATRIX_ij(AA,i,jj,eye2[ppp][tt]);
					tt++;
				}				
			}		
			if (i>= _S+_H)
			{
//				AA[i][jj]= GT[pp][jj];
				SET_MATRIX_ij(AA,i,jj,GT[pp][jj]);
				
			}
		}
	}

	MATRIX_DEL(GL,_H);
	MATRIX_DEL(GH,_S);
	MATRIX_DEL(GT,_K);
	MATRIX_DEL(eye1,_S);
	MATRIX_DEL(eye2,_H);
	MATRIX_DEL(zero,_S);
	return ;
}

void raptor_encoder::Inversecalculate(long long **AA, int _L, int _H, int _K, int _S, elim_stack* _mtr_op,long long *_mtr_op_size) 
{//the inverse processing: solve (M_A^^-1)*M_D
//Inversecalculate( (long long **)A, L, H, K, S, SYMBOL_SIZE, (elim_stack*)mtr_op, &mtr_op_size);
	/*
		This process try to make the matrix AA into a I_L matrix, using Gauss-Jordan elimination, the result turn out to be a Jordan-regular matrix.
		there are 3 points should be attentioned:
		1) Matrix A is a 0-1 matrix, which is unusual to other matrix
		2) All the operation in this process is XOR, including abstruct in matrix A as well as in block operation showing in 2-dim array D, when you try to abstract a number, equaling to XOR it with the other number
		3) The data stored in D is regarded as a whole part, undividable
		
		The Algorithm in the process show as follows:
					jj
		---------------------------------
		|		  |		|	  |			|
		|       i |	  i'|  i''|			|
		|  		 \|/	|     |			|
		|			   \|/	  |			|
		|					  |			|
		|					 \|/		|
		|								|
		|								|
		|								|
		|								|
		|								|
		---------------------------------
		
		set jj = 0
		1) record all the number of rows upside line jj, stored in HI, k
		2) record all the number of rows downside, inclusive, line jj, stored in LOW, kk
		3) from 1 to kk, for all line in col jj is 1, XOR them with line Low[0] and eliminate it, so as the blocks
		4) from 0 to k,  for all line in col jj is 1, XOR them with line Low[0] and eliminate it, so as the blocks
		5) exchange the line Low[0] with jj
		6£©Do the process until jj is equal to _L
		COMMENTS: after the process 4), there is only 1 line has 1 in position (Low[0],jj), which is the principle of Gauss-Jordan elimination works 
		
		output of the process: the inversed matrix mult data vector, which is 
		(M_A^^-1)*M_D
	*/
	const int lenth_row_L=INT_CEIL64(_L);
	*_mtr_op_size = 0;

	long long *temp = new long long[lenth_row_L];
	byte *tempo = NULL;
	int base_zero_line=0;
	int i=0;
	for (int jj=0; jj<_L; jj++)
	{
		tempo=NULL;
		memset((long long*)temp,0,lenth_row_L*sizeof(long long));
		for(i=jj; (i<_L) && (!GET_MATRIX_ij(AA,i,jj)); ++i);
		if (i==_L){
			error = 3;
			continue;
		}//all is 0, non-mark matrix
		base_zero_line=i;
		for(i=0;i<base_zero_line;++i)
		{
			if( GET_MATRIX_ij(AA,i,jj) )
			{
				memxor((char *)(AA[i]),(char*)(AA[base_zero_line]),lenth_row_L*sizeof(long long));
				ELIM_STACK_OPRS(_mtr_op, eliml, base_zero_line, i, (*_mtr_op_size));
			}
		}
		for(++i;i<_L;++i)
		{
			if( GET_MATRIX_ij(AA,i,jj) )
			{

				memxor((long long *)(AA[i]),(long long*)(AA[base_zero_line]),lenth_row_L*sizeof(long long));
				ELIM_STACK_OPRS(_mtr_op, eliml, base_zero_line, i, (*_mtr_op_size));
			}
		}
		if(base_zero_line!=jj)
		{
			memcpy((long long*)temp,(long long *)(AA[jj]),lenth_row_L*sizeof(long long));
			memcpy((long long*)(AA[jj]),(long long*)(AA[base_zero_line]),lenth_row_L*sizeof(long long));
			memcpy((long long*)(AA[base_zero_line]),(long long*)temp,lenth_row_L*sizeof(long long));
			ELIM_STACK_OPRS(_mtr_op, chpos, base_zero_line, jj, (*_mtr_op_size));
		}
	}
	SAFE_DELETE_ARR(temp);
	return ;
}

void raptor_encoder::EncoderCalculate(int _K, int _L, int _Lp, elim_stack *_mtr_op, long long _mtr_op_size, byte **VV, int _H, int _S, int _SYMBOL_SIZE, int _Pr, int** _U, byte** e)
{//encoding process main module
	const int lenth_row_L=INT_CEIL64(_L);
	int *d = (int*)(_U[0]);
	int *a = (int*)(_U[1]);
	int *b = (int*)(_U[2]);
	byte **intermid_index = new byte*[_L];//Lx1(pointer array)
	MATRIX_ALLOC( c, _L, _SYMBOL_SIZE, byte, 0);
	for(int i=0;i<_K;++i)
		memcpy(e[i],VV[i],_SYMBOL_SIZE*sizeof(byte));
	for(int i=0;i<_L;++i)
		intermid_index[i]=(byte*)(c[i]);
	Ensymbol_process( (elim_stack*)_mtr_op, _mtr_op_size, (byte **)VV,_SYMBOL_SIZE,_K, _L, (byte **)intermid_index);
	for (int i=_K; i<_Pr; i++)
	{//e is used for return value, store in here
		Lncalculate(_Lp, (byte **)intermid_index, d[i], a[i], b[i], _L, _SYMBOL_SIZE, (byte*)(e[i]));
	}
	MATRIX_DEL(c,_L);
	SAFE_DELETE_ARR(intermid_index);
	return ;
}

void raptor_encoder::Ensymbol_process(elim_stack *_mtr_op, long long _mtr_op_size, byte **b,int _SYMBOL_SIZE,int _K, int _L, byte **_intermid_index)
{
	int P=0;
	for (int i=_L-_K; i<_L; i++)
	{//move _K symbols into last D part
		memcpy((byte*)(_intermid_index[i]),(byte*)(b[P++]),_SYMBOL_SIZE*sizeof(byte));
	}
	
	for(int i=0;i<_mtr_op_size;++i)
	{	
		byte *tempo = NULL;
		short int opr0=_mtr_op[i].opr;
		short int opr1=_mtr_op[i].addr1;
		short int opr2=_mtr_op[i].addr2;
		
		if(opr0==eliml)
		{
			memxor((byte*)(_intermid_index[opr2]),(byte*)(_intermid_index[opr1]),_SYMBOL_SIZE);
		}
		if(opr0==chpos)
		{
			tempo = (byte*)(_intermid_index[opr2]);
			_intermid_index[opr2] = (byte*)(_intermid_index[opr1]);
			_intermid_index[opr1] = (byte*)(tempo);
		}
	}
	return ;
}

void raptor_encoder::Lncalculate(int _Lp, byte **_intermid_index, int d, int a, int b, int _L, int _SYMBOL_SIZE, byte* e) 
{//Using the algorithm in page 22, to generate advinced-'degree' b, then combine b symbols in c, the result is a packet to be general

	while (b>=_L)
		b=((b+a) % _Lp);
	memcpy((byte*)e,(byte*)(_intermid_index[b]),sizeof(byte)*_SYMBOL_SIZE);
	int SS=0;
	SS=MIN(d-1, _L-1);

	for (int ss=0; ss<SS; ss++)
	{
		b=((b+a) % _Lp);
		while (b>=_L)
			b=((b+a) % _Lp);
		memxor((byte*)e, (byte*)(_intermid_index[b]),_SYMBOL_SIZE);
	}
	return ;
}

