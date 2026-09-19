#pragma once

#include"../objDef.h"
#include"math/vec3.h"


typedef struct playerInMemoly{
	uint16_t jumpInFrame;//ジャンプが押されてからのフレーム/
	uint16_t jumpFrame;//ジャンプをしてからのフレーム/
	//flag
	uint16_t jumped;//ジャンプをしたか 入力じゃなくてゲーム側が管理/
}playerInMemoly;

//!typedefつけてはならない/
struct objPlayer{
	OBJ_BASE;//継承的な/
	vec2Basis cameraIn;
	float height;
	playerInMemoly inputMemoly;
	uint16_t flags;
};

