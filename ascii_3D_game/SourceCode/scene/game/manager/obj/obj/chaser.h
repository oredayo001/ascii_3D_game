#pragma once

#include"../objDef.h"
#include"math/vec3.h"


struct objChaser{
	OBJ_BASE;
	InstPtr target;
	vec3 powders[32];//ƒ‹[ƒgŒŸõ‚Ì‚â‚Â/
};