#pragma once
#include<stdint.h>
#include"common.h"


//#############################################################################################
// Liniar Allocator
//#############################################################################################

//線形アロケーター的な/
typedef struct LiniarAllocator{
	uint64_t* p;
	size_t cnt;
#if ENABLE_DEBUG + 0
	size_t capa;
#endif
}LiniarAllocator;

// --- cpp --- /

//作成/
//LiniarAllocator* createLA(size_t byte);
//破棄/
//void LADestroy(LiniarAllocator**);

// --- inline --- /

//値をセット/
static inline void setLAParam(LiniarAllocator* allocator, size_t byte){
#if ENABLE_DEBUG
	allocator->capa = byte;
#endif
	memset(allocator->p, 0, byte);
}

//0クリア freeはしない/
static inline void clearLAParam(LiniarAllocator* allocator){
	memset(allocator, 0, sizeof(LiniarAllocator));
}

static inline void* allocateLA(LiniarAllocator* allocator,size_t byte){
	size_t size = (byte + 7) >> 3;
#if ENABLE_DEBUG + 0
	ASSERT((allocator->cnt + size) <= (allocator->capa), "リニアアロケーターのあふれ");
#endif
	void* r = &(allocator->p[allocator->cnt]);
	allocator->cnt += size;
	return r;
}

static inline void clearLA(LiniarAllocator* allocator){
	allocator->cnt = 0;
}

