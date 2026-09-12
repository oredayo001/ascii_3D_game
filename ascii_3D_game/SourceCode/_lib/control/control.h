#pragma once

#pragma once
#pragma once

#include<stdint.h>
#include<stdbool.h>

#include"macro/macro.h"

//#include"Windows.h"

#define KEY_LIST_X(X)\
/*--- movement ---*/\
X(up,			'W')\
X(left,			'A')\
X(down,			'S')\
X(right,		'D')\
/*--- up down ---*/\
X(jump			,VK_SPACE)\
X(shift			,VK_SHIFT)\
/*--- arrow ---*/\
X(arrow_up		,VK_UP)\
X(arrow_left	,VK_LEFT)\
X(arrow_down	,VK_DOWN)\
X(arrow_right	,VK_RIGHT)\
/*--- ctrl ---*/\
X(ctrl			,VK_CONTROL)\
/*--- mb ---*/\
X(mbLeft		,VK_LBUTTON)\
X(mbRight		,VK_RBUTTON)\
/*--- esc ---*/\
X(esc			,VK_ESCAPE)\
/*--- debug ---*/\
X(debugBreakPoint			,VK_F10)\
X(debugShadingToggle			,'8')\
X_MACRO_END


#define AS_ENUM_X(name,...) ATTACH(vk_,name),
enum{
	KEY_LIST_X(AS_ENUM_X)
	AS_ENUM_X(max)
};
#undef AS_ENUM_X



typedef struct KeyStates{
	uint32_t h;//hold
	uint32_t p;//plessed
	uint32_t r;//released
}KeyStates;

static inline bool lib_keyboardCheck(uint32_t key, uint32_t state){
	return (state >> (key)) & 1;
}
static inline void lib_keyboardSet(uint32_t key, uint32_t* state){
	*state |= 1 << key;
}

void lib_updateControl(KeyStates*);

void lib_updateKeyStateUsingInput(KeyStates* s, uint32_t input);



void lib_updateMouse();
int lib_getMouseDx();
int lib_getMouseDy();