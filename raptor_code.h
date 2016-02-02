#include <time.h>

//#define ___CENTOS64 0

#define _TRIPLE_ROW_ 3
#define byte char
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define K_MAX 8192
#define K_MIN 4
#define K_RECOMMEND 1024
#define RES_DIV_64 6
#define RES_MOD_64 (0x3f)
#define INT_CEIL64(INT_NAME) ((INT_NAME>>RES_DIV_64)+(!!(INT_NAME&RES_MOD_64)))
//Matrix set i,j
#define SET_MATRIX_ij(MATRIX_NAME,ROW,COL,VALUE)	\
							do{	MATRIX_NAME[ROW][COL>>RES_DIV_64] = (MATRIX_NAME[ROW][COL>>RES_DIV_64] | (((long long)(!!VALUE))<<(COL&RES_MOD_64)));}while(0)
//Matrix get i,j
#define GET_MATRIX_ij(MATRIX_NAME,i,j)	(((MATRIX_NAME[i][j>>6]) >> (j&(0x3f)))&0x01)

//#define GET_MATRIX_ij(MATRIX_NAME,i,j)	((MATRIX_NAME[i][j>>6]) & (((long long)0x01)<<(j&(0x3f))))

#define ELIM_STACK_OPRS(STACK_NAME, OPR_NAME, BASELINE, OPR_LINE, OPR_POS)	\
							do{	STACK_NAME[OPR_POS].opr=OPR_NAME;	\
								STACK_NAME[OPR_POS].addr1=BASELINE;	\
								STACK_NAME[OPR_POS].addr2=OPR_LINE;	\
								++(OPR_POS);	\
							}while(0)
							
#define MATRIX_ALLOC(MATRIX_NAME, MATRIX_LINE, MATRIX_COL, MATRIX_TYPE, INI_VALUE) \
							MATRIX_TYPE **MATRIX_NAME= new MATRIX_TYPE*[MATRIX_LINE];\
							for(int mm=0;mm<MATRIX_LINE;++mm)\
							{\
								MATRIX_NAME[mm] = new MATRIX_TYPE [MATRIX_COL];\
								memset(( MATRIX_TYPE *)(MATRIX_NAME[mm]),INI_VALUE,(MATRIX_COL)*sizeof(MATRIX_TYPE));\
							}
							
#define MATRIX_ALLOC_NOINIT(MATRIX_NAME, MATRIX_LINE, MATRIX_COL, MATRIX_TYPE) \
							MATRIX_TYPE **MATRIX_NAME= new MATRIX_TYPE*[MATRIX_LINE];\
							for(int mm=0;mm<MATRIX_LINE;++mm)\
							{\
								MATRIX_NAME[mm] = new MATRIX_TYPE [MATRIX_COL];\
							}
							
#define MATRIX_ALLOC_NODEF(MATRIX_NAME, MATRIX_LINE, MATRIX_COL, MATRIX_TYPE, INI_VALUE) \
							MATRIX_NAME= new MATRIX_TYPE*[MATRIX_LINE];\
							for(int mm=0;mm<MATRIX_LINE;++mm)\
							{\
								MATRIX_NAME[mm] = new MATRIX_TYPE [MATRIX_COL];\
								memset(( MATRIX_TYPE *)(MATRIX_NAME[mm]),INI_VALUE,(MATRIX_COL)*sizeof(MATRIX_TYPE));\
							}
							
#define MATRIX_COPY(MATRIX_NAME_DES, MATRIX_NAME_SRC, MATRIX_LINE, MATRIX_COL, MATRIX_TYPE) \
							for(int mm=0;mm<MATRIX_LINE;++mm)\
							{	\
								memcpy( (MATRIX_TYPE *)(MATRIX_NAME_DES[mm]),(MATRIX_TYPE *)(MATRIX_NAME_SRC[mm]),(MATRIX_COL)*sizeof(MATRIX_TYPE));\
							}
							
#define MATRIX_DEL(MATRIX_NAME,MATRIX_LINE)	\
							do{				\
								for(int mm=0; mm<MATRIX_LINE; ++mm)	\
								{									\
									if(MATRIX_NAME[mm]!=NULL)		\
									{								\
										delete [] MATRIX_NAME[mm];	\
									}								\
								}									\
								if(MATRIX_NAME!=NULL)				\
								{									\
									delete [] MATRIX_NAME;			\
								}									\
							}while(0)

#define ARR_PRINT(ARRAY_NAME, ARRAY_LENTH)				\
							do{										\
								printf("ARRAY_NAME = ");			\
								for(int ii=0;ii<ARRAY_LENTH;++ii)	\
									printf("%d ",ARRAY_NAME[ii]);	\
								printf("\n");						\
							}while(0)
							
#define SAFE_DELETE_ARR(ARRAY_NAME)				\
							do{					\
								if(ARRAY_NAME!=NULL)	\
									delete	[] ARRAY_NAME;	\
							}while(0)
							
#define MATRIX_PRINT(MATRIX_NAME, MATRIX_LINE, MATRIX_COL) \
							for(int mm=0;mm<MATRIX_LINE;++mm)\
							{									\
								for(int mn=0; mn<MATRIX_COL;++mn)\
								{									\
									printf("%X",MATRIX_NAME[mm][mn]);\
								}						\
								printf("\n");	\
							}
							
#define TRIPLE_PRINT(TRIPLE_NAME, TRIPLE_COL) \
							for(int mm=0;mm<3;++mm)\
							{									\
								for(int mn=0; mn<TRIPLE_COL;++mn)\
								{									\
									printf("%d ",TRIPLE_NAME[mm][mn]);\
								}						\
								printf("\n");	\
							}				
//C(H,ceil(H/2))
const int combination_numset[]={0,1,2,3,6,10,20,35,70,126,252,462,924,1716,3432,6435,12870};
const int combination_size=17;

//C(n,2) list
const int combination_cn2_set[]={0,0,1,3,6,10,15,21,28,36,45,55,66,78,91,105,120,136,153,171,190,210,231,253,276,300,325,351,378,406,435,465,496,528,561,595,630,666,703,741,780,820,861,903,946,990,1035,1081,1128,1176,1225,1275,1326,1378,1431,1485,1540,1596,1653,1711,1770,1830,1891,1953,2016,2080,2145,2211,2278,2346,2415,2485,2556,2628,2701,2775,2850,2926,3003,3081,3160,3240,3321,3403,3486,3570,3655,3741,3828,3916,4005,4095,4186,4278,4371,4465,4560,4656,4753,4851,4950,5050,5151,5253,5356,5460,5565,5671,5778,5886,5995,6105,6216,6328,6441,6555,6670,6786,6903,7021,7140,7260,7381,7503,7626,7750,7875,8001,8128,8256};
const int combination_cn2_size=130;

//Prime list
const int prime_numset[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,991,997,1009,1013,1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,1087,1091,1093,1097,1103,1109,1117,1123,1129,1151,1153,1163,1171,1181,1187,1193,1201,1213,1217,1223,1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,1297,1301,1303,1307,1319,1321,1327,1361,1367,1373,1381,1399,1409,1423,1427,1429,1433,1439,1447,1451,1453,1459,1471,1481,1483,1487,1489,1493,1499,1511,1523,1531,1543,1549,1553,1559,1567,1571,1579,1583,1597,1601,1607,1609,1613,1619,1621,1627,1637,1657,1663,1667,1669,1693,1697,1699,1709,1721,1723,1733,1741,1747,1753,1759,1777,1783,1787,1789,1801,1811,1823,1831,1847,1861,1867,1871,1873,1877,1879,1889,1901,1907,1913,1931,1933,1949,1951,1973,1979,1987,1993,1997,1999,2003,2011,2017,2027,2029,2039,2053,2063,2069,2081,2083,2087,2089,2099,2111,2113,2129,2131,2137,2141,2143,2153,2161,2179,2203,2207,2213,2221,2237,2239,2243,2251,2267,2269,2273,2281,2287,2293,2297,2309,2311,2333,2339,2341,2347,2351,2357,2371,2377,2381,2383,2389,2393,2399,2411,2417,2423,2437,2441,2447,2459,2467,2473,2477,2503,2521,2531,2539,2543,2549,2551,2557,2579,2591,2593,2609,2617,2621,2633,2647,2657,2659,2663,2671,2677,2683,2687,2689,2693,2699,2707,2711,2713,2719,2729,2731,2741,2749,2753,2767,2777,2789,2791,2797,2801,2803,2819,2833,2837,2843,2851,2857,2861,2879,2887,2897,2903,2909,2917,2927,2939,2953,2957,2963,2969,2971,2999,3001,3011,3019,3023,3037,3041,3049,3061,3067,3079,3083,3089,3109,3119,3121,3137,3163,3167,3169,3181,3187,3191,3203,3209,3217,3221,3229,3251,3253,3257,3259,3271,3299,3301,3307,3313,3319,3323,3329,3331,3343,3347,3359,3361,3371,3373,3389,3391,3407,3413,3433,3449,3457,3461,3463,3467,3469,3491,3499,3511,3517,3527,3529,3533,3539,3541,3547,3557,3559,3571,3581,3583,3593,3607,3613,3617,3623,3631,3637,3643,3659,3671,3673,3677,3691,3697,3701,3709,3719,3727,3733,3739,3761,3767,3769,3779,3793,3797,3803,3821,3823,3833,3847,3851,3853,3863,3877,3881,3889,3907,3911,3917,3919,3923,3929,3931,3943,3947,3967,3989,4001,4003,4007,4013,4019,4021,4027,4049,4051,4057,4073,4079,4091,4093,4099,4111,4127,4129,4133,4139,4153,4157,4159,4177,4201,4211,4217,4219,4229,4231,4241,4243,4253,4259,4261,4271,4273,4283,4289,4297,4327,4337,4339,4349,4357,4363,4373,4391,4397,4409,4421,4423,4441,4447,4451,4457,4463,4481,4483,4493,4507,4513,4517,4519,4523,4547,4549,4561,4567,4583,4591,4597,4603,4621,4637,4639,4643,4649,4651,4657,4663,4673,4679,4691,4703,4721,4723,4729,4733,4751,4759,4783,4787,4789,4793,4799,4801,4813,4817,4831,4861,4871,4877,4889,4903,4909,4919,4931,4933,4937,4943,4951,4957,4967,4969,4973,4987,4993,4999,5003,5009,5011,5021,5023,5039,5051,5059,5077,5081,5087,5099,5101,5107,5113,5119,5147,5153,5167,5171,5179,5189,5197,5209,5227,5231,5233,5237,5261,5273,5279,5281,5297,5303,5309,5323,5333,5347,5351,5381,5387,5393,5399,5407,5413,5417,5419,5431,5437,5441,5443,5449,5471,5477,5479,5483,5501,5503,5507,5519,5521,5527,5531,5557,5563,5569,5573,5581,5591,5623,5639,5641,5647,5651,5653,5657,5659,5669,5683,5689,5693,5701,5711,5717,5737,5741,5743,5749,5779,5783,5791,5801,5807,5813,5821,5827,5839,5843,5849,5851,5857,5861,5867,5869,5879,5881,5897,5903,5923,5927,5939,5953,5981,5987,6007,6011,6029,6037,6043,6047,6053,6067,6073,6079,6089,6091,6101,6113,6121,6131,6133,6143,6151,6163,6173,6197,6199,6203,6211,6217,6221,6229,6247,6257,6263,6269,6271,6277,6287,6299,6301,6311,6317,6323,6329,6337,6343,6353,6359,6361,6367,6373,6379,6389,6397,6421,6427,6449,6451,6469,6473,6481,6491,6521,6529,6547,6551,6553,6563,6569,6571,6577,6581,6599,6607,6619,6637,6653,6659,6661,6673,6679,6689,6691,6701,6703,6709,6719,6733,6737,6761,6763,6779,6781,6791,6793,6803,6823,6827,6829,6833,6841,6857,6863,6869,6871,6883,6899,6907,6911,6917,6947,6949,6959,6961,6967,6971,6977,6983,6991,6997,7001,7013,7019,7027,7039,7043,7057,7069,7079,7103,7109,7121,7127,7129,7151,7159,7177,7187,7193,7207,7211,7213,7219,7229,7237,7243,7247,7253,7283,7297,7307,7309,7321,7331,7333,7349,7351,7369,7393,7411,7417,7433,7451,7457,7459,7477,7481,7487,7489,7499,7507,7517,7523,7529,7537,7541,7547,7549,7559,7561,7573,7577,7583,7589,7591,7603,7607,7621,7639,7643,7649,7669,7673,7681,7687,7691,7699,7703,7717,7723,7727,7741,7753,7757,7759,7789,7793,7817,7823,7829,7841,7853,7867,7873,7877,7879,7883,7901,7907,7919,7927,7933,7937,7949,7951,7963,7993,8009,8011,8017,8039,8053,8059,8069,8081,8087,8089,8093,8101,8111,8117,8123,8147,8161,8167,8171,8179,8191,8209,8219,8221,8231,8233,8237,8243,8263,8269,8273,8287,8291,8293,8297,8311,8317,8329,8353,8363,8369,8377,8387,8389,8419,8423,8429,8431,8443,8447,8461,8467,8501,8513,8521,8527,8537,8539,8543,8563,8573,8581,8597,8599,8609,8623,8627,8629,8641,8647,8663,8669,8677,8681,8689,8693,8699,8707,8713,8719,8731,8737,8741,8747,8753,8761,8779,8783,8803,8807,8819,8821,8831,8837,8839,8849,8861,8863,8867,8887,8893,8923,8929,8933,8941,8951,8963,8969,8971,8999,9001,9007,9011,9013,9029,9041,9043,9049,9059,9067,9091,9103,9109,9127,9133,9137,9151,9157,9161,9173,9181,9187,9199,9203,9209,9221,9227,9239,9241,9257,9277,9281,9283,9293,9311,9319,9323,9337,9341,9343,9349,9371,9377,9391,9397,9403,9413,9419,9421,9431,9433,9437,9439,9461,9463,9467,9473,9479,9491,9497,9511,9521,9533,9539,9547,9551,9587,9601,9613,9619,9623,9629,9631,9643,9649,9661,9677,9679,9689,9697,9719,9721,9733,9739,9743,9749,9767,9769,9781,9787,9791,9803,9811,9817,9829,9833,9839,9851,9857,9859,9871,9883,9887,9901,9907,9923,9929,9931,9941,9949,9967,9973};//prime between [0,300], since the Max_K fit 211
const int prime_size=1229;

enum matrix_operation{
	chpos,
	eliml
};//cmd for matrix operation

typedef struct _elim_stack{
	matrix_operation opr;
	short int addr1;//eliml:base line, chops: change line addr2 with addr1
	short int addr2;//opr line
}elim_stack;

//raptor encoder class							
class raptor_encoder
{
private:
	int K;
	int SYMBOL_SIZE;
	int BLOCK_SIZE;
	int S;
	int H;
	int L;
	int Lp;
	int Pr;
	int **U;//3xPr
	int error;
	clock_t start,finish;
	long long mtr_op_size;
	elim_stack *mtr_op;//elimination process
	byte **source_block;//KxSYMBOL_SIZE
	byte **des_block;//PrxSYMBOL_SIZE
public:
	raptor_encoder(int, int, double);
	~raptor_encoder();
	void reset_raptor(int, int, double);
	void base_process_encode(byte *, int, int *);//deal with single block, whose size is BLOCK_SIZE with Byte, if the lenth < BLOCK_SIZE, then pass lenth to funcion(other buffer padding with 0), otherwise BLOCK_SIZE
	int get_sub_block(byte* , int );
	//encode time count
	void start_encode_clock();
	clock_t get_encode_clock();
	//clear buffer of source and destination blocks
	void clear_src_blocks();
	void clear_des_blocks();
	//error bit deal
	int get_err_bit();
	void clear_err_bit();
	int get_Pr();
	void get_A(long long **_A);
private:
	//encoder module
	void ppscalculate(int _K, int _H, int _L, int _Lp, int _S, int _Pr, long long **AA, int **_U);
	void Inversecalculate(long long **AA, int _L, int _H, int _K, int _S, elim_stack* _mtr_op,long long *_mtr_op_size);
	void Ensymbol_process(elim_stack *_mtr_op, long long _mtr_op_size, byte **VV,int _SYMBOL_SIZE, int _K, int _L, byte **_intermid_index);
	void EncoderCalculate(int _K, int _L, int _Lp, elim_stack *_mtr_op, long long _mtr_op_size, byte **VV, int _H, int _S, int _SYMBOL_SIZE, int _Pr, int** _U, byte** e);
	//LT process
	void Lncalculate(int _Lp, byte **c, int d, int a, int b, int _L, int _SYMBOL_SIZE, byte* e);
};

class raptor_decoder
{
private:
	int K;
	int SYMBOL_SIZE;
	int BLOCK_SIZE;
	int S;
	int H;
	int L;
	int Lp;
	int type;
	int error;
	clock_t start,finish;
	int **U;//3xPr
	long long **B;//LxL
	byte **source_block;//KxSYMBOL_SIZE: decode result
	typedef void (raptor_decoder::*___DecoderCalculate)(int _K, int _Lp, long long **_B, byte **erc1, int _N, int _M, int _L, int _SYMBOL_SIZE, int** _U, int* _ESIs, int r, byte**y);
	___DecoderCalculate pDecoderCalculate;
	
private:
	int N;
	int M;
	int unfailsouceblock;
	double Overhead;//get how many blocks more
	int *ESIs;
	byte **rev_block;//NxSYMBOL_SIZE: receive block
	byte **backup_block;//NxSYMBOL_SIZE
public:
	raptor_decoder(int, int, double, double, int type=0);
	~raptor_decoder();
	void reset_raptor(int, int, double, double, int type=0);
	void process_decode(byte **, int , byte* , int , int *, int*);
	//count decode time
	void start_decode_clock();
	clock_t get_decode_clock();
	//error count and dealing process
	int get_err_bit();
	void clear_err_bit();
	//clear buffer of source and destination blocks
	void clear_src_blocks();
	void clear_rev_blocks();
	//count the decode success packets, after the decoding fail, error code is 4
	int get_fail_decode();
	void clear_fail_decode();
	void add_fail_decode();
	void get_A(long long **_A, int row, int col);
	
private:	
	void base_process_decode(byte **,int *, int , int *, int* );//Dealing with decode process
	void get_decode_result(byte* , int );//write decode result
	//decoder module
	void ppscalculate(int _K, int _H, int _L, int _Lp, int _S, long long **AA);
	void AAcalculate( long long** A, int _K, int* _ESIs, int _N, int _L, int* d, int* a, int* b, int _Lp, long long** Anew);
	//decode solution 2.
	void at_AAcalculate( long long** AA, int _K, int* _ESIs, int _N, int _L, int _M, int* d, int* a, int* b, int _Lp, elim_stack *_mtr_op, long long *_mtr_op_size);
	void at_Desymbol_process(elim_stack *_mtr_op, long long _mtr_op_size, byte **cdc, byte **erc1, int _SYMBOL_SIZE, int _M, int _N, byte **_intermid_index);
	void at_DecoderCalculate(int _K, int _Lp, long long **_B, byte **erc1, int _N, int _M, int _L, int _SYMBOL_SIZE, int** _U, int* _ESIs, int r, byte**y);
	//decode solution 1.
	void GEcalculate(long long **AA, int _K, int _L, int _N, int _M, int _SYMBOL_SIZE, int r, byte** _intermid_index);
	void DecoderCalculate(int _K, int _Lp, long long **_B, byte **erc1, int _N, int _M, int _L, int _SYMBOL_SIZE, int** _U, int* _ESIs, int r, byte**y);
	//LT process
	void Lncalculate(int _Lp, byte **c, int d, int a, int b, int _L, int _SYMBOL_SIZE, byte* e);
};


//math function that used in all program 
int rg(int );
int rd(int , int , int );
void tn(int , int , int , int *);
void sh(int , int , int* );
int count1_of_int32(int ,int );
int TripleCalculate(int , int , int , int **);
void memxor(void *__des, void* __srct, long long __lenth);
