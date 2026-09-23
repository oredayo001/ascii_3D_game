#pragma once

#include"math/vec3.h"
#include"control/control.h"


#include <stdint.h>
#include <float.h>
#include <stddef.h>
//fps
#include <time.h>
//malloc free
#include <stdlib.h>
//printf
#include <stdio.h>
//getch(debug)
#include <conio.h>
//memset
#include <string.h>

#include <assert.h>

// --- debug --- /
#define ENABLE_DEBUG 1
#define ENABLE_DEBUG_MAIN_STRONG_X 1
#define _ENABLE_DEBUG_MAIN (ENABLE_DEBUG||ENABLE_DEBUG_MAIN_STRONG_X)

#include"debug/debug.h"
#if _ENABLE_DEBUG_MAIN + 0
typedef struct DebugMembers{
	int lastFpsCount;
	int fps_noFix;
	int fps;
	uint32_t fps_sum;
	uint32_t fps_countedNum;
	int triCnt;
	int triCntStatic;
	float fovAngle;
	int memolyUsed;
	int objNum;
}DebugMembers;
extern DebugMembers debugMember;//main.cpp
//リテラルのみ/
#define vec3Print(txt,v) printf(txt "(%.3f,%.3f,%.3f)",v.x,v.y,v.z)
#else
#define vec3Print(txt,v) do{}while(0)
#endif
// --- fps --- /

//とりあえず/
#define FPS 30
#define INTERVAL (CLOCKS_PER_SEC/FPS)
#define MAX_LOOP_CNT 5

// --- 画面 --- /

//simdを使う場合16の倍数がいい というかそうせんと/
#define WIDTH 128
#define HEIGHT 128

// --- thread --- /

//とりあえず/
#define THREAD_NUM 4
#define THREAD_STACK_SIZE (4*1024)//とりあえず/

//便利/
#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

//危ないマクロを消す/
#undef max
#undef min
#undef far
#undef near
//てか大文字にしろよ怖いな/