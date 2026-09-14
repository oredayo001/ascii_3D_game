#pragma once

#include"math/vec3.h"

#define MAX_V_CNT 10000
typedef struct mdlTextureRLE{
	int index;
	int cnt;
}mdlTextureRLE;
typedef struct bbox_t{
	vec3 max;
	vec3 min;
}bbox_t;
typedef struct Model3D{
	//展開済み/
	vec3* vertices;
	vec3* norms;
	vec2* uv;
	mdlTextureRLE* txInfo;
	int triCnt;
	bbox_t bbox;
	//pad4
}Model3D;

enum{
	modelLoader_error,
	modelLoader_ok,
};

int pushLoadRequest(const char* path, Model3D* /*入れたいやつ*/target);

int loadAllModel();
//一時的なメモリに確保する　戻り値はmarker/
int loadAllModel_temp();

//ポインタ　ポインタのポインタではない 中のポインタはnull埋めする/
void destroyModel(Model3D*);

static inline int isModelLoadError(int result){ return result == modelLoader_error; }