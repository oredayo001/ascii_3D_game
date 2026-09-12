#include "engine.h"
#include "common.h"
#include "screen/screen.h"
#include "graphics/render3d.h"
#include "buffer/gameBuff.h"

//---------------------------------------------
// private:
//---------------------------------------------

static void nothingFunc(){ };
static Scene SceneNone = { nothingFunc,nothingFunc,nothingFunc };
static struct{
	int isRunning;
	Scene currentScene;
	SceneSetFunc nextFunc;

	int memolyMarker;
}m;
ISystemContext systemContext;

static void changeScene(){
	m.currentScene.fin();
	SceneInitFunc initFunc = m.nextFunc(&m.currentScene);
	m.nextFunc = NULL;
	initFunc(&systemContext);
}
static void _requestChangeScene(SceneSetFunc f){
	ASSERT(m.nextFunc == NULL, "シーンの二重遷移");
	m.nextFunc = f;
}
//---------------------------------------------
// public:
//---------------------------------------------

void requestChangeScene(SceneSetFunc f){
	_requestChangeScene(f);
}

void requestQuitMsg(){
	m.isRunning = 0;
}

//#############################################
// main
//#############################################

void engineInit(SceneSetFunc firstSceneFunc){
	//メモリの準備/
	gm_d_pushStack();
	m.memolyMarker = gm_getMarker();
	//初期設定/
	m.currentScene = SceneNone;
	m.isRunning = 1;
	_requestChangeScene(firstSceneFunc);
	//グラフィックの初期化/
	gm_d_assertStack(gameBuffStack_engine);
	systemContext.rCtx = createRenderContext(createScreen(), createCamera());
}

int engineUpdate(){
	// --- シーン変更チェック --- /
	if(m.nextFunc)changeScene();

	// --- 更新 --- /
	//入力/
	lib_updateControl(&systemContext.keyStates);
	lib_updateMouse();
	//シーン/
	m.currentScene.update();

	return m.isRunning;
}

void engineRender(){
#if ENABLE_DEBUG + 0
	debugMember.triCntStatic = 0;
	debugMember.triCnt = 0;
#endif
	// --- 更新 --- /
	m.currentScene.render();

	// --- 出力 --- /
	//テキストに変換/
	updateScreen(systemContext.rCtx->sc);
	//出力/
	flushScreen(systemContext.rCtx->sc);
}

void engineFin(){
	m.currentScene.fin();
	//destroyScreen(&(systemContext.rCtx->sc));
	//cameraDestroy(&(systemContext.rCtx->c));
	//renderContextDestroy(&(systemContext.rCtx));
	gm_freeToMarker(m.memolyMarker);
	gm_d_popStack();
}