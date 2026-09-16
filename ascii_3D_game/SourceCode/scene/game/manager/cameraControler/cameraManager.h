#pragma once
#include"macro/macro.h"

struct Camera;
typedef struct Camera Camera;

// --- X MACRO --- /
#define CAMERA_CONTROLER_LIST_X(X)\
X(debugCameraControler)\
X(gameCameraControler)\
X(gameCameraControler3rd)\
X_MACRO_END

#define AS_ENUM_CAMERA_CONTROLER_X(name) ATTACH(cameraControler_,name),
enum{
	CAMERA_CONTROLER_LIST_X(AS_ENUM_CAMERA_CONTROLER_X)
	AS_ENUM_CAMERA_CONTROLER_X(max)
};

#define AS_TYPEDEF_CAMERA_CONTROLER(type) struct type; typedef struct type type;
CAMERA_CONTROLER_LIST_X(AS_TYPEDEF_CAMERA_CONTROLER)

// --- base --- /

//基底/
typedef struct cameraControler{
	void (*update)(struct cameraControler*);//カメラを操作する関数の関数ポインタ/
	Camera* camera;//基本的にコピー/
}cameraControler;
#define CameraColtrolerBase cameraControler base

//カメラのメモリを確保/
cameraControler* createCameraControler(Camera* camera);
//カメラを更新 まあセットされてる関数を実行してカメラを動かすやつ　基本的にこれを毎フレームやる/
void updateCamera(cameraControler* controler);

// --- カメラのコントローラーをセットするためのやつ --- /

//typedef struct{
//	void* args;//引数をまとめた構造体のポインタ/
//	void (*func)(cameraControler*,void*);//初期化関数/
//}CameraControlerInitializer;//関数ポインタと引数をまとめた構造体/

//!initializerは関数ポインタじゃなくて構造体/
//カメラコントローラーをセットするための関数 でもよくよく考えたら直で呼び出すのんでよかった? これ要らん気がするな　どうやろ/
//void setCameraControler(cameraControler* controler, CameraControlerInitializer* initializer);//結局消した/
void destroyCameraControler(cameraControler** c);

// --- 継承 --- /

//--debug

//自由に動き回れるカメラ操作 ゲームに1ミリも依存しない/
void debugCameraControlerInitializer(cameraControler* _controler);

struct objBase;
typedef struct objBase objBase;
//--player
void gameCameraControlerInitializer(cameraControler* _controler, objBase* arg);
void gameCameraControlerSetFollow(gameCameraControler* _controler, objBase* inst);