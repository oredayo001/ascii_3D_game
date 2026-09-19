#pragma once

#include"loader/modelLoader.h"
#include"common.h"

// --- struct --- /

struct Screen;
typedef struct Screen Screen;

typedef struct{
	int triCnt;
	//pad 4byte
	struct Triangle{
		vec3 v[3];
		vec3 norm;
	} *tri;//ちゃんと管理されてる/
	vec2* uv;
	mdlTextureRLE* texInfo;
} StaticRenderStack;


//ゲーム側でcameraControlerってのを作ってそいつが動かす/
typedef struct Camera{
	vec3 p;
	Basis b;//b.zがangle
	vec3 lightVec;
	float fov;//角度　piを除く/
	float shadeLength;
}Camera;
//fovの計算/

//floatのangleからfovを求めるやつ　主にcameraControlerが呼び出してcameraのfovに入れる想定 まあ重たいけど毎フレームでも1回なら無視できるやろ/
static inline float angleToFov(float angle){
	//定数/
	const float screenScale = (float)((WIDTH < HEIGHT ? WIDTH : HEIGHT) >> 1);
	//早期撃墜(?)/
	if(angle <= 0.0001f) return 999999.0f;
	//fov = 1/(tan(angle)/2)
	return screenScale / (tanf((angle * PI) * .5f));
}


//カメラのメモリ確保/
Camera* createCamera();
//カメラを開放/
//!ポインタ変数をNULLに書き換えるためにポインタのポインタを入れる/
void cameraDestroy(Camera** c);


typedef struct RenderContext{
	//コピー/
	Camera* c;
	Screen* sc;
}RenderContext;

//メモリ確保 カメラスクリーンはコピー メモリ管理はしない/
RenderContext* createRenderContext(Screen* __restrict sc, Camera* __restrict c);
//カメラと同じくポインタのポインタ/
void renderContextDestroy(RenderContext** ctx);


// --- static buff --- /

//pushModelしたのをstaticRenderStackに書き込んだやつを返す/
//!つまりこれ呼ぶ前にpushModelをしないと何も入ってないstackが出来上がる/
StaticRenderStack* createStaticRenderStack();
//描画/
void renderStaticRenderStack(StaticRenderStack* __restrict staticStack,const RenderContext* const __restrict ctx);
//解放/
void destroyStaticRenderStack(StaticRenderStack** s);

// --- stack render --- /

//主に動くキャラの描画/
void renderStackAll(RenderContext* ctx);

// --- push model --- /

//z向きのスケール1/
void pushModel(const Model3D* mdl, vec3 p);


void pushModelBS3(const Model3D* mdl, vec3 p, Basis angle, vec3 scale);
void pushModelBS(const Model3D* mdl, vec3 p, Basis angle, float scale);
void pushModelB(const Model3D* mdl, vec3 p, Basis angle);


void pushModelAS3(const Model3D* mdl, vec3 p, vec3 angle, vec3 scale);
void pushModelAS(const Model3D* mdl, vec3 p, vec3 angle, float scale);
void pushModelA(const Model3D* mdl, vec3 p, vec3 angle);


void pushModelA2S3(const Model3D* mdl, vec3 p, vec2 angle, vec3 scale);
void pushModelA2S(const Model3D* mdl, vec3 p, vec2 angle, float scale);
void pushModelA2(const Model3D* mdl, vec3 p, vec2 angle);

//static
void pushStaticRenderStack(const StaticRenderStack* srs);

// --- shading --- /
void shadingScreen(Screen* sc);//zbuffをもとに遠くに行くほど暗くする/