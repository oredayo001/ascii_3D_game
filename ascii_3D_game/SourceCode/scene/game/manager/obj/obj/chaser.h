#pragma once

#include"../objDef.h"
#include"math/vec3.h"

#define __CHASER_POWDER_NUM__ 32
struct objChaser{
	OBJ_BASE;
	InstPtr target;
	uint8_t powderIndex_write;
	uint8_t powderIndex_read;
	//pad6
	vec3 powders[__CHASER_POWDER_NUM__];//ƒ‹[ƒgŒŸõ‚Ì‚â‚Â ˆê’U‚±‚±‚É’u‚­/
};