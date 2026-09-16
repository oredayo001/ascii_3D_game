#include "cameraManager.h"
#include "engine/graphics/render3d.h"
#include "engine/buffer/gameBuff.h"
#include "scene/commonManager/input.h"
#include "common.h"
#include "macro/macro.h"

//obj
#include"../obj/objManager.h"
#include"../obj/obj/player.h"



//###################################################################################
// debug camera controler
//###################################################################################

//---------------------------------------------
// private:
//---------------------------------------------
struct debugCameraControler{
	CameraColtrolerBase;
	float fovAngle;
};
static void debugCameraControlerUpdate(cameraControler* _controler){
	debugCameraControler* controler = (debugCameraControler*)_controler;
	Camera* c = controler->base.camera;

	float xdir = (float)(keyboardCheck(vk_right) - keyboardCheck(vk_left));
	float ydir = (float)(keyboardCheck(vk_jump) - keyboardCheck(vk_shift));
	float zdir = (float)(keyboardCheck(vk_up) - keyboardCheck(vk_down));

	//angle
	const float cameraSpd = 50.f;
	const float cameraYSpd = 50.f;
	const float cameraAngleXspd = .01f;
	const float cameraAngleYspd = .01f;
	float dx = getMouseDx();
	float dy = getMouseDy();
	if(dx || dy){
		float h = dx * cameraAngleXspd;
		float v = -dy * cameraAngleYspd;
		c->b.z = v3normalize(v3add(c->b.z, v3add(v3mul(c->b.x, h), v3mul(c->b.y, v))));

		c->b = createBasis(c->b.z);
	}

	vec3 v = (vec3){ 0 };

	float xspd = xdir;
	float zspd = zdir;

	v = v3add(v, v3mul(c->b.x, xspd));
	v = v3add(v, v3mul(c->b.z, zspd));

	v.y = 0.f;
	v = v3normalize(v);
	v.y = ydir * cameraYSpd;
	v.x *= cameraSpd;
	v.z *= cameraSpd;

	c->p = v3add(c->p, v);

	float fovDir = (float)(keyboardCheck(vk_arrow_up) - keyboardCheck(vk_arrow_down));
	float fovSpd = .1f;
	controler->fovAngle += fovDir * fovSpd;
	if(controler->fovAngle < 0) controler->fovAngle = .001f;
#if ENABLE_DEBUG + 0
	debugMember.fovAngle = controler->fovAngle;
#endif
	c->fov = angleToFov(controler->fovAngle);
}

//---------------------------------------------
// public:
//---------------------------------------------
//arg:NULL
void debugCameraControlerInitializer(cameraControler* _controler){
	debugCameraControler* controler = (debugCameraControler*)_controler;
	controler->base.update = debugCameraControlerUpdate;
	controler->fovAngle = .5f;
}
//###################################################################################
// 1人称視点/
//###################################################################################

//---------------------------------------------
// private:
//---------------------------------------------
struct gameCameraControler{
	CameraColtrolerBase;
	InstPtr follow;
	float fovAngle;
	float dist;
	vec3 angle;
	vec3 focus;
};
static void gameCameraControlerUpdate(cameraControler* _controler){
	gameCameraControler* controler = (gameCameraControler*)_controler;
	Camera* c = controler->base.camera;
	objBase* follow = getInstPtr(&(controler->follow));//ポインタをもらう　すでに死んでたらnull/

	if(follow == NULL) return;//死んでる/

	//angle
	const float cameraAngleXspd = .01f;
	const float cameraAngleYspd = .01f;
	float dx = getMouseDx();
	float dy = getMouseDy();
	if(dx || dy){
		Basis b = createBasis(controler->angle);
		vec3 lastAngle = b.z;
		float h = dx * cameraAngleXspd;
		float v = -dy * cameraAngleYspd;
		//TODO y制限/
		b.z = v3normalize(v3add(b.z, v3add(v3mul(b.x, h), v3mul(b.y, v))));
		const float maxY = .94f;
		if(maxY < fabsf(b.z.y)){
			//横だけ/
			vec3 hOnly = v3normalize(v3add(lastAngle, v3mul(b.x, h)));
			//縦を最大値にする/
			hOnly.y = b.z.y > 0 ? maxY : -maxY;
			//正規化/
			b.z = v3normalize(hOnly);
		}
		controler->angle = b.z;
		vec2 noY = angleToVec2(b.z);

		if(follow->attribute & objAtt_playable){
			vec2Basis* pCameraInput = follow->interfaces->playableInterface.getCameraIn(follow);
			*pCameraInput = createVec2Basis(noY);
		}
	}


	vec3 followPos = v3add(follow->render->p, v3mul(v3y, 160.f));
	//pos
	c->p = followPos;

	//c->p = targetPos;
	c->b = createBasis(controler->angle);
}

//---------------------------------------------
// public:
//---------------------------------------------
void gameCameraControlerInitializer(cameraControler* _controler, objBase* follow){
	gameCameraControler* controler = (gameCameraControler*)_controler;
	controler->fovAngle = .5f;
	controler->follow = makeInstPtr(follow);
	controler->angle = v3z;
	controler->dist = 160.f * 10;
	controler->focus = follow->render->p;
	_controler->update = gameCameraControlerUpdate;
	_controler->camera->fov = angleToFov(controler->fovAngle);
}

void gameCameraControlerSetFollow(gameCameraControler* controler, objBase* inst){
	controler->follow = makeInstPtr(inst);
}
//###################################################################################
// 3人称視点/
//###################################################################################

//---------------------------------------------
// private:
//---------------------------------------------
struct gameCameraControler3rd{
	CameraColtrolerBase;
	InstPtr follow;
	float fovAngle;
	float dist;
	vec3 angle;
	vec3 focus;
};
static void gameCameraControler3rdUpdate(cameraControler* _controler){
	gameCameraControler3rd* controler = (gameCameraControler3rd*)_controler;
	Camera* c = controler->base.camera;
	objBase* follow = getInstPtr(&(controler->follow));//ポインタをもらう　すでに死んでたらnull/
	
	if(follow == NULL) return;//死んでる/

	//angle
	const float cameraAngleXspd = .01f;
	const float cameraAngleYspd = .01f;
	float dx = getMouseDx();
	float dy = getMouseDy();
	if(dx || dy){
		Basis b = createBasis(controler->angle);
		vec3 lastAngle = b.z;
		float h = dx * cameraAngleXspd;
		float v = -dy * cameraAngleYspd;
		//TODO y制限/
		b.z = v3normalize(v3add(b.z, v3add(v3mul(b.x, h), v3mul(b.y, v))));
		const float maxY = .94f;
		if(maxY < fabsf(b.z.y)){
			//横だけ/
			vec3 hOnly = v3normalize(v3add(lastAngle, v3mul(b.x, h)));
			//縦を最大値にする/
			hOnly.y = b.z.y > 0 ? maxY : -maxY;
			//正規化/
			b.z = v3normalize(hOnly);
		}
		controler->angle = b.z;
		vec2 noY = angleToVec2(b.z);

		if(follow->attribute & objAtt_playable){
			vec2Basis* pCameraInput = follow->interfaces->playableInterface.getCameraIn(follow);
			*pCameraInput = createVec2Basis(noY);
		}
	}


	vec3 targetFocus = v3add(follow->render->p, v3mul(v3y, 160.f));
	targetFocus = v3add(follow->render->p, follow->v);
	//場所 + 向き * -距離/
	vec3 targetPos = v3add(targetFocus, v3mul(controler->angle, -controler->dist));

	//線形補完/

	//focus
	const float focusSpd = .7f;
	vec3 focusSub = v3sub(targetFocus, controler->focus);
	controler->focus = v3add(controler->focus, v3mul(focusSub, focusSpd));
	//pos
	const float cameraSpd = .2f;
	vec3 posSub = v3sub(targetPos, c->p);
	c->p = v3add(c->p, v3mul(posSub, cameraSpd));


	//とりあえずそのまま適応/
	vec3 angle = v3normalize(v3sub(controler->focus, c->p));
	//c->p = targetPos;
	c->b = createBasis(angle);
}

//---------------------------------------------
// public:
//---------------------------------------------
void gameCameraControler3rdInitializer(cameraControler* _controler, objBase* follow){
	gameCameraControler3rd* controler = (gameCameraControler3rd*)_controler;
	controler->fovAngle = .25f;
	controler->follow = makeInstPtr(follow);
	controler->angle = v3z;
	controler->dist = 160.f * 10;
	controler->focus = follow->render->p;
	_controler->update = gameCameraControler3rdUpdate;
	_controler->camera->fov = angleToFov(controler->fovAngle);
}

void gameCameraControler3rdSetFollow(gameCameraControler3rd* controler, objBase* inst){
	controler->follow = makeInstPtr(inst);
}

//###################################################################################
// camera controler Base
//###################################################################################

//---------------------------------------------
// private:
//---------------------------------------------
#define AS_UNION_CAMERA_CONTROLER_X(type) type ATTACH(_,type);
typedef union{
	CAMERA_CONTROLER_LIST_X(AS_UNION_CAMERA_CONTROLER_X)
}MaxCameraControlerMemory;

//---------------------------------------------
// public:
//---------------------------------------------

cameraControler* createCameraControler(Camera* camera){
	// --- メモリ確保 --- /

	//最大メモリとしてもらう/
	cameraControler* r = (cameraControler*)gm_allocate(sizeof(MaxCameraControlerMemory));
	//NULL_CHECK(r,"カメラコントローラーの作成でm\allocが塗るぽを返した");
	if(r == NULL) return NULL;//エラーチェック/

	// --- 初期化 --- /

	//与えられたメモリを0初期化/
	*(MaxCameraControlerMemory*)r = (MaxCameraControlerMemory){ 0 };
	//カメラをセット/
	r->camera = camera;
	return (cameraControler*)r;
}

void updateCamera(cameraControler* controler){
	controler->update(controler);
}

//void setCameraControler(cameraControler* controler, CameraControlerInitializer* initializer){
//	initializer->func(controler, initializer->args);
//}
void destroyCameraControler(cameraControler** c){
	//free(*c);
	*c = NULL;
}