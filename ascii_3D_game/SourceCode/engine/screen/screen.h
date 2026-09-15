#pragma once


#include "common.h"
#include "../engineTypes.h"


typedef struct Screen{
	char* buff;//printfに渡される奴 (width + \n) * height
	pixel_t* screen;//色情報 width * height
	pixel_t* prevscreen;
	float* zbuff;
	size_t size;
}Screen;


#define ASCII_8 0
#define ASCII_32 1
#define ASCII_64 2
#define ASCII_SHADE_MODE ASCII_32


#if ASCII_SHADE_MODE == ASCII_8
#define ASCII_SHADE_TABLE_LIST_X(X)\
X(0     ,' ')\
X(1     ,'.')\
X(2     ,':')\
X(3     ,'~')\
X(4     ,'=')\
X(5     ,'*')\
X(6    ,'#')\
X(7    ,'@')\
X_MACRO_END
#elif ASCII_SHADE_MODE == ASCII_32
#define ASCII_SHADE_TABLE_LIST_X(X)\
X(0     ,' ')\
X(1     ,'.')\
X(2     ,'`')\
X(3     ,',')\
X(4     ,':')\
X(5     ,';')\
X(6     ,'-')\
X(7     ,'~')\
X(8     ,'=')\
X(9     ,'<')\
X(10    ,'>')\
X(11    ,'i')\
X(12    ,'!')\
X(13    ,'r')\
X(14    ,'+')\
X(15    ,'*')\
X(16    ,'s')\
X(17    ,'o')\
X(18    ,'7')\
X(19    ,'v')\
X(20    ,'e')\
X(21    ,'c')\
X(22    ,'4')\
X(23    ,'X')\
X(24    ,'P')\
X(25    ,'O')\
X(26    ,'K')\
X(27    ,'#')\
X(28    ,'d')\
X(29    ,'m')\
X(30    ,'B')\
X(31    ,'@')\
X_MACRO_END
#elif ASCII_SHADE_MODE == ASCII_64

#define ASCII_SHADE_TABLE_LIST_X(X)\
X(0    ,' ')\
X(1    ,'^')\
X(2    ,'.')\
X(3    ,'"')\
X(4    ,'`')\
X(5    ,'\'')\
X(6    ,',')\
X(7    ,'_')\
X(8    ,':')\
X(9    ,'/')\
X(10   ,';')\
X(11   ,'\\')\
X(12   ,'-')\
X(13   ,'|')\
X(14   ,'~')\
X(15   ,'(')\
X(16   ,'=')\
X(17   ,')')\
X(18   ,'<')\
X(19   ,'[')\
X(20   ,'>')\
X(21   ,']')\
X(22   ,'i')\
X(23   ,'{')\
X(24   ,'!')\
X(25   ,'}')\
X(26   ,'r')\
X(27   ,'I')\
X(28   ,'+')\
X(29   ,'l')\
X(30   ,'*')\
X(31   ,'?')\
X(32   ,'s')\
X(33   ,'f')\
X(34   ,'o')\
X(35   ,'j')\
X(36   ,'7')\
X(37   ,'t')\
X(38   ,'v')\
X(39   ,'z')\
X(40   ,'e')\
X(41   ,'x')\
X(42   ,'c')\
X(43   ,'C')\
X(44   ,'4')\
X(45   ,'Z')\
X(46   ,'X')\
X(47   ,'Y')\
X(48   ,'P')\
X(49   ,'U')\
X(50   ,'O')\
X(51   ,'a')\
X(52   ,'K')\
X(53   ,'n')\
X(54   ,'#')\
X(55   ,'w')\
X(56   ,'d')\
X(57   ,'h')\
X(58   ,'m')\
X(59   ,'q')\
X(60   ,'B')\
X(61   ,'W')\
X(62   ,'@')\
X(63   ,'M')\
X_MACRO_END
#endif

#define ASCII_TABLE_LIST_X(X)\
X(sp		,' ')\
X(hline		,'-')/*横*/\
X(vline		,'|')/*縦*/\
X(ruline	,'/')/*右上*/\
X(luline	,'\\')/*左上*/\
X(plus		,'+')\
X(dot		,'.')\
X_MACRO_END

#define AS_ENUM_ASCII_TABLE_X(name,...) ATTACH(a_,name),
#define AS_ENUM_ASCII_SHADE_TABLE_X(name,...) ATTACH(asciiShade_,name),
//ascii
enum{
	ASCII_SHADE_TABLE_LIST_X(AS_ENUM_ASCII_SHADE_TABLE_X)
	AS_ENUM_ASCII_SHADE_TABLE_X(max)//num
	AS_ENUM_ASCII_TABLE_X(minm1 = asciiShade_max - 1)//max-1
	ASCII_TABLE_LIST_X(AS_ENUM_ASCII_TABLE_X)
	AS_ENUM_ASCII_TABLE_X(max)
};

#define ASCII_SHADE_MAX asciiShade_max

Screen* createScreen();
void updateScreen(Screen* screen);
void flushScreen(Screen* sc);
//!スクリーンポインタのアドレス/
void destroyScreen(Screen** s);



static inline int getScreenPos(int x, int y){
	return x + (y * WIDTH);//横に連続/
}
static inline pixel_t* getScreenAtPos(Screen* sc,int x, int y){
	return &sc->screen[getScreenPos(x, y)];
}

static inline void clearPrevScreen(Screen* sc){
	memset(sc->prevscreen, 0, (WIDTH * HEIGHT) * sizeof(pixel_t));
}

static inline void clearScreen(Screen* sc){
	memset(sc->screen, 0, (WIDTH * HEIGHT) * sizeof(pixel_t));
	memset(sc->zbuff, 0, (WIDTH * HEIGHT) * sizeof(float));

}