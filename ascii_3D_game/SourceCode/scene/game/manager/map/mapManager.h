#pragma once

#include "math/vec3.h"

#define WORLD_SIZE 16384
#define POS_MAX (WORLD_SIZE/2)
#define POS_MIN (-POS_MAX)
#define GRID_NUM 16

static inline int getGrid(int x){
	return ((x + POS_MAX) * GRID_NUM) / WORLD_SIZE;
}

struct RenderContext;
typedef struct RenderContext RenderContext;

//pushModelしたモデルをマップとして初期化する/
void createMap();
//マップを描画/
void renderMap(RenderContext* rCtx);
//マップを開放/
void destroyMap();

// --- collision --- /

typedef struct{
	vec3 norm;
	float force;
	float currentMinDist;
	float nextMinDist;
	struct Triangle* tri;
}fcResult;//floor ceiling result

typedef struct{
	int isHit;
	float force;      
	struct Triangle* tri;             
	vec3 norm;
}wallResult;

void getNearestFloorDist(vec3 p, vec3 v, float checkRange, fcResult* result);

void getNearestCeilingDist(vec3 p, vec3 v, float checkRange, fcResult* result);

void getNearestWall(vec3 p, vec3 v, float r, float h, wallResult* result);
