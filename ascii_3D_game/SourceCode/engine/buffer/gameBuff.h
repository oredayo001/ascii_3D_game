#pragma once
#include<stdint.h>
#include"common.h"

//8byte
#define GAME_BUFF_ALIGN_SIZE_L2 (3)
#define GAME_BUFF_ALIGN_SIZE (1<<GAME_BUFF_ALIGN_SIZE_L2)

//gameBuffのサイズ
#define GAME_BUFF_SIZE_KB 512
#define GAME_BUFF_SIZE (1024 * (GAME_BUFF_SIZE_KB))
#define ALLOCATOR_MAX_CNT (GAME_BUFF_SIZE/GAME_BUFF_ALIGN_SIZE)

//アロケータのデバックが有効か　1でもenubleDebugが無効なら無効になる/
#define __ENABLE_ALOCATOR_DEBUG 1
#define ENABLE_ALOCATOR_DEBUG (__ENABLE_ALOCATOR_DEBUG&&ENABLE_DEBUG)

//8byteアライメント/
#define ALIGN_GM(byte) (_byte + (GAME_BUFF_ALIGN_SIZE - 1)) & (~(GAME_BUFF_ALIGN_SIZE - 1))
#define BYTE_TO_COUNT(byte) ((byte)>>GAME_BUFF_ALIGN_SIZE_L2)
#define BYTE_TO_COUNT_U(byte) (((byte)+(GAME_BUFF_ALIGN_SIZE-1))>>GAME_BUFF_ALIGN_SIZE_L2)

enum{
	gameBuffStack_engine,
	gameBuffStack_scene,
	gameBuffStack_stage,
};

enum{
	gmDebugFlag_usingTemp,
};

typedef struct AllocatorInfo{
	uint64_t* buff;
	int current;
	int capacity;
#if ENABLE_ALOCATOR_DEBUG
	int currentStack;
	uint32_t useableFlag;//使ってはいけないかどうかのフラグ/
	int usedBackMem;
#endif
}AllocatorInfo;

extern AllocatorInfo gameAllocator;

//gm:gameMemoly

// --- debug --- /

#if ENABLE_ALOCATOR_DEBUG
#define _FLAG (gameAllocator.useableFlag)
#define _FLAG_ON(i) _FLAG |= 1<<(i)
#define _FLAG_OFF(i) _FLAG &= ~(1<<(i))
#define _FLAG_READ(i) (((_FLAG)>>(i))&1)
static inline void gm_d_lockMemoly(){
	ASSERT(!_FLAG, "使ってはいけないときに一時として使った gm");
	_FLAG_ON(gmDebugFlag_usingTemp);
}
static inline void gm_d_unlockMemoly(){
	ASSERT(_FLAG_READ(gmDebugFlag_usingTemp), "ロックされてないのにロック解除　gm");
	_FLAG_OFF(gmDebugFlag_usingTemp);
}
static inline void gm_d_pushStack(){
	gameAllocator.currentStack++;
}
static inline void gm_d_popStack(){
	gameAllocator.currentStack--;
}
static inline void gm_d_assertStack(int stack){
	ASSERT(gameAllocator.currentStack == stack, "別のスタックに入れようとしてる gm");
}

#else
#define gm_d_usedAsTempMem() DO_NOTHING
#define gm_d_freeTempMem() DO_NOTHING
#define gm_d_pushStack() DO_NOTHING
#define gm_d_popStack() DO_NOTHING
#define gm_d_assertStack() DO_NOTHING
#endif

// --- 機能 --- /

//今の場所をもらう/
static inline int gm_getMarker(){ return gameAllocator.current; }
//マーカーの位置まで解放 これを使うときはロックする　で使い終わったらアンロックする/
static inline void gm_freeToMarker(int marker){ gameAllocator.current = marker; }
//今の位置をもらう/
//一時的に使うときから何バイトいるかわからん時とかに使える/
static inline void* gm_getCurrent(){
	ASSERT(!_FLAG, "使ってはいけないときに一時として使った gm");
	return &(gameAllocator.buff[gameAllocator.current]);
}
//ポインタをバイト分進める/
//何バイトいるかわからん時にgetCurrentを使った後に使うのがいい/
static inline void gm_increment(size_t byte){
	int inc = (int)BYTE_TO_COUNT_U(byte);
	gameAllocator.current += inc;

	ASSERT(gameAllocator.current < gameAllocator.capacity, "アロケーターのバッファあふれ gm");
}
//メモリをもらう/
static inline void* gm_allocate(size_t byte){
	ASSERT(!_FLAG, "使ってはいけないときに一時として使った gm");
	void* r = gm_getCurrent();
	gm_increment(byte);
	return r;
}
//一時的に使うけどスタックに影響するとだめな時に使う/
static inline void* gm_allocate_back(size_t byte){
	int cnt = (int)BYTE_TO_COUNT_U(byte);
#if ENABLE_ALOCATOR_DEBUG
	gameAllocator.usedBackMem = 1;
#endif
	gameAllocator.capacity -= cnt;
	return &(gameAllocator.buff[gameAllocator.capacity]);
}
//backの開放/
static inline void gm_free_back(){
#if ENABLE_ALOCATOR_DEBUG
	ASSERT(gameAllocator.usedBackMem, "必要のない開放 gm");
	gameAllocator.usedBackMem = 0;
#endif
	gameAllocator.capacity = ALLOCATOR_MAX_CNT;
}

static inline int gm_getMarker_back(){
	return gameAllocator.capacity;
}

static inline void gm_free_back_to_marker(int marker){
	gameAllocator.capacity = marker;
}

#undef _FLAG
#undef _FLAG_ON
#undef _FLAG_OFF
#undef _FLAG_READ