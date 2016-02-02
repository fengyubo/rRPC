#include "raptor_code.h"
#include <math.h>
#include <memory.h>
#include <stdio.h>

raptor_decoder::raptor_decoder(int _K, int _SYMBOL_SIZE, double _Overhead, double PRECENT_PR, int _type)
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
	type=_type;
	Overhead = _Overhead;
	N=(int) ((1+ Overhead)* K);  //Receive Overhead more encoding packet
	int Pr=(int)((1+PRECENT_PR)*K);
	int X=1;
	int i=0;
	while((X<combination_cn2_size)&(combination_cn2_set[X]<K))	++X;
	for(S=1;S<ceil(0.01*K)+X;++S)	;
	for(i=0;(i<prime_size)&(prime_numset[i]<S);++i)	;	S=prime_numset[i];	
	for(H=1;(H<combination_size)&(combination_numset[H]<K+S);++H)	;	
	L=K+S+H;//The most important statement: define the lenth of L, which are used to construct the code vector
	for(Lp=L,i=0;(i<prime_size)&(prime_numset[i]<Lp);++i); Lp=prime_numset[i];
	M=N+S+H;
	MATRIX_ALLOC_NODEF(U,_TRIPLE_ROW_,Pr,int,0);
	MATRIX_ALLOC_NODEF(B,S+H,INT_CEIL64(L),long long,0);
	MATRIX_ALLOC_NODEF(source_block,K,SYMBOL_SIZE,byte,0);
	MATRIX_ALLOC_NODEF(backup_block,N,SYMBOL_SIZE,byte,0);
	unfailsouceblock=0;
	rev_block = NULL;
	ESIs = NULL;
	switch(type)
	{
		case(1):
			pDecoderCalculate =&raptor_decoder::at_DecoderCalculate;
			break;
		case(2):
			break;
		default:
			pDecoderCalculate =&raptor_decoder::DecoderCalculate;
			break;
	}
	TripleCalculate(K, Lp, Pr,(int**)U);
	ppscalculate(K, H, L, Lp, S, (long long**)B);
}

raptor_decoder::~raptor_decoder()
{
	MATRIX_DEL(U,_TRIPLE_ROW_);
	MATRIX_DEL(B,S+H);
	MATRIX_DEL(backup_block,N);
	MATRIX_DEL(source_block,K);
}

void raptor_decoder::reset_raptor(int _K, int _SYMBOL_SIZE, double _Overhead, double PRECENT_PR, int _type)
{
	MATRIX_DEL(U,_TRIPLE_ROW_);
	MATRIX_DEL(B,S+H);
	MATRIX_DEL(backup_block,N);
	MATRIX_DEL(source_block,K);
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
	Overhead = _Overhead;
	type=_type;
	N=(int) ((1+ Overhead)* K);  //Receive Overhead more encoding packet
	int Pr=(int)((1+PRECENT_PR)*K);
	int X=1;
	int i=0;
	while((X<combination_cn2_size)&(combination_cn2_set[X]<K))	++X;
	for(S=1;S<ceil(0.01*K)+X;++S)	;
	for(i=0;(i<prime_size)&(prime_numset[i]<S);++i)	;	S=prime_numset[i];	
	for(H=1;(H<combination_size)&(combination_numset[H]<K+S);++H)	;	
	L=K+S+H;//The most important statement: define the lenth of L, which are used to construct the code vector
	for(Lp=L,i=0;(i<prime_size)&(prime_numset[i]<Lp);++i); Lp=prime_numset[i];
	M=N+S+H;
	MATRIX_ALLOC_NODEF(U,_TRIPLE_ROW_,Pr,int,0);
	MATRIX_ALLOC_NODEF(B,S+H,INT_CEIL64(L),long long,0);
	MATRIX_ALLOC_NODEF(source_block,K,SYMBOL_SIZE,byte,0);
	MATRIX_ALLOC_NODEF(backup_block,N,SYMBOL_SIZE,byte,0);
	unfailsouceblock=0;
	rev_block = NULL;
	ESIs = NULL;
	switch(type)
	{
		case(1):
			pDecoderCalculate =&raptor_decoder::at_DecoderCalculate;
			break;
		case(2):
			break;
		default:
			pDecoderCalculate =&raptor_decoder::DecoderCalculate;
			break;
	}
	
	TripleCalculate(K, Lp, Pr,(int**)U);
	ppscalculate(K, H, L, Lp, S, (long long**)B);
}

void raptor_decoder::base_process_decode(byte **erc,int *_ESIs, int rev_n, int *lost_ESIs, int *lostcount)
{//input should be blocks array erc and the index of erc ESIs, which marks all the position of sub-block of whole block, 
	//if decoding process fail, then we just give the blocks that still could be used, otherwise, we could recover all the packets
/*
input: 
	byte array: erc, encoding packets
	int array: _ESIs, ESIs marks of encoding packets
	int : rev_n, the size of _ESIs and erc 
*/	
	*lostcount=0;
	if(rev_n < N)
	{
		error = 5;
		rev_block = (byte**)erc;
		clear_fail_decode();
		ESIs = (int*)_ESIs;
		byte *flag_source_empty = new byte[K];
		memset((byte*)(flag_source_empty),0,K*sizeof(byte));
		for(int i=0;i<rev_n;++i)
		{
			if(ESIs[i]<K)
			{
				memcpy((byte*)(source_block[ESIs[i]]),(byte*)(rev_block[i]),SYMBOL_SIZE*sizeof(byte));
				add_fail_decode();
				flag_source_empty[ESIs[i]]=1;
			}
		}
		for(int i=0;i<K;++i)
		{
			if(!(flag_source_empty[i]))
			{
				lost_ESIs[*lostcount]=i;++(*lostcount);
				memset((byte*)(source_block[i]),0,SYMBOL_SIZE*sizeof(byte));
			}
		}
		SAFE_DELETE_ARR(flag_source_empty);
	}
	else
	{//attention: size of ESIs should no more than N
		rev_block = (byte**)erc;
		MATRIX_COPY(backup_block,rev_block,N,SYMBOL_SIZE,byte);
		clear_fail_decode();
		ESIs = (int*)_ESIs;
		(this->*pDecoderCalculate)(K, Lp, (long long **)B, (byte **)rev_block, N, M, L, SYMBOL_SIZE, (int**)U, (int*)ESIs, 0, (byte**)source_block);
		//dealing with the possibly that the decode is fail
		if(get_err_bit())
		{
			byte *flag_source_empty = new byte[K];
			memset((byte*)(flag_source_empty),0,K*sizeof(byte));
			for(int i=0;i<N;++i)
			{
				if(ESIs[i]<K)
				{
					memcpy((byte*)(source_block[ESIs[i]]),(byte*)(backup_block[i]),SYMBOL_SIZE*sizeof(byte));
					add_fail_decode();
					flag_source_empty[ESIs[i]]=1;
				}
			}
			for(int i=0;i<K;++i)
			{
				if(!(flag_source_empty[i]))
				{
					lost_ESIs[*lostcount]=i;++(*lostcount);
//					memset((byte*)(source_block[i]),0,SYMBOL_SIZE*sizeof(byte));
				}
			}
			SAFE_DELETE_ARR(flag_source_empty);
		}
	}
	return ;
}

void raptor_decoder::get_A(long long **_A, int row, int col)
{
	//MATRIX_PRINT(A,L,INT_CEIL64(L));
	for(int i=0;i<row;++i)
	{
		for(int j=0;j<col;++j)
		{
			printf("%lld",GET_MATRIX_ij(_A,i,j));
		}
		printf("\n");
	}
	printf("\n");
	return ;
}

void raptor_decoder::get_decode_result(byte* result_buffer, int lenth_buffer)
{//lenth_buffer: number of Byte
	byte *tp_write=(byte*)result_buffer;
	int max_buffer = lenth_buffer;
	if(result_buffer==NULL)
	{
		error = 6;
		return ;
	}
	int k=0;
	while( (max_buffer >= SYMBOL_SIZE) && (k<N))
	{
		memcpy((byte*)(tp_write),(byte*)(source_block[k]),SYMBOL_SIZE*sizeof(byte));
		tp_write+=SYMBOL_SIZE;
		max_buffer-=SYMBOL_SIZE;
		++k;
	}
	if( ( max_buffer > 0 ) && (k<N) )
	{
		memcpy((byte*)(tp_write),(byte*)(source_block[k]),max_buffer*sizeof(byte));
	}
	return ;
}

void raptor_decoder::process_decode(byte **rec_packets, int rev_n, byte* result_buffer, int lenth_buffer, int *lost_ESIs, int *lostcount)
{
	MATRIX_ALLOC(erc,rev_n,SYMBOL_SIZE,byte,0);
	int *ESIs=new int[rev_n];
	for(int i=0;i<rev_n;++i)
	{
		ESIs[i]=((int*)(rec_packets[i]))[0];
		memcpy((byte*)(erc[i]),(byte*)(rec_packets[i])+sizeof(int),SYMBOL_SIZE*sizeof(byte));
	}
//printf("ESI decode = \n"); for(int i=0;i<rev_n;++i)	printf("%d ",ESIs[i]);printf("\n");
	base_process_decode((byte**)erc,(int*)ESIs,rev_n,lost_ESIs,lostcount);
	get_decode_result((byte*)result_buffer,lenth_buffer);
	MATRIX_DEL(erc,rev_n);
	SAFE_DELETE_ARR(ESIs);
	return ;
}

int raptor_decoder::get_fail_decode()
{
	return unfailsouceblock;
}

void raptor_decoder::clear_fail_decode()
{
	unfailsouceblock=0;
	return ;
}

void raptor_decoder::add_fail_decode()
{
	++unfailsouceblock;
	return ;
}

int raptor_decoder::get_err_bit()
{
	return error;
}

void raptor_decoder::clear_err_bit()
{
	error=0;
	return ;
}

void raptor_decoder::start_decode_clock()
{
	start = clock();
	return ;
}

clock_t raptor_decoder::get_decode_clock()
{
	clock_t pass_time=clock();
	return (pass_time - start);
}

void raptor_decoder::clear_src_blocks()
{
	for(int i=0;i<K;++i)
	{
		memset((byte*)(source_block[i]),0,SYMBOL_SIZE*sizeof(byte));
	}
	return ;
}

void raptor_decoder::clear_rev_blocks()
{
	for(int i=0;i<N;++i)
	{
		memset((byte*)(rev_block[i]),0,SYMBOL_SIZE*sizeof(byte));
	}
	memset((int*)(ESIs),0,N*sizeof(int));
	return ;
}

void raptor_decoder::ppscalculate(int _K, int _H, int _L, int _Lp, int _S, long long **AA)
{//pps:"pre-coding process", calculate the Matrix A, the A is used to mark the symbols that join in the LTEnc calculation latterly, and Matrix A is a 0-1 matrix, just show the relationships
//The pre-coding relationships amongst the _L intermediate symbols are defined by expressing the last _L-_K intermediate symbols in terms of the first _K intermediate symbols.
//Generate a matrix which is marks all realtionships from _L to _L-_K		
	double Hp=0;
//TRIPLE_PRINT(U,_K);
	Hp= ceil(((double)_H)/2.);
//printf("Hp = %f\n",Hp);
	MATRIX_ALLOC(GL,_H,_K+_S,int,0);
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
	for (int i=0; i<_S+_H; i++)
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
		}
	}

	MATRIX_DEL(GL,_H);
	MATRIX_DEL(GH,_S);
	MATRIX_DEL(eye1,_S);
	MATRIX_DEL(eye2,_H);
	MATRIX_DEL(zero,_S);
	return ;
}

//decode solution 1.
void raptor_decoder::AAcalculate( long long** AA, int _K, int* _ESIs, int _N, int _L, int* d, int* a, int* b, int _Lp, long long** Anew)
{//Construct the Matrix A which described in page 24 whose is (_N+_S+_H) * _L, this matrix is differenet from Matrix A in encoder part whose is _L*_L; Through sloving this matrix, the inter-maxtrix could be done
		const int lenth_row_L=INT_CEIL64(_L);
		int value=0x01;
		MATRIX_ALLOC(GLTnew,_N,lenth_row_L,long long,0);
		int tem=0;
		int esis_jj = 0;
		int esis_a = 0;
		int esis_b = 0;
		for (int jj=0; jj<_N; jj++)
		{
			esis_jj = _ESIs[jj];
			esis_a = a[_ESIs[jj]];
			esis_b = b[_ESIs[jj]];
			tem=esis_b;
			while (tem>=_L)
				tem=((tem+esis_a) % _Lp);

			SET_MATRIX_ij(GLTnew,jj,tem,value);
			int SS=MIN(d[_ESIs[jj]]-1, _L-1);
			for (int i=0; i<SS; i++)
			{
				tem=((tem+esis_a) % _Lp);
				while (tem>=_L)
					tem=((tem+esis_a) % _Lp);
				SET_MATRIX_ij(GLTnew,jj,tem,value);
			}
		}
		int i=0;
		int _S_H=_L-_K;
		for(;i<_S_H;++i)
		{
			memcpy((long long*)(Anew[i]),(long long*)(AA[i]),lenth_row_L*sizeof(long long));
		}
		int _M = _S_H + _N;
		for(int p=0;i<_M;++i)
		{
			memcpy((long long*)(Anew[i]),(long long*)(GLTnew[p++]),lenth_row_L*sizeof(long long));
		}
		MATRIX_DEL(GLTnew,_N);
		return ;
}

void raptor_decoder::GEcalculate(long long **AA, int _K, int _L, int _N, int _M, int _SYMBOL_SIZE, int r, byte** _intermid_index)
{//GEcalculate(Aw, erc1, _SYMBOL_SIZE, r);Aw:matrix build before: (_N+S+H) * _L, erc1: _N * _SYMBOL_SIZE
	const int lenth_row_L=INT_CEIL64(_L);
	int P=0;
	long long *temp = new long long[lenth_row_L];
	byte *tempo = NULL;
	int base_zero_line=0;
	int i=0;
	for (int jj=0; jj<_L; jj++)
	{
		tempo=NULL;
		memset((long long*)temp,0,lenth_row_L*sizeof(long long));
		for(i=jj; (i<_M) && (!GET_MATRIX_ij(AA,i,jj)); ++i);
		if (i==_M){
			error = 4;
			continue;
		}//all is 0, non-mark matrix
		base_zero_line=i;
		for(i=0;i<base_zero_line;++i)
		{
			if( GET_MATRIX_ij(AA,i,jj) )
			{
				memxor((byte*)(_intermid_index[i]),(byte*)(_intermid_index[base_zero_line]),_SYMBOL_SIZE);
				memxor((long long *)(AA[i]),(long long*)(AA[base_zero_line]),lenth_row_L*sizeof(long long));
			}
		}
		for(++i;i<_M;++i)
		{
			if( GET_MATRIX_ij(AA,i,jj) )
			{
				memxor((byte*)(_intermid_index[i]),(byte*)(_intermid_index[base_zero_line]),_SYMBOL_SIZE);
				memxor((char *)(AA[i]),(char*)(AA[base_zero_line]),lenth_row_L*sizeof(long long));
			}
		}
		memcpy((long long*)temp,(long long *)(AA[jj]),lenth_row_L*sizeof(long long));
		memcpy((long long*)(AA[jj]),(long long*)(AA[base_zero_line]),lenth_row_L*sizeof(long long));
		memcpy((long long*)(AA[base_zero_line]),(long long*)temp,lenth_row_L*sizeof(long long));

		tempo = (byte*)(_intermid_index[jj]);
		_intermid_index[jj] = (byte*)(_intermid_index[base_zero_line]);
		_intermid_index[base_zero_line] = (byte*)(tempo);
	}
	SAFE_DELETE_ARR(temp);
	//discarded _M-_L rows to get the result
	return ;
}

void raptor_decoder::DecoderCalculate(int _K, int _Lp, long long **_B, byte **erc1, int _N, int _M, int _L, int _SYMBOL_SIZE, int** _U, int* _ESIs, int r, byte**y )
{	//input: B,the matrix of the 'A', erc1: the ereasuing blocks, _U: tiples that made before, also should be knowing, _ESIs, index pointer of erc1, _N is lenth of _ESIs
/*The order in which Gaussian elimination is performed to form the decoding schedule has no bearing on whether or not the decoding is successful. However, the speed of the decoding depends heavily on the order in which Gaussian elimination is performed. (Furthermore, maintaining a sparse representation of A is crucial, although this is not described here). The remainder of this section describes an order in which Gaussian elimination could be performed that is relatively efficient.*/

	int* d=(int*)(_U[0]);
	int* a=(int*)(_U[1]);
	int* b=(int*)(_U[2]);
	const int lenth_row_L=INT_CEIL64(_L);
	byte **intermid_index = new byte*[_M];//Lx1(pointer array)
	MATRIX_ALLOC( Aw, _M, lenth_row_L, long long, 0 );
	MATRIX_ALLOC(cdc, _M, _SYMBOL_SIZE, byte, 0 );	
	for (int i=0,p=0; i<_M; ++i)
	{
		(i<_M-_N) ?	memset((byte*)(cdc[i]), 0, _SYMBOL_SIZE*sizeof(byte)) : memcpy( (byte*)(cdc[i]), (byte*)(erc1[p++]), _SYMBOL_SIZE*sizeof(byte) );
		intermid_index[i]=(byte*)(cdc[i]);
	}
	AAcalculate( _B, _K, _ESIs, _N, _L, d, a, b, _Lp, (long long**)Aw);
	GEcalculate(Aw, _K, _L, _N, _M, _SYMBOL_SIZE, r, (byte**) intermid_index);
	for (int j=0; j<_K; j++)
		Lncalculate(_Lp, intermid_index, d[j], a[j], b[j], _L, _SYMBOL_SIZE, (byte*)(y[j]));
	MATRIX_DEL(Aw,_M);
	MATRIX_DEL(cdc,_M);
	return ;
}

//decode solution 2.
void raptor_decoder::at_AAcalculate( long long** AA, int _K, int* _ESIs, int _N, int _L, int _M, int* d, int* a, int* b, int _Lp, elim_stack *_mtr_op, long long *_mtr_op_size)
{//Construct the Matrix A which described in page 24 whose is (_N+_S+_H) * _L, this matrix is differenet from Matrix A in encoder part whose is _L*_L; Through sloving this matrix, the inter-maxtrix could be done
		const int lenth_row_L=INT_CEIL64(_L);
		int value=0x01;
		MATRIX_ALLOC(GLTnew,_N,lenth_row_L,long long,0);
		MATRIX_ALLOC(Anew, _M, lenth_row_L, long long, 0 );
		int tem=0;
		for (int jj=0; jj<_N; jj++)
		{
			tem=b[_ESIs[jj]];
			while (tem>=_L)
				tem=((tem+a[_ESIs[jj]]) % _Lp);

			SET_MATRIX_ij(GLTnew,jj,tem,value);
			int SS=MIN(d[_ESIs[jj]]-1, _L-1);
			for (int i=0; i<SS; i++)
			{
				tem=((tem+a[_ESIs[jj]]) % _Lp);
				while (tem>=_L)
					tem=((tem+a[_ESIs[jj]]) % _Lp);
				SET_MATRIX_ij(GLTnew,jj,tem,value);
			}
		}
		int i=0;
		int _S_H=_L-_K;
		for(;i<_S_H;++i)
		{
			memcpy((long long*)(Anew[i]),(long long*)(AA[i]),lenth_row_L*sizeof(long long));
		}
		
		for(int p=0;i<_M;++i)
		{
			memcpy((long long*)(Anew[i]),(long long*)(GLTnew[p++]),lenth_row_L*sizeof(long long));
		}
		MATRIX_DEL(GLTnew,_N);
//printf("BREAK\n");
		//pre-matrix eliminate
		long long *temp = new long long[lenth_row_L];
		int base_zero_line=0;
		i=0;
		for (int jj=0; jj<_L; jj++)
		{
			for(i=jj; (i<_M) && (!GET_MATRIX_ij(Anew,i,jj)); ++i);
			if (i==_M){
				error = 4;
				return;
			}//all is 0, non-mark matrix
			base_zero_line=i;
			for(i=0;i<_M;++i)
			{
				if( (i!=base_zero_line) && GET_MATRIX_ij(Anew,i,jj) )
				{
					for (int q=0; q<lenth_row_L; q++)
					{
						Anew[i][q]=(long long) (Anew[i][q] ^ Anew[base_zero_line][q]);
					}
					ELIM_STACK_OPRS(_mtr_op, eliml, base_zero_line, i, (*_mtr_op_size));
				}
			}
			memcpy((long long*)temp,(long long *)(Anew[jj]),lenth_row_L*sizeof(long long));
			memcpy((long long*)(Anew[jj]),(long long*)(Anew[base_zero_line]),lenth_row_L*sizeof(long long));
			memcpy((long long*)(Anew[base_zero_line]),(long long*)temp,lenth_row_L*sizeof(long long));
			ELIM_STACK_OPRS(_mtr_op, chpos, base_zero_line, jj, (*_mtr_op_size));
		}
		MATRIX_DEL(Anew,_M);
		return ;
}

void raptor_decoder::at_Desymbol_process(elim_stack *_mtr_op, long long _mtr_op_size, byte **cdc, byte **erc1, int _SYMBOL_SIZE, int _M, int _N, byte **_intermid_index)
{
	for (int i=0,p=0; i<_M; ++i)
	{
		(i<_M-_N) ?	memset((byte*)(cdc[i]), 0, _SYMBOL_SIZE*sizeof(byte)) : memcpy( (byte*)(cdc[i]), (byte*)(erc1[p++]), _SYMBOL_SIZE*sizeof(byte) );
		_intermid_index[i]=(byte*)(cdc[i]);
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
		if(_mtr_op[i].opr==chpos)
		{
			tempo = (byte*)(_intermid_index[opr2]);
			_intermid_index[opr2] = (byte*)(_intermid_index[opr1]);
			_intermid_index[opr1] = (byte*)(tempo);
		}
	}
	return ;
}

void raptor_decoder::at_DecoderCalculate(int _K, int _Lp, long long **_B, byte **erc1, int _N, int _M, int _L, int _SYMBOL_SIZE, int** _U, int* _ESIs, int r, byte**y )
{	//input: B,the matrix of the 'A', erc1: the ereasuing blocks, _U: tiples that made before, also should be knowing, _ESIs, index pointer of erc1, _N is lenth of _ESIs
/*The order in which Gaussian elimination is performed to form the decoding schedule has no bearing on whether or not the decoding is successful. However, the speed of the decoding depends heavily on the order in which Gaussian elimination is performed. (Furthermore, maintaining a sparse representation of A is crucial, although this is not described here). The remainder of this section describes an order in which Gaussian elimination could be performed that is relatively efficient.*/

	int* d=(int*)(_U[0]);
	int* a=(int*)(_U[1]);
	int* b=(int*)(_U[2]);
	const int lenth_row_L=INT_CEIL64(_L);
	elim_stack *mtr_op=new elim_stack[_M*_L];;//elimination process
	long long mtr_op_size=0;
	at_AAcalculate( _B, _K, _ESIs, _N, _L, _M, d, a, b, _Lp, (elim_stack*)mtr_op, &mtr_op_size);
	if(get_err_bit()==4)
	{
		return ;
	}
	MATRIX_ALLOC(cdc, _M, _SYMBOL_SIZE, byte, 0 );
	byte **intermid_index = new byte*[_M];//Lx1(pointer array)
	at_Desymbol_process((elim_stack*)mtr_op, mtr_op_size, (byte**)cdc, (byte**)erc1, _SYMBOL_SIZE, _M, _N, (byte**)intermid_index);
	for (int j=0; j<_K; j++)
		Lncalculate(_Lp, intermid_index, d[j], a[j], b[j], _L, _SYMBOL_SIZE, (byte*)(y[j]));
	MATRIX_DEL(cdc,_M);
	SAFE_DELETE_ARR(intermid_index);
	return ;
}

void raptor_decoder::Lncalculate(int _Lp, byte **c, int d, int a, int b, int _L, int _SYMBOL_SIZE, byte* e) 
{//Using the algorithm in page 22, to generate advinced-'degree' b, then combine b symbols in c, the result is a packet to be general

	while (b>=_L)
		b=((b+a) % _Lp);

	memcpy((byte*)e,(byte*)(c[b]),sizeof(byte)*_SYMBOL_SIZE);
	int SS=0;
	SS=MIN(d-1, _L-1);
	for (int ss=0; ss<SS; ss++)
	{
		b=((b+a) % _Lp);
		while (b>=_L)
			b=((b+a) % _Lp);
		memxor((byte*)e, (byte*)(c[b]),_SYMBOL_SIZE);
	}
	return ;
}



