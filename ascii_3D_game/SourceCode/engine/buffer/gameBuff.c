#include"gameBuff.h"

uint64_t gameBuff[ALLOCATOR_MAX_CNT];

AllocatorInfo gameAllocator = {
	.buff = gameBuff,
	.current = 0,
	.capacity = ALLOCATOR_MAX_CNT,
#if ENABLE_ALOCATOR_DEBUG
	.currentStack = -1,
	.useableFlag = 0,
#endif
};