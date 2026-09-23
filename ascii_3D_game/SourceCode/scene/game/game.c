#include "game.h"
//engine
#include "engine/screen/screen.h"
#include "engine/graphics/render3d.h"
#include "engine/buffer/gameBuff.h"
#include "engine/graphics/loader/textureLoader.h"

//scene
#include "scene/title/title.h"

//managers
#include "scene/commonManager/input.h"
#include "manager/cameraControler/cameraManager.h"
#include "manager/obj/objManager.h"
#include "manager/map/mapManager.h"

//######################################################################
// private:
//######################################################################

static struct{
	// --- コピー --- /
	RenderContext* rCtx;
	const KeyStates* keyStates;
	// --- 作成 --- /
	cameraControler* cameraControler;
	int frame;
	// --- バッファの先頭 --- /
	int memolyMarker;
	// --- debug --- /
	int toggleShading;
	int stage_memolyMarker;
}m;

//test
#include "scene/commonManager/gameModelLoader.h"

#pragma region debug

//#define playerPos (vec3){ 0.f, 5000.f, 0.f }
#define playerPos v3zero

static objPlayer* player = NULL;

static void debug_changeCamera(){
	static int mode = 0;
	mode = !mode;
	switch(mode){
		//--switch--/
	case 0:
		player = (objPlayer*)instanceCreate(obj_player, playerPos);
		gameCameraControlerInitializer(m.cameraControler, (objBase*)player);
		break;
	case 1:
	{
		debugCameraControlerInitializer(m.cameraControler);
		instanceDestroy((objBase*)player);
		player = NULL;
	}
	//--switch end--/
	}
}
#pragma endregion

static void gameInit(ISystemContext* context){
	// --- メモリの準備 --- /
	gm_d_pushStack();
	gm_d_assertStack(gameBuffStack_scene);
	m.memolyMarker = gm_getMarker();
	// --- 上の階層からもらう --- /
	m.rCtx = context->rCtx;
	m.keyStates = &context->keyStates;

	// --- 初期化 --- /
	m.frame = 0;

	// --- 作成 --- /
	m.cameraControler = createCameraControler(m.rCtx->c);

	// --- managers --- /
	initInstances();

#pragma region test
	//test
	m.toggleShading = 1;
	/*
//TODO ここに書いてるやつは基本gameLogicManager的なのが全部やる
モデルの読み込み　マリオの生成場所　その他キャラの生成はstageManager的な奴がやる
でgameLogicManagerがどのstageManagerを使うかはgameDataってのをcontextに入れといて初期化時にそれをlogicManagerに渡しとく
データはgameLogicManagerが常に渡して使うかはgameDataが判断　常にというより必要に応じて　自動セーブならそのタイミング/
	*/

	// --- ここで層がstageになる --- /

	//メモリの準備/
	gm_d_pushStack();
	m.stage_memolyMarker = gm_getMarker();

	//プレイヤー/
	player = (objPlayer*)instanceCreate(obj_player, playerPos);
	instanceCreate(obj_chaser, (vec3){ 312.089f, -500.000f, -324.060f });//(312.089,-500.000,-324.060)
	//カメラ/
	gameCameraControlerInitializer(m.cameraControler, (objBase*)player);
	//テクスチャ/
	initTextureLoader(getMaxTextureID() + 1);
	//モデル/
	int mdlIndexes[2] = { objModel_player,objModel_chaser };
	loadObjModels(mdlIndexes, ARRAY_SIZE(mdlIndexes));
	m.rCtx->c->b = basisZ;

	int stageModelMarker = gm_getMarker_back();
	//loadStageModel(stageModel_stageDemo);
	loadStageModel(stageModel_stage01);
	//test
	//Model3D* playerMdl = getObjMdl(objModel_player);

	pushModel(getStageModel(), (vec3){ 0.f, 0.f, 0.f });
	createMap();//pushModelしたのをマップデータとして作成してる/
	gm_free_back_to_marker(stageModelMarker);
	destroyStageModel();
#pragma endregion
}

static void gameUpdate(){
	m.frame++;

	//入力/
	setKeyStates(m.keyStates);
	updateMouse(.7f);

	//obj
	updateInstances();

	//カメラ/
	updateCamera(m.cameraControler);

	//終了か/
	if(lib_keyboardCheck(vk_esc, m.keyStates->p)) requestChangeScene(titleSetFunc);

	//--debug--/
#pragma region debug
	if(keyboardCheckPressed(vk_ctrl))debug_changeCamera();
	if(keyboardCheckPressed(vk_debugShadingToggle))m.toggleShading = !m.toggleShading;
#pragma endregion
}

static void gameRender(){
	trueBreakPoint(keyboardCheck(vk_debugBreakPoint));

	// --- スクリーンのクリーン --- /
	clearScreen(m.rCtx->sc);

	// --- モデルをスタックに入れる --- /

	//キャラ描画/
	renderInstances();
	//マップ描画/
	renderMap();

	// --- 描画 --- /
	renderStackAll(m.rCtx);

	//_mapDebug_drawGrid(m.rCtx);

	//if(m.toggleShading)shadingScreen(m.rCtx->sc);
}

static void gameFin(){
	// --- stage層 --- /
	gm_d_assertStack(gameBuffStack_stage);
	destroyMap();
	destroyAllObjModels();
	gm_freeToMarker(m.stage_memolyMarker);
	gm_d_popStack();

	// --- scene層 --- /
	finInstances();
	destroyCameraControler(&(m.cameraControler));

	gm_freeToMarker(m.memolyMarker);
	gm_d_popStack();
}

//######################################################################
// public:
//######################################################################

SceneInitFunc gameSetFunc(Scene* scene){
	setSceneFuncs(scene, gameUpdate, gameRender, gameFin);
	return gameInit;
}

int getGameFrame(){
	return m.frame;
}
