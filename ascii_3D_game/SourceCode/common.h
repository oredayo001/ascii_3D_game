#pragma once

#include"math/vec3.h"
#include"control/control.h"


#include<stdint.h>
#include<stddef.h>
//fps
#include<time.h>
//malloc free
#include<stdlib.h>
//printf
#include<stdio.h>
//getch(debug)
#include<conio.h>
//memset
#include<string.h>

// --- debug --- /
#define ENABLE_DEBUG 1

#include"debug/debug.h"
#if ENABLE_DEBUG + 0
typedef struct DebugMembers{
	int fps;
	int triCnt;
	int triCntStatic;
	float fovAngle;
	int memolyUsed;
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

//simdを使う場合32の倍数がいい というかそうせんと/
#define WIDTH 128
#define HEIGHT 128



//危ないマクロを消す/
#undef max
#undef min
#undef far
#undef near
//てか大文字にしろよ怖いな/